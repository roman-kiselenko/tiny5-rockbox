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
#define BATT_RED    LCD_RGBPACK(0xf4, 0x62, 0x4c)
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
        return ui_accent();
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
        draw_icon(cx, font, st->bluetooth > 1 ? ui_accent() : UI_COL_STATUS,
                  UI_G_BT);
    }
    return icons_width(font, st);
}

/* The battery as a number, terminal style: "37%", "+" when it is taking
 * charge, red when low, "--%" when there is no reading. Its right edge is
 * at x_right; returns the width it took. */
static int draw_battery(int x_right, int percent, bool charging)
{
    const int font = ui_font(UI_FONT_SMALL);
    char buf[16];

    if (percent < 0)
        snprintf(buf, sizeof buf, "--%%");
    else
        snprintf(buf, sizeof buf, "%d%%", percent > 100 ? 100 : percent);
    if (charging)
        strcat(buf, "+");

    int w = ui_text_width(font, buf);
    ui_text_vcenter(x_right - w, 0, UI_STATUS_H, font,
                    percent >= 0 && percent <= BATT_LOW && !charging ?
                        BATT_RED : UI_COL_STATUS,
                    buf, 0, -1);
    return w;
}

void ui_status_draw(const struct ui_status *st)
{
    struct viewport *vp = lcd_current_viewport;
    int font = ui_font(UI_FONT_SMALL);

    {
        /* the iPod's bar: a vertical gradient (silver, or dark grey) and a
           hairline */
        unsigned top = ui_col(UI_C_BAR_TOP), bot = ui_col(UI_C_BAR_BOTTOM);
        int h = vp->height - 1;
        for (int y = 0; y < h; y++)
        {
            int t = y * 256 / (h - 1);
            unsigned c = LCD_RGBPACK(
                RGB_UNPACK_RED(top) + ((RGB_UNPACK_RED(bot) - RGB_UNPACK_RED(top)) * t >> 8),
                RGB_UNPACK_GREEN(top) + ((RGB_UNPACK_GREEN(bot) - RGB_UNPACK_GREEN(top)) * t >> 8),
                RGB_UNPACK_BLUE(top) + ((RGB_UNPACK_BLUE(bot) - RGB_UNPACK_BLUE(top)) * t >> 8));
            ui_fill_rect(0, y, vp->width, 1, c);
        }
        ui_fill_rect(0, h, vp->width, 1, ui_col(UI_C_BAR_LINE));
    }

    const bool titled = st->title && st->title[0];
    int icons = icons_width(font, st);

    if (titled)
    {
        /* the iPod's layout: the screen's name left, the icons and the
           battery on the right */
        int x = vp->width - UI_MARGIN;
        if (st->battery_visible)
            x -= draw_battery(x, st->battery, st->charging) + ICON_GAP;
        if (icons)
            draw_icons(x - icons, font, st);
        ui_text_vcenter(UI_MARGIN, 0, UI_STATUS_H, ui_font(UI_FONT_BODY),
                        UI_COL_STATUS, st->title, x - icons - ICON_GAP - UI_MARGIN,
                        -1);
    }
    else
    {
        /* left: the icons, the clock centred */
        if (icons)
            draw_icons(UI_MARGIN, font, st);

        if (st->time)
        {
            char buf[16];
            int h = st->time->tm_hour, m = st->time->tm_min;
            if (st->clock_24h)
                snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
            else
                snprintf(buf, sizeof(buf), "%d:%02d %s",
                         (h % 12) ? h % 12 : 12, m, h < 12 ? "AM" : "PM");
            ui_text_vcenter(0, 0, UI_STATUS_H, font, UI_COL_STATUS, buf,
                            vp->width, 0);
        }
    }
}
