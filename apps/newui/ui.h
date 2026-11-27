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

/* The new code-drawn UI (see docs/newui/PLAN.md). */

#ifndef NEWUI_UI_H
#define NEWUI_UI_H

#include <stdbool.h>
#include <stdint.h>
#include "lcd.h"
#include "metadata.h"   /* struct mp3entry, the codec names */

/* --- design tokens (PLAN.md §2) ---
 * Sized for Tiny5 Mono: 9 px tall, 7 px ascent, 5 px cap height. PLAN.md
 * §2 was written for the 20 px Noto faces, so the rows are tighter now. */

#define UI_STATUS_H     22  /* status bar height */
#define UI_MARGIN       12  /* side margin */
#define UI_GAP           8
#define UI_ROW_H        16  /* text row: a 9 px glyph + 7 px of air */
#define UI_LINE_H       12  /* one line of text, console screens */
#define UI_ART_ROW_H    48  /* row with a cover: UI_THUMB fills the row */
#define UI_LINE2_ROW_H  24  /* name + grey second line: two lines + 6 px */

/* colours follow the theme (Dark, Light, Nord or Gotham, ui_style.c) */
enum ui_col {
    UI_C_BG, UI_C_TEXT, UI_C_TEXT2, UI_C_MUTED, UI_C_SEL, UI_C_SEL_TEXT,
    UI_C_SEL_TEXT2, UI_C_TRACK, UI_C_PANEL, UI_C_ARTIST, UI_C_ALBUM, UI_C_FILL,
    UI_C_BAR_LINE, UI_C_PH_BG, UI_C_PH_FG,
    UI_C_OUTLINE, UI_C_EDGE,
    UI_C_ON_ACCENT, UI_C_ON_ACCENT2, UI_C_STATUS_FG,
    UI_C_LETTER_TOP, UI_C_LETTER_BOTTOM, UI_C_LETTER_FG, UI_C_COUNT
};
unsigned ui_col(enum ui_col c);
#define UI_COL_BG       ui_col(UI_C_BG)
#define UI_COL_TEXT     ui_col(UI_C_TEXT)
#define UI_COL_TEXT2    ui_col(UI_C_TEXT2)     /* secondary */
#define UI_COL_MUTED    ui_col(UI_C_MUTED)
#define UI_COL_SEL      ui_col(UI_C_SEL)       /* Minimal selection */
#define UI_COL_SEL_TEXT ui_col(UI_C_SEL_TEXT)
#define UI_COL_SEL_TEXT2 ui_col(UI_C_SEL_TEXT2) /* secondary text on it */
#define UI_COL_TRACK    ui_col(UI_C_TRACK)     /* recessed surfaces */
#define UI_COL_FILL     ui_col(UI_C_FILL)      /* progress, sliders */
#define UI_COL_ARTIST   ui_col(UI_C_ARTIST)    /* Now Playing */
#define UI_COL_ALBUM    ui_col(UI_C_ALBUM)
#define UI_COL_BAR_LINE ui_col(UI_C_BAR_LINE)  /* the track a bar fills */
#define UI_COL_PH_BG    ui_col(UI_C_PH_BG)     /* the no-cover placeholder */
#define UI_COL_PH_FG    ui_col(UI_C_PH_FG)
#define UI_COL_EDGE     ui_col(UI_C_EDGE)      /* a cover seen edge-on */
#define UI_COL_ON_ACCENT  ui_col(UI_C_ON_ACCENT) /* text on accent bars */
#define UI_COL_ON_ACCENT2 ui_col(UI_C_ON_ACCENT2)
#define UI_COL_STATUS   ui_col(UI_C_STATUS_FG) /* the status bar's text and icons */
#define UI_COL_ACCENT   ui_accent()  /* the theme's accent colour */
#define UI_COL_PANEL    ui_col(UI_C_PANEL)     /* pop-up panels */

/* --- the chosen look (ui_style.c) --- */

/* The eight palettes: Dark (black), Light (white, as the iPod), Nord (deep
 * polar-night blue with warm grey text), Gotham (navy with teal text), Rose
 * Pine (plum purple with pale rose text), Tokyo Night (midnight blue with pale
 * periwinkle text), One Light and GitHub Light (dark text on off-white). Each
 * carries its own accent; there is no separate accent setting. */
enum ui_theme {
    UI_THEME_DARK,
    UI_THEME_LIGHT,
    UI_THEME_NORD,
    UI_THEME_GOTHAM,
    UI_THEME_ROSE_PINE,
    UI_THEME_TOKYO_NIGHT,
    UI_THEME_ONE_LIGHT,
    UI_THEME_GITHUB_LIGHT,
    UI_THEME_COUNT
};

