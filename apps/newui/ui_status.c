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

/* Colours: the playing/charging indicators in the accent colour, everything
 * else in the theme's status colour (UI_COL_STATUS). The icons are glyphs of
 * the UI font (ui.h UI_G_*), not drawn shapes or bitmaps. The battery is a
 * number: "37%", "+" while it is taking charge, red when low. */

/* The status bar (PLAN.md §4.1): play state on the left, clock in the
 * centre, Bluetooth and battery on the right. Drawn into the current
 * viewport, UI_STATUS_H high. */

#include "config.h"
#include <stdio.h>
#include "lcd.h"
#include "status.h"
#include "timefuncs.h"
#include "string-extra.h"
#include "statusbar.h"
#include "ui.h"

#define BATT_LOW    20     /* below this the battery number turns red */
#define ICON_GAP    6      /* between two status icons */

static char status_title[48];
static int status_width;

void ui_status_title(const char *title)
{
    if (!title)
        title = "";
    if (!strcmp(title, status_title))
        return;
    strmemccpy(status_title, title, sizeof(status_title));
    statusbar_update(true);
}

const char *ui_status_get_title(void)
{
    return status_title;
}

void ui_status_width(int width)
{
    if (width == status_width)
        return;
    status_width = width;
    statusbar_update(true);
}

int ui_status_get_width(void)
{
    return status_width;
}

/* The play state, Bluetooth and key lock are glyphs of the UI font, not
 * drawn shapes. */
static const char *play_glyph(int mode)
{
    switch (mode)
    {
        case STATUS_PLAY:         return UI_G_PLAY;
        case STATUS_PAUSE:        return UI_G_PAUSE;
        case STATUS_FASTFORWARD:  return UI_G_FF;
        case STATUS_FASTBACKWARD: return UI_G_FB;
        case STATUS_RECORD:       return UI_G_REC;
        default:                  return UI_G_STOP; /* stopped */
    }
}

static unsigned glyph_color(int mode)
{
    if (mode == STATUS_PLAY || mode == STATUS_RECORD)
        return ui_accent_highlight();
    if (mode == STATUS_STOP)
        return UI_COL_MUTED;   /* stopped: keep it quiet */
    return UI_COL_STATUS;
}

/* One icon, left aligned at x, vertically centred in the bar */
static void draw_icon(int x, int font, unsigned color, const char *glyph)
{
    ui_text_vcenter(x, 0, UI_STATUS_H, font, color, glyph, 0, -1);
}

/* The icon cluster: play state, key lock, Bluetooth */
static int icons_width(int font, const struct ui_status *st)
{
    int w = ui_text_width(font, play_glyph(st->playmode)) + ICON_GAP;
    if (st->keylock)   w += ui_text_width(font, UI_G_LOCK) + ICON_GAP;
    if (st->bluetooth) w += ui_text_width(font, UI_G_BT);
    return w - ICON_GAP;
}

/* The icons, left aligned at x; returns their total width */
static int draw_icons(int x, int font, const struct ui_status *st)
{
    int cx = x;
    const char *g = play_glyph(st->playmode);

    draw_icon(cx, font, glyph_color(st->playmode), g);
    cx += ui_text_width(font, g) + ICON_GAP;

    if (st->keylock)
    {
        draw_icon(cx, font, UI_COL_STATUS, UI_G_LOCK);
        cx += ui_text_width(font, UI_G_LOCK) + ICON_GAP;
    }
    if (st->bluetooth)
    {
        /* 1 = on, 2 = on and streaming */
        draw_icon(cx, font, st->bluetooth > 1 ? ui_accent_highlight() : UI_COL_STATUS,
                  UI_G_BT);
    }
    return icons_width(font, st);
}

/* The battery as a number, terminal style: "37%", "+" when it is taking
 * charge, red when low, "--%" when there is no reading, drawn with its
 * right edge at x_right. */
