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

/* System > System Status: what the player is doing right now - CPU load,
 * memory, storage, IO, the database, uptime - as a text readout. The
 * numbers are live: the screen redraws once a second while it is open. */

#include "config.h"
#include <stdio.h>
#include "action.h"
#include "audio.h"
#include "core_alloc.h"
#include "kernel.h"
#include "lang.h"
#include "language.h"
#include "lcd.h"
#include "metadata.h"
#include "misc.h"
#include "mv.h"
#include "pathfuncs.h"
#include "storage.h"
#include "viewport.h"
#include "ui.h"
#ifdef HAVE_TAGCACHE
#include "tagcache.h"
#endif

#define ST_LINE_H  UI_LINE_H          /* one readout line */
#define ST_COL     (UI_MARGIN + 66)   /* where the values start */
#define ST_BAR_H   6                  /* the CPU load bar */

/* bytes as "31.0 GB", "16.4 MB", "912 KB" or "512 B" */
static void fmt_size(char *buf, size_t len, unsigned long long b)
{
    if (b >= 1024ULL * 1024 * 1024)
        snprintf(buf, len, "%llu.%llu GB", b / (1024ULL * 1024 * 1024),
                 (b * 10 / (1024ULL * 1024 * 1024)) % 10);
    else if (b >= 1024 * 1024)
        snprintf(buf, len, "%llu.%llu MB", b / (1024 * 1024),
                 (b * 10 / (1024 * 1024)) % 10);
    else if (b >= 1024)
        snprintf(buf, len, "%llu KB", b / 1024);
    else
        snprintf(buf, len, "%llu B", b);
}

/* bytes with what they are: "31.0 MB pool" */
static void named(char *buf, size_t len, unsigned long long b, const char *what)
{
    fmt_size(buf, len, b);
    snprintf(buf + strlen(buf), len - strlen(buf), " %s", what);
}

/* "1:23:45" */
static void fmt_time(char *buf, size_t len, long s)
{
    if (s >= 3600)
        snprintf(buf, len, "%ld:%02ld:%02ld", s / 3600, (s / 60) % 60, s % 60);
    else
        snprintf(buf, len, "%ld:%02ld", s / 60, s % 60);
}

/* percent of the CPU not idle; -1 where the target doesn't measure it */
static int cpu_busy(void)
{
#ifdef X1000_CPUIDLE_STATS
    return (1000 - __cpu_idle_cur + 5) / 10;
#else
    return -1;
#endif
}

/* two parts joined by the readout's separator */
static void join(char *out, size_t len, const char *a, const char *b)
{
    snprintf(out, len, "%s%s%s", a, UI_SEP, b);
}

/* "label  value", the value starting at ST_COL; returns the next line */
static int row(int y, const char *label, const char *value)
{
    const int font = ui_font(UI_FONT_ROW);
    ui_text_vcenter(UI_MARGIN, y, ST_LINE_H, font, UI_COL_MUTED, label,
                    ST_COL - UI_MARGIN, -1);
    ui_text_vcenter(ST_COL, y, ST_LINE_H, font, UI_COL_TEXT, value,
                    LCD_WIDTH - UI_MARGIN - ST_COL, -1);
    return y + ST_LINE_H;
}

