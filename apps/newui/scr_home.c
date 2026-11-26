/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* Home (PLAN.md §4.2): the menu list, plain - one row per screen, left
 * aligned at the margin, the chosen row in inverse video. No banner, no
 * numbering, no cover art: covers live in Cover Flow. Along the bottom edge
 * a grey readout of the music database (tracks and total playing time).
 *
 * Wheel/Prev/Next move (wrapping), Centre opens, Back returns to the first
 * row, holding Back opens Now Playing. */

#include "config.h"
#include "action.h"
#include "audio.h"
#include "kernel.h"
#include "lang.h"
#include "language.h"
#include "lcd.h"
#include "menu.h"
#include "misc.h"
#include "root_menu.h"
#include "settings.h"
#include "viewport.h"
#include "ui.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif
#ifdef HAVE_BLUETOOTH
#include "menus/exported_menus.h"
#endif

#define HOME_ROW_H    UI_ROW_H    /* the same row pitch as every other list */

enum home_target {
    HOME_NOW_PLAYING,
    HOME_MUSIC,
    HOME_COVERFLOW,
    HOME_FILES,
    HOME_PLAYLISTS,
#ifdef HAVE_BLUETOOTH
    HOME_BLUETOOTH,
#endif
    HOME_SETTINGS,
    HOME_SYSTEM,
    HOME_COUNT
};

struct home_item {
    int lang_id;
    int screen;     /* GO_TO_* to return, or -1: handled here */
};

static const struct home_item items[HOME_COUNT] = {
    [HOME_NOW_PLAYING] = { LANG_NOW_PLAYING,  GO_TO_WPS },
    [HOME_MUSIC]       = { LANG_NEWUI_MUSIC,  GO_TO_DBBROWSER },
    [HOME_COVERFLOW]   = { LANG_NEWUI_COVERFLOW, -1 },
    [HOME_FILES]       = { LANG_DIR_BROWSER,  GO_TO_FILEBROWSER },
    [HOME_PLAYLISTS]   = { LANG_PLAYLISTS,    GO_TO_PLAYLISTS_SCREEN },
#ifdef HAVE_BLUETOOTH
    [HOME_BLUETOOTH]   = { LANG_BLUETOOTH,    -1 },
#endif
    [HOME_SETTINGS]    = { LANG_SETTINGS,     GO_TO_MAINMENU },
    [HOME_SYSTEM]      = { LANG_SYSTEM,       GO_TO_SYSTEM_SCREEN },
};

/* Now Playing is listed while a track is loaded, or one can be resumed */
static bool can_play(void)
{
    return audio_status() || global_status.resume_index != -1;
}

/* true when audio status or the playing track differs from the last call */
static bool state_changed(void)
{
    static int last_status = -1;
    static unsigned long last_hash;
    struct mp3entry *id3 = audio_status() ? audio_current_track() : NULL;
    unsigned long hash = 5381;
    if (id3)
        for (const char *p = id3->path; *p; p++)
            hash = hash * 33 + (unsigned char)*p;
    bool changed = audio_status() != last_status || hash != last_hash;
    last_status = audio_status();
    last_hash = hash;
    return changed;
}

static int visible_items(enum home_target *out)
{
    int n = 0;
    for (int i = 0; i < HOME_COUNT; i++)
    {
        if (i == HOME_NOW_PLAYING && !can_play())
            continue;
        out[n++] = i;
    }
    return n;
}

static const char *item_label(enum home_target t)
{
    if (t == HOME_NOW_PLAYING && !audio_status())
        return str(LANG_RESUME_PLAYBACK);
    return str(items[t].lang_id);
}

#ifdef HAVE_TAGCACHE
/* The music database's readout, stuck to the bottom of Home: how many tracks
 * the database holds and how long they play, in grey. The total is summed in
 * one pass over the tag cache and cached; the pass runs again only when the
 * database has been scanned or rebuilt, so spinning the wheel costs nothing.
 * While the database is still building the line says so, and Home refreshes
 * it as the build finishes. */