unsigned ui_accent(void);
/* the accent where it is a fill or an icon (progress, plots, status icons):
 * the dark themes' deep selection blue lightens to its band's top there */
unsigned ui_accent_highlight(void);
/* the theme's warning red (a low battery) */
unsigned ui_red(void);
/* The chosen theme, 0..UI_THEME_COUNT-1 */
int ui_theme(void);
/* A light-background theme: Light, One Light or GitHub Light */
bool ui_light(void);
/* Minimal style: the selection is the theme's own band, else an accent bar */
bool ui_minimal(void);
/* Draws the selection behind a row at x, y, w, h */
void ui_selection(int x, int y, int w, int h);
/* the selection's colour, and the text on it (main, secondary) */
unsigned ui_sel_bg(void);
/* the selection bar's top colour (Light: lighter, for its gradient) */
unsigned ui_sel_top(void);
unsigned ui_sel_text(void);
unsigned ui_sel_text2(void);
/* text to draw on any band of colour `band` (a bright band takes the theme's
 * background, the rest take the theme's on-accent colour) */
unsigned ui_text_on(unsigned band);
unsigned ui_text_on2(unsigned band);

/* --- transitions (ui_transition.c) --- */

/* Before a list changes level: keeps the screen to slide from, dir +1
 * (enter, from the right) or -1 (back, from the left) */
void ui_transition(int dir);
/* Called with the area a list just drew: slides to it if a transition is
 * pending and the area is the whole screen below the status bar. True if
 * it did (and updated the display). */
bool ui_transition_run(int x, int y, int w, int h);

/* --- fonts (ui_font.c) --- */

enum ui_font {
    /* All five are Tiny5 Mono (9 px) for now; the slots stay separate so
     * the sizes can diverge again without touching the callers. */
    UI_FONT_SMALL,  /* times, counts, status bar */
    UI_FONT_BODY,   /* secondary text (artist, album, setting values) */
    UI_FONT_ROW,    /* list rows (also the default UI font) */
    UI_FONT_TITLE,  /* titles */
    UI_FONT_BIG,    /* fast-scroll letter, lock screen clock */
    UI_FONT_COUNT
};

/* Loads the fonts once, at the end of boot; safe to call again. Missing
 * fonts fall back to the system font, as does ui_font() before this. */
void ui_font_init(void);
/* Re-pins FONT_UI to the currently selected font family (call after the
 * newui_font setting changes) */
void ui_font_apply(void);
/* Rockbox font id for one of the UI fonts */
int ui_font(enum ui_font f);
/* FONT_UI as the theme's actual UI font id; other ids unchanged */
int ui_resolve_font(int font);

/* --- drawing (ui_draw.c); coordinates relative to the current viewport --- */

void ui_blend_pixel(int x, int y, unsigned color, int alpha); /* alpha 0..256 */
/* `color` at alpha 0..256 over `bg`, both RGB565, as a colour rather than a
 * screen pixel */
unsigned ui_mix(unsigned bg, unsigned color, int alpha);
void ui_fill_rect(int x, int y, int w, int h, unsigned color);
void ui_fill_round_rect(int x, int y, int w, int h, int r, unsigned color);
/* filled with a vertical gradient from top to bottom */
void ui_gradient_round_rect(int x, int y, int w, int h, int r, unsigned top,
                            unsigned bottom);
void ui_stroke_round_rect(int x, int y, int w, int h, int r, int stroke,
                          unsigned color);
/* --- the playing track as text --- */
#define UI_SEP " \xc2\xb7 "  /* between the parts of a readout line */
/* The file line: the codec, then bitrate and sample rate when the codec
 * reports them ("FLAC <sep> 921 kbps <sep> 44.1 kHz"). Returns the length
 * written, 0 when nothing is known. */
int ui_file_line(const struct mp3entry *id3, char *buf, int size);
/* The track's name: the title tag, or the file name without its extension
 * when the file has no tags. Copies into buf when the fallback is needed. */
const char *ui_track_title(const struct mp3entry *id3, char *buf, int size);

/* --- icons: glyphs of the UI font, not bitmaps ---
 * Tiny5 Mono carries transport and status symbols. One glyph per icon, in
 * one UI_CELL_W character cell. */