static void draw_battery(int x_right, int w, const char *text, int percent,
                         bool charging)
{
    ui_text_vcenter(x_right - w, 0, UI_STATUS_H, ui_font(UI_FONT_SMALL),
                    percent >= 0 && percent <= BATT_LOW && !charging ?
                        ui_red() : UI_COL_STATUS,
                    text, 0, -1);
}

/* fills buf and returns its width; "--%" when there is no reading */
static int battery_text(char *buf, size_t size, const struct ui_status *st)
{
    if (st->battery < 0)
        snprintf(buf, size, "--%%");
    else
        snprintf(buf, size, "%d%%", st->battery > 100 ? 100 : st->battery);
    if (st->charging)
        strcat(buf, "+");
    return ui_text_width(ui_font(UI_FONT_SMALL), buf);
}

/* the time, "14:41" or "2:41 PM"; 0 when the clock is off */
static int clock_text(char *buf, size_t size, const struct ui_status *st)
{
    if (!st->time)
        return 0;

    int h = st->time->tm_hour, m = st->time->tm_min;
    if (st->clock_24h)
        snprintf(buf, size, "%02d:%02d", h, m);
    else
        snprintf(buf, size, "%d:%02d %s",
                 (h % 12) ? h % 12 : 12, m, h < 12 ? "AM" : "PM");
    return ui_text_width(ui_font(UI_FONT_SMALL), buf);
}

void ui_status_draw(const struct ui_status *st)
{
    struct viewport *vp = lcd_current_viewport;
    int font = ui_font(UI_FONT_SMALL);

    {
        /* The bar is the same colour as the rest of the screen: no gradient,
           no rule, no band. It is just the top 22 px of the page, like a
           terminal's header line; only its text and icons stand out. */
        ui_fill_rect(0, 0, vp->width, vp->height, UI_COL_BG);
    }

    const bool titled = st->title && st->title[0];
    int icons = icons_width(font, st);
    char clock[16], batt[16];
    int clock_w = clock_text(clock, sizeof clock, st);
    int batt_w = st->battery_visible ? battery_text(batt, sizeof batt, st) : 0;

    if (titled)
    {
        /* the screen's name left, the clock in the middle, the icons and
           the battery on the right */
        int right = vp->width - UI_MARGIN;   /* the battery's right edge */
        int block = icons + batt_w + (icons && batt_w ? ICON_GAP : 0);
        int room = right - block;           /* the free space ends here */

        /* the clock is centred in the bar as long as the title still reads
           beside it; otherwise it tucks in against the icons */
        int cx = room - clock_w;
        if (clock_w &&
            ui_text_width(ui_font(UI_FONT_BODY), st->title) + clock_w + 2 * ICON_GAP
                <= room - UI_MARGIN)
            cx = (vp->width - clock_w) / 2;
        cx = MAX(cx, UI_MARGIN);

        if (batt_w)
            draw_battery(right, batt_w, batt, st->battery, st->charging);
        if (icons)
            draw_icons(right - block, font, st);
        if (clock_w)
            ui_text_vcenter(cx, 0, UI_STATUS_H, font, UI_COL_STATUS, clock, 0,
                            -1);
        ui_text_vcenter(UI_MARGIN, 0, UI_STATUS_H, ui_font(UI_FONT_BODY),
                        UI_COL_STATUS, st->title, MAX(0, cx - ICON_GAP - UI_MARGIN),
                        -1);
    }
    else
    {
        /* left: the icons, the clock centred, the battery on the right */
        if (icons)
            draw_icons(UI_MARGIN, font, st);

        if (clock_w)
            ui_text_vcenter((vp->width - clock_w) / 2, 0, UI_STATUS_H, font,
                            UI_COL_STATUS, clock, 0, -1);

        if (batt_w)
            draw_battery(vp->width - UI_MARGIN, batt_w, batt, st->battery,
                         st->charging);
    }
}