static long long db_total_ms;             /* tag_length summed over the entries */
static int db_tracks = 0;                 /* how many entries that total covers */
static unsigned long db_scanned_key = 0;  /* state the total was computed for */
static long db_scanned_tick = 0;
static bool db_scanned = false;
static char db_text[64];                  /* the line as Home last drew it */

/* Anything that moves when the database is scanned or rebuilt. total_entries
 * alone is not enough: it is only refreshed when the master header is read, so
 * a rebuild that has not been opened yet would leave it at its old value. */
static unsigned long db_key(const struct tagcache_stat *stat)
{
    return (unsigned long)stat->total_entries * 4096
         + (unsigned long)stat->processed_entries * 64
         + (unsigned long)stat->commit_step;
}

static void db_scan(const struct tagcache_stat *stat)
{
    struct tagcache_search tcs;
    /* as wide as Cover Flow's: a path longer than the buffer makes the
     * search give up, and the count would come back short */
    char path[MAX_PATH];
    long long total = 0;
    int n = 0;

    if (!tagcache_search(&tcs, tag_filename))
        return;
    while (tagcache_get_next(&tcs, path, sizeof path))
    {
        long len = tagcache_get_numeric(&tcs, tag_length);
        if (len > 0)
            total += len;
        n++;
    }
    tagcache_search_finish(&tcs);

    db_total_ms = total;
    db_tracks = n;
    db_scanned_key = db_key(stat);
    db_scanned_tick = current_tick;
    db_scanned = true;
}

/* "42 min", "12 h 34 min", "3 d 12 h" */
static void fmt_hours(char *buf, size_t len, long long ms)
{
    long long min = ms / 60000;

    if (min < 60)
        snprintf(buf, len, "%lld min", min);
    else if (min < 24 * 60)
        snprintf(buf, len, "%lld h %02lld min", min / 60, min % 60);
    else
        snprintf(buf, len, "%lld d %lld h", min / 1440, (min / 60) % 24);
}

/* Usable means searchable: a database committing its indices is not. */
static bool db_ready(const struct tagcache_stat *stat)
{
    return tagcache_is_usable() && stat->commit_step == 0;
}

static void db_readout(char *text, size_t len)
{
    struct tagcache_stat *stat = tagcache_get_stat();
    char time[32];

    if (!db_ready(stat))
    {
        int step = stat->commit_step, max = tagcache_get_max_commit_step();
        if (step > 0 && max > step)
            snprintf(text, len, "%s [%d/%d]", str(LANG_NEWUI_DB_UNREADY),
                     step, max);
        else
            snprintf(text, len, "%s", str(LANG_NEWUI_DB_UNREADY));
        return;
    }

    if (!db_scanned ||
        (db_key(stat) != db_scanned_key &&
         /* the database moved; a full pass is not cheap, so while it is
           scanning do not repeat it more than once every few seconds */
         TIME_AFTER(current_tick, db_scanned_tick + 5 * HZ)))
        db_scan(stat);

    if (db_tracks <= 0)
        snprintf(text, len, "%s", str(LANG_NEWUI_DB_EMPTY));
    else
    {
        fmt_hours(time, sizeof time, db_total_ms);
        snprintf(text, len,
                 db_tracks == 1 ? str(LANG_NEWUI_DB_ONE)
                                : str(LANG_NEWUI_DB_TOTAL),
                 db_tracks, time);
    }
}

static void db_line(struct viewport *vp, int rows_bottom)
{
    db_readout(db_text, sizeof db_text);

    if (rows_bottom <= vp->height - UI_ROW_H)
        ui_text_vcenter(UI_MARGIN, vp->height - UI_ROW_H, UI_ROW_H,
                        ui_font(UI_FONT_ROW), UI_COL_TEXT2, db_text,
                        vp->width - 2 * UI_MARGIN, -1);
}

/* Should Home redraw because the readout changed under it? */
static bool db_readout_changed(void)
{
    char text[sizeof db_text];

    db_readout(text, sizeof text);
    return strcmp(text, db_text) != 0;
}
#endif /* HAVE_TAGCACHE */