static void draw(struct viewport *vp)
{
    struct viewport *last = lcd_set_viewport(vp);
    const int font = ui_font(UI_FONT_ROW), title = ui_font(UI_FONT_TITLE);
    char val[96], a[32], b[32];
    int y = 4;

    ui_status_title(str(LANG_NEWUI_STATUS));
    ui_fill_rect(0, 0, vp->width, vp->height, UI_COL_BG);

    ui_text_vcenter(UI_MARGIN, y, ST_LINE_H, title, UI_COL_TEXT,
                    str(LANG_NEWUI_STATUS), 0, -1);
    y += ST_LINE_H + 6;

    /* CPU: the load as a bar, the number at the right */
    const int load = cpu_busy();          /* -1 when not measured */
    const int busy = MIN(MAX(load, 0), 100);
    char pct[8];
    if (load >= 0)
        snprintf(pct, sizeof pct, "%d%%", busy);
    else
        snprintf(pct, sizeof pct, "--%%");

    const int pw = ui_text_width(font, pct);
    const int bx = ST_COL, bw = vp->width - UI_MARGIN - pw - 8 - ST_COL;
    ui_text_vcenter(UI_MARGIN, y, ST_LINE_H, font, UI_COL_MUTED, "CPU",
                    ST_COL - UI_MARGIN, -1);
    ui_fill_rect(bx, y + (ST_LINE_H - ST_BAR_H) / 2, bw, ST_BAR_H,
                 ui_col(UI_C_BAR_LINE));
    if (busy > 0)
        ui_fill_rect(bx, y + (ST_LINE_H - ST_BAR_H) / 2, bw * busy / 100,
                     ST_BAR_H, ui_accent_highlight());
    ui_text_vcenter(vp->width - UI_MARGIN - pw, y, ST_LINE_H, font,
                    UI_COL_TEXT, pct, pw + 4, 1);
    y += ST_LINE_H;

    /* memory: the core pool is everything Rockbox allocates from; the
       codec and spare buffers sit beside it, the rest is the firmware */
    const size_t pool = core_pool_size(), free = core_available();
    const size_t buffers = CODEC_SIZE + PLUGIN_BUFFER_SIZE;
    const size_t firmware =
        (size_t)MEMORYSIZE * 1024 * 1024 > pool + buffers
            ? (size_t)MEMORYSIZE * 1024 * 1024 - pool - buffers : 0;

    named(a, sizeof a, pool, "pool");
    named(b, sizeof b, free, "free");
    join(val, sizeof val, a, b);
    y = row(y, "RAM", val);

    named(a, sizeof a, pool > free ? pool - free : 0, "used");
    named(b, sizeof b, core_allocatable(), "largest");
    join(val, sizeof val, a, b);
    y = row(y, " core", val);

    named(a, sizeof a, CODEC_SIZE, "codec");
    named(b, sizeof b, PLUGIN_BUFFER_SIZE, "spare");
    join(val, sizeof val, a, b);
    y = row(y, " buffers", val);

    if (firmware)
    {
        fmt_size(a, sizeof a, firmware);
        y = row(y, " firmware", a);
    }

    /* storage */
    for (int vol = 0; vol < NUM_VOLUMES; vol++)
    {
        sector_t size, avail;
        char name[VOL_MAX_LEN + 1];

        volume_size(vol, &size, &avail);
        if (!size)
            continue;
        if (get_volume_name(vol, name) <= 0)
            snprintf(name, sizeof name, "vol %d", vol);
        /* the volume name comes decorated as "<microSD0>" */
        if (name[0] == '<')
            memmove(name, name + 1, strlen(name) - 1);
        while (name[0] && name[strlen(name) - 1] == '>')
            name[strlen(name) - 1] = '\0';

        fmt_size(val, sizeof val, (unsigned long long)size * 512);
        snprintf(a, sizeof a, "%s %s", name, val);
        named(b, sizeof b, (unsigned long long)avail * 512, "free");
        join(val, sizeof val, a, b);
        y = row(y, "storage", val);

        named(a, sizeof a, 512, "sectors");
        named(b, sizeof b, volume_get_cluster_size(vol), "clusters");
        join(val, sizeof val, a, b);
        y = row(y, " format", val);
    }

    /* IO since boot */
    uint64_t rd, wr;
    storage_io_stats(&rd, &wr);
    named(a, sizeof a, rd, "read");
    named(b, sizeof b, wr, "written");
    join(val, sizeof val, a, b);
    y = row(y, "IO", val);

    struct mp3entry *id3 = audio_status() ? audio_current_track() : NULL;
    if (id3 && ui_file_line(id3, val, sizeof val))
        y = row(y, "playing", val);

#ifdef HAVE_TAGCACHE
    struct tagcache_stat *stat = tagcache_get_stat();
    snprintf(a, sizeof a, "%d %s", stat->total_entries,
             stat->total_entries == 1 ? "track" : "tracks");
    named(b, sizeof b, (unsigned long long)stat->ramcache_used, "in RAM");
    join(val, sizeof val, a, b);
    y = row(y, "database", val);
#endif

    fmt_time(a, sizeof a, current_tick / HZ);
    y = row(y, "uptime", a);

    if (!ui_transition_run(vp->x, vp->y, vp->width, vp->height))
        lcd_update_viewport();
    lcd_set_viewport(last);
}

int ui_stats(void)
{
    struct viewport vp;
    viewport_set_defaults(&vp, SCREEN_MAIN);
    draw(&vp);

    while (true)
    {
        int action = get_action(CONTEXT_STD, HZ);
        if (action == ACTION_STD_CANCEL || action == ACTION_STD_MENU)
            return 0;
        if (action == ACTION_NONE)
            draw(&vp);
        else if (default_event_handler(action) == SYS_USB_CONNECTED)
            return 1;
    }
}