#define UI_CELL_W  6      /* Tiny5 Mono advances 6 px per character */
#define UI_G_PLAY  "\xe2\x8f\xb5"  /* ⏵ U+23F5 */
#define UI_G_PAUSE "\xe2\x8f\xb8"  /* ⏸ U+23F8 */
#define UI_G_STOP  "\xe2\x8f\xb9"  /* ⏹ U+23F9 */
#define UI_G_FF    "\xe2\x8f\xa9"  /* ⏩ U+23E9 */
#define UI_G_FB    "\xe2\x8f\xaa"  /* ⏪ U+23EA */
#define UI_G_REC   "\xe2\x8f\xba"  /* ⏺ U+23FA */
#define UI_G_LOCK  "\xf0\x9f\x94\x92" /* 🔒 U+1F512 */
#define UI_G_BT    "BT"            /* Unicode has no Bluetooth rune */
#define UI_G_DRILL ">"            /* a row that opens something */
/* room a drill glyph takes at the right end of a list row */
#define UI_DRILL_W (UI_CELL_W + 4)
void ui_drill(int x_right, int y, int h, unsigned color);
/* Rounds the corners of what is drawn at x,y,w,h, painting outside them in bg */
void ui_round_corners(int x, int y, int w, int h, int r, unsigned bg);
int ui_text_width(int font, const char *text);
/* Transparent text, shortened with an ellipsis to max_w (0: no limit);
 * align -1 left, 0 centre, 1 right within max_w. Returns the drawn width. */
int ui_text(int x, int y, int font, unsigned color, const char *text,
            int max_w, int align);
/* The y to draw text of font at so its capitals are centred in the band
 * [y, y + h) */
int ui_text_y(int font, int y, int h);
/* Same, vertically centred in the band [y, y + h) */
int ui_text_vcenter(int x, int y, int h, int font, unsigned color,
                    const char *text, int max_w, int align);

/* --- readout lines of the playing track --- */
#define UI_SEP " \xc2\xb7 "  /* between the parts of a readout line */
/* "FLAC \xc2\xb7 921 kbps \xc2\xb7 44.1 kHz": the codec, then bitrate and
 * sample rate when the codec reports them. Returns the length written, 0
 * when nothing is known. */
int ui_file_line(const struct mp3entry *id3, char *buf, int size);

/* --- pop-ups (ui_popup.c) --- */

#define UI_POPUP_RADIUS   12
#define UI_POPUP_PAD      16  /* panel's inner margin */
#define UI_POPUP_LINES     8
#define UI_POPUP_LINE_LEN 96
struct ui_lines {
    int count;
    int widest;     /* pixels */
    char line[UI_POPUP_LINES][UI_POPUP_LINE_LEN];
};
/* The pop-up panel's rounded rectangle */
void ui_panel(int x, int y, int w, int h);
/* Darkens what is in the current viewport, behind a pop-up */
void ui_dim(void);
/* Appends text to l, word wrapped to max_w; '\n' and '\t' break lines */
void ui_wrap(struct ui_lines *l, const char *text, int font, int max_w);
/* Most choices a choice panel shows; more go in a full-screen list */
#define UI_CHOICE_MAX 6
/* A panel with a title and up to UI_CHOICE_MAX choices, sel highlighted
 * (the yes/no panel's look) */
void ui_choice_draw(struct viewport *vp, const char *title, int count, int sel,
                    const char *(*label)(int i, void *data, char *buf, size_t len),
                    void *data);

/* --- album art (ui_art.c) --- */

#define UI_ART_MAX 240 /* largest cover size decoded (Home, Light) */

struct bitmap;
/* The playing track's cover scaled to fit size x size (aspect kept), or
 * NULL when nothing plays or it has no art. Cached until the track or the
 * size changes. */
const struct bitmap *ui_art_current(int size);
/* Draws a cover centred in the size x size square, corners rounded */
void ui_art_draw(const struct bitmap *bm, int x, int y, int size, int radius);
/* Draws a cover turned a little (iPod Now Playing): w wide on screen, its
 * left edge hl tall and right edge hr, centred on the taller one from y,
 * with a reflection refl_h tall below it starting at refl_a/256 */
void ui_art_draw_tilted(const struct bitmap *bm, int x, int y, int w,
                        int hl, int hr, int refl_h, int refl_a);
/* The no-art placeholder: a rounded square with the Rockbox clef */
void ui_art_placeholder(int x, int y, int size, int radius);
struct mp3entry;
/* Decodes id3's cover into out (size x size, centred on black); invalidates
 * the cached current art. False if there is none. */
bool ui_art_decode_to(const struct mp3entry *id3, int size, fb_data *out);

/* --- battery usage (scr_battery.c) --- */

/* Starts logging the battery level against running time, a point every 5% */
void ui_battery_log_init(void);
/* System > Battery Usage: the log as a chart. Returns 1 for USB. */
int ui_battery_usage(void);