static void draw(struct viewport *vp, const enum home_target *list, int n,
                 int sel)
{
    struct viewport *last = lcd_set_viewport(vp);
    const int row = ui_font(UI_FONT_ROW);
    const int text_w = vp->width - 2 * UI_MARGIN;
    int y = 0;

    ui_fill_rect(0, 0, vp->width, vp->height, UI_COL_BG);

    for (int i = 0; i < n; i++)
    {
        const bool on = i == sel;
        if (on)
            ui_selection(0, y, vp->width, HOME_ROW_H);
        ui_text_vcenter(UI_MARGIN, y, HOME_ROW_H, row,
                        on ? ui_sel_text() : UI_COL_TEXT,
                        item_label(list[i]), text_w, -1);
        y += HOME_ROW_H;
    }

#ifdef HAVE_TAGCACHE
    db_line(vp, y);
#endif

    lcd_set_viewport(vp);

    /* slide in if a screen change asked for it */
    if (!ui_transition_run(vp->x, vp->y, vp->width, vp->height))
        lcd_update_viewport();
    lcd_set_viewport(last);
}

/* *selected holds the selected item (enum home_target), not its row, so
 * the selection stays on the same item when Now Playing comes and goes */
int ui_home(int *selected)
{
    enum home_target list[HOME_COUNT];
    struct viewport vp;
    int n, sel = 0;
    int target = selected ? *selected : 0;
    bool redraw = true;
    int ret = GO_TO_ROOT;

    viewport_set_defaults(&vp, SCREEN_MAIN);
    ui_status_width(LCD_WIDTH);
    ui_status_title(str(LANG_NEWUI_HOME_TITLE));

    while (true)
    {
        n = visible_items(list);
        sel = 0;
        for (int i = 0; i < n; i++)
            if ((int)list[i] == target)
                sel = i;
        target = list[sel];

        if (redraw)
        {
            draw(&vp, list, n, sel);
            redraw = false;
        }

        int action = get_action(CONTEXT_MAINMENU, HZ);
        switch (action)
        {
            case ACTION_STD_NEXT:
            case ACTION_STD_NEXTREPEAT:
                target = list[(sel + 1) % n];
                redraw = true;
                break;
            case ACTION_STD_PREV:
            case ACTION_STD_PREVREPEAT:
                target = list[(sel + n - 1) % n];
                redraw = true;
                break;
            case ACTION_STD_CANCEL:
                target = list[0];
                redraw = true;
                break;
            case ACTION_TREE_WPS:
                if (can_play())
                {
                    ret = GO_TO_WPS;
                    goto out;
                }
                break;
            case ACTION_STD_OK:
            {
                enum home_target t = list[sel];
                ui_transition(1);
                ui_status_width(0);
                if (t == HOME_COVERFLOW)
                {
                    int r = ui_coverflow();
                    if (r != GO_TO_ROOT)
                    {
                        ret = r;
                        goto out;
                    }
                    ui_transition(-1);
                    ui_status_width(LCD_WIDTH);
                    ui_status_title(str(LANG_NEWUI_HOME_TITLE));
                    redraw = true;
                    break;
                }
#ifdef HAVE_BLUETOOTH
                if (t == HOME_BLUETOOTH)
                {
                    if (do_menu(&bluetooth_menu, NULL, NULL, false) == MENU_ATTACHED_USB)
                    {
                        ret = MENU_ATTACHED_USB;
                        goto out;
                    }
                    ui_transition(-1);
                    ui_status_width(LCD_WIDTH);
                    ui_status_title(str(LANG_NEWUI_HOME_TITLE));
                    redraw = true;
                    break;
                }
#endif
                ret = items[t].screen;
                goto out;
            }
            case ACTION_NONE:
                /* the status bar redraws itself; redraw Home when the
                   track or the play state changed (Now Playing appears
                   and goes away, the battery, the clock), or when the
                   database finished building behind the scenes */
                if (state_changed())
                    redraw = true;
#ifdef HAVE_TAGCACHE
                else if (db_readout_changed())
                    redraw = true;
#endif
                break;
            default:
                if (default_event_handler(action) == SYS_USB_CONNECTED)
                {
                    ret = MENU_ATTACHED_USB;
                    goto out;
                }
                break;
        }
    }
out:
    ui_status_width(0);
    if (selected)
        *selected = target;
    return ret;
}
