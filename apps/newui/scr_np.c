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

/* Now Playing's drawing (PLAN.md §4.5), console style: the track as text
 * and one progress line at the bottom. The screen's logic stays in
 * apps/gui/wps.c, which calls ui_np_draw():
 *
 *   Come Together
 *   The Beatles
 *   Abbey Road
 *   3 of 12 · shuffle
 *   FLAC · 921 kbps · 44.1 kHz
 *
 *   ──────────────────────────────────
 *   0:15 ███████-------------   -6:05
 *
 * When the volume changes, the band shows it for a moment instead of the
 * progress. */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "lcd.h"
#include "font.h"
#include "metadata.h"
#include "screen_access.h"
#include "audio.h"
#include "playlist.h"
#include "settings.h"
#include "sound.h"
#include "viewport.h"
#include "wps.h"
#include "lang.h"
#include "language.h"
#include "ui.h"

#define NP_LINE_H      UI_LINE_H     /* one line of the readout */
#define NPL_BAND_Y     (LCD_HEIGHT - UI_STATUS_H - 42) /* rule + progress */
#define NPL_BAR_H      8
#define OVERLAY_TICKS  (HZ * 3 / 2)

static long volume_until;   /* tick until which the volume overlay shows */
static bool volume_shown;
static bool scrubbing;

static void format_mmss(char *buf, size_t len, long ms, bool minus)
{
    long s = ms / 1000;
    if (s < 0)
        s = 0;
    if (s >= 3600)
        snprintf(buf, len, "%s%ld:%02ld:%02ld", minus ? "-" : "",
                 s / 3600, (s / 60) % 60, s % 60);
    else
        snprintf(buf, len, "%s%ld:%02ld", minus ? "-" : "", s / 60, s % 60);
}

/* title, falling back to the file name without its extension */
/* "0:04 ███████---- -5:32": the empty track in the rule's grey, the
 * played part in the accent. No outline, no gloss. */
static void draw_bar(int bx, int by, int bw, long part, long total)
{
    const unsigned track = ui_col(UI_C_BAR_LINE);
    int fill = total > 0 ? (int)((long long)bw * part / total) : 0;
    ui_fill_rect(bx, by, bw, NPL_BAR_H, track);
    if (fill > 0)
        ui_fill_rect(bx, by, MIN(fill, bw), NPL_BAR_H, ui_accent());
}

/* While scrubbing: the same bar with a block where the cursor is */
#define SCRUB_W  4
static void draw_scrub_bar(int bx, int by, int bw, long part, long total)
{
    int x = bx + (total > 0 ? (int)((long long)bw * part / total) : 0);
    ui_fill_rect(bx, by, bw, NPL_BAR_H, ui_col(UI_C_BAR_LINE));
    ui_fill_rect(x - SCRUB_W / 2, by, SCRUB_W, NPL_BAR_H, UI_COL_FILL);
}

/* the volume in place of the progress line */
static void draw_volume_band(struct viewport *vp)
{
    const int font = ui_font(UI_FONT_ROW);
    const int band_h = vp->height - NPL_BAND_Y;
    int lo = sound_min(SOUND_VOLUME), hi = sound_max(SOUND_VOLUME);
    char label[32];

    snprintf(label, sizeof label, "%s %d%%", str(LANG_VOLUME),
             hi > lo ? (global_status.volume - lo) * 100 / (hi - lo) : 0);
    int lw = ui_text_width(font, label);
    int bx = UI_MARGIN + lw + UI_GAP;
    int bw = vp->width - UI_MARGIN - bx;
    int by = NPL_BAND_Y + 1 + (band_h - 1 - NPL_BAR_H) / 2;

    ui_fill_rect(0, NPL_BAND_Y, vp->width, band_h, UI_COL_BG);
    ui_fill_rect(0, NPL_BAND_Y, vp->width, 1, ui_col(UI_C_BAR_LINE));
    ui_text_vcenter(UI_MARGIN, by, NPL_BAR_H, font, UI_COL_TEXT,
                    label, lw + 4, -1);
    draw_bar(bx, by, bw, global_status.volume - lo, hi - lo);
}

