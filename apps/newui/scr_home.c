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

/* Home (PLAN.md §4.2), console style: one column of text, a rule under
 * the banner, a '>' cursor and the chosen line in inverse video. No cover
 * art: covers live in Cover Flow.
 *
 * Wheel/Prev/Next move (wrapping), Centre opens, Back returns to the first
 * row, holding Back opens Now Playing. */

#include "config.h"
#include <stdio.h>
#include "action.h"
#include "audio.h"
#include "font.h"
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
#ifdef HAVE_BLUETOOTH
#include "menus/exported_menus.h"
#endif

/* Tiny5 Mono advances 6 px per character, so the menu is a text grid */
#define HOME_COL      6     /* one character column */

#define HOME_LINE_H   UI_LINE_H   /* the menu line pitch */
#define BLINK_TICKS   50    /* the cursor flips every half second */

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

/* has the cursor just flipped? redraw Home only on those ticks */
static bool blink_changed(void)
{
    static int phase = -1;
    int p = (current_tick / BLINK_TICKS) & 1;
    if (p == phase)
        return false;
    phase = p;
    return true;
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

static void draw(struct viewport *vp, const enum home_target *list, int n,
                 int sel)
{
    struct viewport *last = lcd_set_viewport(vp);
    const int title = ui_font(UI_FONT_TITLE);
    const int row = ui_font(UI_FONT_ROW);
    const int small = ui_font(UI_FONT_SMALL);
    char line[64];
    int y = 4;

    ui_fill_rect(0, 0, vp->width, vp->height, UI_COL_BG);

    /* the prompt: the player's name, a '>', and a blinking cursor */
    snprintf(line, sizeof line, "%s>", str(LANG_NEWUI_HOME_TITLE));
    ui_text_vcenter(UI_MARGIN, y, HOME_LINE_H, title, UI_COL_TEXT, line, 0, -1);
    if ((current_tick / BLINK_TICKS) & 1)
        ui_fill_rect(UI_MARGIN + HOME_COL * (int)strlen(line), y + 1,
                     HOME_COL, font_get(title)->height, UI_COL_TEXT);
    y += HOME_LINE_H;

    ui_fill_rect(0, y, vp->width, 1, ui_col(UI_C_BAR_LINE));
    y += 2;

    /* the menu: numbered lines, the chosen one in inverse video */
    for (int i = 0; i < n; i++)
    {
        bool on = i == sel;
        snprintf(line, sizeof line, "%2d %s", i + 1, item_label(list[i]));
        if (on)
            ui_fill_rect(0, y, vp->width, HOME_LINE_H, ui_sel_bg());
        ui_text_vcenter(UI_MARGIN, y, HOME_LINE_H, row,
                        on ? ui_sel_text() : UI_COL_TEXT, line,
                        vp->width - UI_MARGIN, -1);
        y += HOME_LINE_H;
    }

    /* the status line: where the cursor is */
    ui_fill_rect(0, y + 2, vp->width, 1, ui_col(UI_C_BAR_LINE));
    snprintf(line, sizeof line, "%d/%d", sel + 1, n);
    ui_text_vcenter(UI_MARGIN, y + 4, HOME_LINE_H, small, UI_COL_MUTED,
                    line, 0, -1);

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
                   track or the play state changed (Now Playing, art), or
                   when the cursor blinks */
                if (state_changed() || blink_changed())
                    redraw = true;
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