/* --- system status (scr_stats.c) --- */

/* System > System Status: CPU, memory, storage and IO. Returns 1 for USB. */
int ui_stats(void);

/* --- thumbnails (ui_thumb.c) --- */

#define UI_THUMB UI_ART_ROW_H /* cover size in list rows: the whole row */
/* The cover of the album of track_path at UI_THUMB size, or NULL. Cached in
 * RAM and in /.rockbox/newui/thumbs, so each album is decoded once. */
const fb_data *ui_thumb_get(const char *track_path);

#define UI_COVER 150 /* Cover Flow's cover size */
/* The UI_COVER cover from RAM only: NULL with *known false if it is not
 * loaded, else the cover or NULL for none */
const fb_data *ui_cover_peek(const char *track_path, bool *known);
/* Loads the cover into RAM if it was saved on the card (fast); false if
 * it still has to be built */
bool ui_cover_load_saved(const char *track_path);
/* True if the cover is saved on the card */
bool ui_cover_saved(const char *track_path);
/* Decodes the cover (slow) and saves it, and its list thumbnail, on the
 * card, without touching the covers in RAM */
void ui_cover_build(const char *track_path);

/* --- the marks from the logo (ui_mark.c) --- */

/* The Rockbox clef, from the logo, centred in the size x size square at x,y
 * and blended over what is already drawn. */
void ui_clef_draw(int x, int y, int size, unsigned color);
/* The same into a size x size native-pixel buffer, over bg */
void ui_clef_fill(fb_data *dst, int size, unsigned bg, unsigned color);
/* The Rockbox icon, size px across, its top-left corner at x,y, painted over
 * what is already there at alpha 0..256: the boot splash, faded in */
void ui_logo_draw(int x, int y, int size, int alpha);

/* --- screens --- */

/* Home (scr_home.c). Returns the root menu screen to open (GO_TO_*),
 * or MENU_ATTACHED_USB. *selected keeps the row between visits. */
int ui_home(int *selected);

/* Cover Flow (scr_coverflow.c). Returns GO_TO_WPS when an album was
 * started, MENU_ATTACHED_USB, or GO_TO_ROOT for Back. */
int ui_coverflow(void);

/* The lock screen (scr_lock.c), shown by action.c while the keys are
 * locked. Returns 0 once unlocked, or a system event (SYS_*) that ended it
 * with the keys still locked. */
int ui_lock_screen(void);

/* Now Playing's drawing (scr_np.c), called by apps/gui/wps.c: full redraw,
 * or just the progress bar and times */
void ui_np_draw(bool full);
/* Shows the volume overlay for a moment */
void ui_np_volume_changed(void);
/* The progress bar as the scrub bar (white, thicker) or back */
void ui_np_set_scrub(bool on);

/* --- status bar (ui_status.c) --- */

struct tm;
struct ui_status {
    int playmode;          /* enum playmode (status.h) */
    bool keylock;
    const struct tm *time; /* NULL: no clock */
    bool clock_24h;
    const char *title;     /* the screen's name, left; NULL none */
    int battery;           /* percent, -1 unknown */
    bool battery_visible;  /* false while blinking off (low battery) */
    bool charging;
    int bluetooth;         /* 0 off, 1 on but not connected, 2 connected */
};
/* Draws the bar into the current viewport (UI_STATUS_H high) */
void ui_status_draw(const struct ui_status *st);
/* As the iPod: the bar shows the screen's name on the left (instead of the
 * clock); screens set it as they draw. "" or NULL: none (the clock) */
void ui_status_title(const char *title);
const char *ui_status_get_title(void);
/* The bar's width (Home: only over its menu); 0 the whole screen */
void ui_status_width(int width);
int ui_status_get_width(void);

/* --- wheel (ui_wheel.c) --- */

/* Rows to move for one step in direction dir (+1/-1) of a list of nb_items,
 * growing while the wheel spins fast */
int ui_wheel_step(int dir, int nb_items);
/* The fast-scroll letter should be on screen */
bool ui_wheel_letter_visible(void);
/* True once after the letter has been drawn and its time ran out */
bool ui_wheel_letter_expired(void);
/* Draws the letter for 'title' centred in the current viewport */
void ui_wheel_draw_letter(const char *title);

/* --- time (ui_time.c) --- */

/* Free-running microsecond counter for measuring frame times */
uint32_t ui_time_us(void);

/* --- gallery (ui_gallery.c) --- */

/* Debug menu: shows the UI's building blocks and measures drawing speed.
 * Returns true if USB was connected. */
bool ui_gallery(void);

#endif /* NEWUI_UI_H */