static void draw_band(struct viewport *vp, const struct mp3entry *id3,
                      long elapsed)
{
    const int font = ui_font(UI_FONT_ROW);
    const int band_h = vp->height - NPL_BAND_Y;
    char left[24], right[24];
    long pos = 0, len = id3 ? (long)id3->length : 0;
    if (len > 0)
        pos = MAX(0, MIN(elapsed, len));
    format_mmss(left, sizeof(left), pos, false);
    format_mmss(right, sizeof(right), len - pos, true);

    int lw = ui_text_width(font, "00:00"), rw = ui_text_width(font, "-00:00");
    int bx = UI_MARGIN + lw + UI_GAP;
    int bw = vp->width - UI_MARGIN - rw - UI_GAP - bx;
    int by = NPL_BAND_Y + 1 + (band_h - 1 - NPL_BAR_H) / 2;

    ui_fill_rect(0, NPL_BAND_Y, vp->width, band_h, UI_COL_BG);
    ui_fill_rect(0, NPL_BAND_Y, vp->width, 1, ui_col(UI_C_BAR_LINE));

    ui_text_vcenter(UI_MARGIN, by, NPL_BAR_H, font, UI_COL_TEXT, left, lw + 4, -1);
    ui_text_vcenter(vp->width - UI_MARGIN - rw - 4, by, NPL_BAR_H, font,
                    UI_COL_TEXT, right, rw + 4, 1);

    if (scrubbing)
        draw_scrub_bar(bx, by, bw, pos, len);
    else
        draw_bar(bx, by, bw, pos, len);
}

/* the track, one line each: title, artist, album, position, the file */
static void draw_info(struct viewport *vp, const struct mp3entry *id3,
                      char *buf, size_t len)
{
    const int row = ui_font(UI_FONT_ROW);
    const int w = vp->width - 2 * UI_MARGIN;
    char line[64];
    int y = 6;

    ui_text(UI_MARGIN, y, row, UI_COL_TEXT, ui_track_title(id3, buf, len), w, -1);
    y += NP_LINE_H;

    const char *artist = id3->artist && *id3->artist ? id3->artist :
                         id3->albumartist && *id3->albumartist ?
                             id3->albumartist : NULL;
    if (artist)
    {
        ui_text(UI_MARGIN, y, row, UI_COL_TEXT2, artist, w, -1);
        y += NP_LINE_H;
    }
    if (id3->album && *id3->album)
    {
        ui_text(UI_MARGIN, y, row, UI_COL_TEXT2, id3->album, w, -1);
        y += NP_LINE_H;
    }

    const char *mode = global_settings.playlist_shuffle ? str(LANG_SHUFFLE) :
                       global_settings.repeat_mode == REPEAT_ONE ? str(LANG_REPEAT_ONE) :
                       global_settings.repeat_mode == REPEAT_ALL ? str(LANG_ALL) : NULL;
    snprintf(line, sizeof line, "%d of %d%s%s",
             playlist_get_display_index(), playlist_amount(),
             mode ? " \xc2\xb7 " : "", mode ? mode : "");
    ui_text(UI_MARGIN, y, row, UI_COL_TEXT2, line, w, -1);
    y += NP_LINE_H;

    ui_file_line(id3, line, sizeof line);
    if (line[0])
        ui_text(UI_MARGIN, y, row, UI_COL_MUTED, line, w, -1);
}


void ui_np_set_scrub(bool on)
{
    scrubbing = on;
    ui_np_draw(false);
}

void ui_np_draw(bool full)
{
    struct wps_state *state = get_wps_state();
    struct mp3entry *id3 = state->id3;
    struct viewport vp;
    viewport_set_defaults(&vp, SCREEN_MAIN);
    struct viewport *last = lcd_set_viewport(&vp);

    long elapsed = id3 ? (long)id3->elapsed + state->ff_rewind_count : 0;
    bool overlay = volume_shown && TIME_BEFORE(current_tick, volume_until);
    if (volume_shown && !overlay)
    {
        volume_shown = false; /* back to the progress bar */
    }

    if (!full)
    {
        if (overlay)
            draw_volume_band(&vp);
        else
            draw_band(&vp, id3, elapsed);
        lcd_update_viewport_rect(0, NPL_BAND_Y, vp.width, vp.height - NPL_BAND_Y);
        lcd_set_viewport(last);
        return;
    }

    ui_status_title(str(LANG_NOW_PLAYING));
    ui_fill_rect(0, 0, vp.width, vp.height, UI_COL_BG);
    if (id3)
    {
        char buf[MAX_PATH];
        draw_info(&vp, id3, buf, sizeof(buf));
    }
    if (overlay)
        draw_volume_band(&vp);
    else
        draw_band(&vp, id3, elapsed);
    /* slide in if a screen change asked for it */
    if (!ui_transition_run(vp.x, vp.y, vp.width, vp.height))
        lcd_update_viewport();
    lcd_set_viewport(last);
}

void ui_np_volume_changed(void)
{
    volume_until = current_tick + OVERLAY_TICKS;
    volume_shown = true;
    ui_np_draw(false); /* only the band changes */
}
