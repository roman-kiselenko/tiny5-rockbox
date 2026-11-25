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

/* The look the user chose (Settings > General > Display): the theme's
 * colours (Dark, Light like the iPod, Nord, Gotham, Rose Pine, Tokyo Night,
 * One Light or GitHub Light; the layout is the same) and the selection colour
 * (the theme's accent in Classic, the theme's own band in Minimal). The
 * selector's shape is the same everywhere: a flat band. */

#include "config.h"
#include "settings.h"
#include "ui.h"

#define RGB LCD_RGBPACK

/* The accent is not a choice: each theme has one, taken from its own
 * palette. Dark and Light use graphite; the deep palettes use their
 * selection-background, so the default selection is that band with its
 * foreground text. */
static const unsigned theme_accents[UI_THEME_COUNT] = {
    [UI_THEME_DARK]        = RGB(0x6e, 0x6e, 0x6e),  /* graphite */
    [UI_THEME_LIGHT]       = RGB(0x6e, 0x6e, 0x6e),  /* graphite */
    [UI_THEME_NORD]        = RGB(0x1b, 0x3a, 0x5b),  /* Nord's selection blue */
    [UI_THEME_GOTHAM]      = RGB(0x0a, 0x37, 0x49),  /* Gotham's selection blue */
    [UI_THEME_ROSE_PINE]   = RGB(0x2f, 0x2c, 0x40),  /* Rose Pine's selection */
    [UI_THEME_TOKYO_NIGHT] = RGB(0x28, 0x34, 0x57),  /* Tokyo Night's selection */
    [UI_THEME_ONE_LIGHT]   = RGB(0x2a, 0x2b, 0x33),  /* One Light's ink */
    [UI_THEME_GITHUB_LIGHT]= RGB(0x26, 0x4f, 0x78),  /* GitHub's selection blue */
};

/* Where the accent is a fill or an icon rather than a band (progress bars,
 * the CPU and battery plots, the status bar's play glyph): the deep selection
 * colours of the dark palettes are too close to the dark tracks they sit on,
 * so there the accent's lighter end is used - the same one ui_sel_top() gives
 * the band's gradient. One Light's ink already stands out on a light track. */
static const unsigned theme_accent_hi[UI_THEME_COUNT] = {
    [UI_THEME_DARK]        = RGB(0x6e, 0x6e, 0x6e),
    [UI_THEME_LIGHT]       = RGB(0x6e, 0x6e, 0x6e),
    [UI_THEME_NORD]        = RGB(0x76, 0x88, 0x9c),
    [UI_THEME_GOTHAM]      = RGB(0x6c, 0x87, 0x91),
    [UI_THEME_ROSE_PINE]   = RGB(0x82, 0x80, 0x8c),
    [UI_THEME_TOKYO_NIGHT] = RGB(0x7e, 0x85, 0x9a),
    [UI_THEME_ONE_LIGHT]   = RGB(0x2a, 0x2b, 0x33),
    [UI_THEME_GITHUB_LIGHT]= RGB(0x26, 0x4f, 0x78),
};

/* Each palette's own warning red, used for a low battery */
static const unsigned theme_red[UI_THEME_COUNT] = {
    [UI_THEME_DARK]        = RGB(0xf4, 0x62, 0x4c),
    [UI_THEME_LIGHT]       = RGB(0xf4, 0x62, 0x4c),
    [UI_THEME_NORD]        = RGB(0xea, 0x6c, 0x73),
    [UI_THEME_GOTHAM]      = RGB(0xc2, 0x31, 0x27),  /* palette red */
    [UI_THEME_ROSE_PINE]   = RGB(0xeb, 0x6f, 0x92),  /* Rose Pine's rose */
    [UI_THEME_TOKYO_NIGHT] = RGB(0xf7, 0x76, 0x8e),  /* Tokyo Night's red */
    [UI_THEME_ONE_LIGHT]   = RGB(0xde, 0x3d, 0x35),  /* One Light's red */
    [UI_THEME_GITHUB_LIGHT]= RGB(0xcf, 0x22, 0x2e),  /* GitHub's red */
};

static const unsigned themes[UI_THEME_COUNT][UI_C_COUNT] = {
    [UI_THEME_DARK] = {
        [UI_C_BG] = RGB(0x00, 0x00, 0x00),       [UI_C_TEXT] = RGB(0xff, 0xff, 0xff),
        [UI_C_TEXT2] = RGB(0xaa, 0xaa, 0xaa),    [UI_C_MUTED] = RGB(0x8c, 0x8c, 0x8c),
        [UI_C_SEL] = RGB(0xee, 0xee, 0xec),      [UI_C_SEL_TEXT] = RGB(0x17, 0x17, 0x17),
        [UI_C_SEL_TEXT2] = RGB(0x48, 0x48, 0x48),
        [UI_C_TRACK] = RGB(0x1c, 0x1c, 0x1c),    [UI_C_PANEL] = RGB(0x26, 0x26, 0x26),
        [UI_C_ARTIST] = RGB(0xcc, 0xcc, 0xcc),   [UI_C_ALBUM] = RGB(0xbb, 0xbb, 0xbb),
        [UI_C_FILL] = RGB(0xee, 0xee, 0xec),
        [UI_C_BAR_LINE] = RGB(0x44, 0x44, 0x48),
        [UI_C_OUTLINE] = RGB(0x5a, 0x5a, 0x5e),
        [UI_C_EDGE] = RGB(0xb4, 0xb4, 0xb8),
        [UI_C_STATUS_FG] = RGB(0xe4, 0xe4, 0xe6),   /* soft white */
        [UI_C_ON_ACCENT] = RGB(0xff, 0xff, 0xff), [UI_C_ON_ACCENT2] = RGB(0xe8, 0xee, 0xf8),
        [UI_C_PH_BG] = RGB(0x1c, 0x1c, 0x1c),    [UI_C_PH_FG] = RGB(0x8c, 0x8c, 0x8c),
        /* the fast-scroll letter: light grey on black */
        [UI_C_LETTER_TOP] = RGB(0xe8, 0xe8, 0xea), [UI_C_LETTER_BOTTOM] = RGB(0xb8, 0xb8, 0xbc),
        [UI_C_LETTER_FG] = RGB(0x14, 0x14, 0x14),
    },
    [UI_THEME_LIGHT] = {   /* Light: white page, dark text */
        [UI_C_BG] = RGB(0xff, 0xff, 0xff),       [UI_C_TEXT] = RGB(0x14, 0x14, 0x14),
        [UI_C_TEXT2] = RGB(0x5e, 0x5e, 0x5e),    [UI_C_MUTED] = RGB(0x8c, 0x8c, 0x8c),
        [UI_C_SEL] = RGB(0xd6, 0xd6, 0xd6),      [UI_C_SEL_TEXT] = RGB(0x14, 0x14, 0x14),
        [UI_C_SEL_TEXT2] = RGB(0x48, 0x48, 0x48),
        [UI_C_TRACK] = RGB(0xdc, 0xdc, 0xdc),    [UI_C_PANEL] = RGB(0xf2, 0xf2, 0xf2),
        [UI_C_ARTIST] = RGB(0x33, 0x33, 0x33),   [UI_C_ALBUM] = RGB(0x4a, 0x4a, 0x4a),
        [UI_C_FILL] = RGB(0x6e, 0x6e, 0x6e),     /* the accent: graphite */
        [UI_C_BAR_LINE] = RGB(0x8c, 0x8c, 0x90),
        [UI_C_OUTLINE] = RGB(0x8c, 0x8c, 0x90),
        [UI_C_EDGE] = RGB(0x70, 0x70, 0x74),
        [UI_C_STATUS_FG] = RGB(0x2e, 0x2e, 0x32),   /* soft black */
        [UI_C_ON_ACCENT] = RGB(0xff, 0xff, 0xff), [UI_C_ON_ACCENT2] = RGB(0xe8, 0xee, 0xf8),
        /* dark enough that a missing cover stands out on white */
        [UI_C_PH_BG] = RGB(0x5c, 0x5c, 0x62),    [UI_C_PH_FG] = RGB(0xd4, 0xd4, 0xd8),
        /* the fast-scroll letter: dark grey on white, as on the iPod */
        [UI_C_LETTER_TOP] = RGB(0x62, 0x62, 0x64), [UI_C_LETTER_BOTTOM] = RGB(0x3a, 0x3a, 0x3c),
        [UI_C_LETTER_FG] = RGB(0xff, 0xff, 0xff),
    },
    [UI_THEME_NORD] = {    /* Nord: polar-night blue, warm grey text */
        [UI_C_BG] = RGB(0x0b, 0x0e, 0x14),       [UI_C_TEXT] = RGB(0xbf, 0xbd, 0xb6),
        [UI_C_TEXT2] = RGB(0x9b, 0x9b, 0x93),    [UI_C_MUTED] = RGB(0x68, 0x68, 0x68),
        [UI_C_SEL] = RGB(0x1e, 0x23, 0x2b),      [UI_C_SEL_TEXT] = RGB(0xbf, 0xbd, 0xb6),
        [UI_C_SEL_TEXT2] = RGB(0x9b, 0x9b, 0x93),
        [UI_C_TRACK] = RGB(0x14, 0x19, 0x20),    [UI_C_PANEL] = RGB(0x1e, 0x23, 0x2b),
        [UI_C_ARTIST] = RGB(0xc7, 0xc7, 0xc7),   [UI_C_ALBUM] = RGB(0x8f, 0x8e, 0x86),
        [UI_C_FILL] = RGB(0xbf, 0xbd, 0xb6),
        [UI_C_BAR_LINE] = RGB(0x39, 0x42, 0x4d),
        [UI_C_OUTLINE] = RGB(0x33, 0x3c, 0x45),
        [UI_C_EDGE] = RGB(0xc7, 0xc7, 0xc7),
        [UI_C_STATUS_FG] = RGB(0xc7, 0xc7, 0xc7),
        /* text on a dark accent band is the palette's foreground; a bright
           accent gets the dark cursor-text instead, see band_is_light() */
        [UI_C_ON_ACCENT] = RGB(0xbf, 0xbd, 0xb6), [UI_C_ON_ACCENT2] = RGB(0x9b, 0x9b, 0x93),
        [UI_C_PH_BG] = RGB(0x1e, 0x23, 0x2b),    [UI_C_PH_FG] = RGB(0x68, 0x68, 0x68),
        /* the fast-scroll letter: the palette's cursor, dark on light */
        [UI_C_LETTER_TOP] = RGB(0xc7, 0xc7, 0xc7), [UI_C_LETTER_BOTTOM] = RGB(0x8f, 0x8e, 0x86),
        [UI_C_LETTER_FG] = RGB(0x0b, 0x0e, 0x14),
    },
    [UI_THEME_GOTHAM] = {  /* Gotham: navy, teal text, blue-grey surfaces */
        [UI_C_BG] = RGB(0x0c, 0x10, 0x14),       [UI_C_TEXT] = RGB(0x99, 0xd1, 0xce),
        [UI_C_TEXT2] = RGB(0x6f, 0xa6, 0xa3),    [UI_C_MUTED] = RGB(0x4e, 0x51, 0x66),
        [UI_C_SEL] = RGB(0x19, 0x54, 0x66),      [UI_C_SEL_TEXT] = RGB(0x99, 0xd1, 0xce),
        [UI_C_SEL_TEXT2] = RGB(0x6f, 0xa6, 0xa3),
        [UI_C_TRACK] = RGB(0x14, 0x18, 0x1f),    [UI_C_PANEL] = RGB(0x14, 0x18, 0x1f),
        [UI_C_ARTIST] = RGB(0x33, 0x85, 0x9e),   [UI_C_ALBUM] = RGB(0x4e, 0x51, 0x66),
        [UI_C_FILL] = RGB(0x99, 0xd1, 0xce),
        [UI_C_BAR_LINE] = RGB(0x2b, 0x38, 0x41),
        [UI_C_OUTLINE] = RGB(0x4e, 0x51, 0x66),
        [UI_C_EDGE] = RGB(0x99, 0xd1, 0xce),
        [UI_C_STATUS_FG] = RGB(0x99, 0xd1, 0xce),
        /* the palette's foreground on its selection-background; a bright band
           would take the dark cursor-text instead, see band_is_light() */
        [UI_C_ON_ACCENT] = RGB(0x99, 0xd1, 0xce), [UI_C_ON_ACCENT2] = RGB(0x6f, 0xa6, 0xa3),
        [UI_C_PH_BG] = RGB(0x14, 0x18, 0x1f),    [UI_C_PH_FG] = RGB(0x4e, 0x51, 0x66),
        /* the fast-scroll letter: the palette's cursor, dark on light */
        [UI_C_LETTER_TOP] = RGB(0x99, 0xd1, 0xce), [UI_C_LETTER_BOTTOM] = RGB(0x33, 0x85, 0x9e),
        [UI_C_LETTER_FG] = RGB(0x11, 0x15, 0x1c),
    },
    [UI_THEME_ROSE_PINE] = {  /* Rose Pine: plum purple, pale rose text */
        [UI_C_BG] = RGB(0x1f, 0x1d, 0x2e),       [UI_C_TEXT] = RGB(0xe0, 0xde, 0xf4),
        [UI_C_TEXT2] = RGB(0xb0, 0xad, 0xc0),    [UI_C_MUTED] = RGB(0x90, 0x8c, 0xaa),
        [UI_C_SEL] = RGB(0x26, 0x23, 0x3a),      [UI_C_SEL_TEXT] = RGB(0xe0, 0xde, 0xf4),
        [UI_C_SEL_TEXT2] = RGB(0xb0, 0xad, 0xc0),
        [UI_C_TRACK] = RGB(0x2a, 0x27, 0x40),    [UI_C_PANEL] = RGB(0x26, 0x23, 0x3a),
        [UI_C_ARTIST] = RGB(0xeb, 0xbc, 0xba),   [UI_C_ALBUM] = RGB(0x90, 0x8c, 0xaa),
        [UI_C_FILL] = RGB(0xe0, 0xde, 0xf4),
        [UI_C_BAR_LINE] = RGB(0x45, 0x3f, 0x5c),
        [UI_C_OUTLINE] = RGB(0x4a, 0x46, 0x61),
        [UI_C_EDGE] = RGB(0xe0, 0xde, 0xf4),
        [UI_C_STATUS_FG] = RGB(0xe0, 0xde, 0xf4),
        /* the palette's foreground on its selection-background; a bright band
           would take the dark cursor-text instead, see band_is_light() */
        [UI_C_ON_ACCENT] = RGB(0xe0, 0xde, 0xf4), [UI_C_ON_ACCENT2] = RGB(0xb0, 0xad, 0xc0),
        [UI_C_PH_BG] = RGB(0x26, 0x23, 0x3a),    [UI_C_PH_FG] = RGB(0x90, 0x8c, 0xaa),
        /* the fast-scroll letter: the palette's cursor, dark on light, plum at
           the bottom */
        [UI_C_LETTER_TOP] = RGB(0xe0, 0xde, 0xf4), [UI_C_LETTER_BOTTOM] = RGB(0xc4, 0xa7, 0xe7),
        [UI_C_LETTER_FG] = RGB(0x1f, 0x1d, 0x2e),
    },
    [UI_THEME_TOKYO_NIGHT] = {  /* Tokyo Night: midnight blue, periwinkle text */
        [UI_C_BG] = RGB(0x1a, 0x1b, 0x26),       [UI_C_TEXT] = RGB(0xc0, 0xca, 0xf5),
        [UI_C_TEXT2] = RGB(0xa9, 0xb1, 0xd6),    [UI_C_MUTED] = RGB(0x41, 0x48, 0x68),
        [UI_C_SEL] = RGB(0x41, 0x48, 0x68),      [UI_C_SEL_TEXT] = RGB(0xc0, 0xca, 0xf5),
        [UI_C_SEL_TEXT2] = RGB(0xa9, 0xb1, 0xd6),
        [UI_C_TRACK] = RGB(0x23, 0x27, 0x33),    [UI_C_PANEL] = RGB(0x15, 0x16, 0x1e),
        [UI_C_ARTIST] = RGB(0x7a, 0xa2, 0xf7),   [UI_C_ALBUM] = RGB(0xa9, 0xb1, 0xd6),
        [UI_C_FILL] = RGB(0xc0, 0xca, 0xf5),
        [UI_C_BAR_LINE] = RGB(0x41, 0x48, 0x68),
        [UI_C_OUTLINE] = RGB(0x41, 0x48, 0x68),
        [UI_C_EDGE] = RGB(0xc0, 0xca, 0xf5),
        [UI_C_STATUS_FG] = RGB(0xc0, 0xca, 0xf5),
        /* the palette's foreground on its selection-background; a bright band
           would take the dark cursor-text instead, see band_is_light() */
        [UI_C_ON_ACCENT] = RGB(0xc0, 0xca, 0xf5), [UI_C_ON_ACCENT2] = RGB(0xa9, 0xb1, 0xd6),
        [UI_C_PH_BG] = RGB(0x15, 0x16, 0x1e),    [UI_C_PH_FG] = RGB(0x41, 0x48, 0x68),
        /* the fast-scroll letter: the palette's cursor, dark on light, blue at
           the bottom */
        [UI_C_LETTER_TOP] = RGB(0xc0, 0xca, 0xf5), [UI_C_LETTER_BOTTOM] = RGB(0x7a, 0xa2, 0xf7),
        [UI_C_LETTER_FG] = RGB(0x1a, 0x1b, 0x26),
    },
    [UI_THEME_ONE_LIGHT] = {  /* One Light: dark ink on off-white */
        [UI_C_BG] = RGB(0xf8, 0xf8, 0xf8),       [UI_C_TEXT] = RGB(0x2a, 0x2b, 0x33),
        [UI_C_TEXT2] = RGB(0x6b, 0x6c, 0x75),    [UI_C_MUTED] = RGB(0xbb, 0xbb, 0xbb),
        [UI_C_SEL] = RGB(0xbb, 0xbb, 0xbb),      [UI_C_SEL_TEXT] = RGB(0x2a, 0x2b, 0x33),
        [UI_C_SEL_TEXT2] = RGB(0x6b, 0x6c, 0x75),
        [UI_C_TRACK] = RGB(0xde, 0xde, 0xe3),    [UI_C_PANEL] = RGB(0xf2, 0xf2, 0xf4),
        [UI_C_ARTIST] = RGB(0x2f, 0x5a, 0xf3),   [UI_C_ALBUM] = RGB(0x6b, 0x6c, 0x75),
        [UI_C_FILL] = RGB(0x2a, 0x2b, 0x33),
        [UI_C_BAR_LINE] = RGB(0x8a, 0x8b, 0x93),
        [UI_C_OUTLINE] = RGB(0xa9, 0xaa, 0xb3),
        [UI_C_EDGE] = RGB(0x9a, 0x9b, 0xa4),
        [UI_C_STATUS_FG] = RGB(0x2a, 0x2b, 0x33),
        /* the selection is the ink band: the palette's foreground on its
           selection-background */
        [UI_C_ON_ACCENT] = RGB(0xf8, 0xf8, 0xf8), [UI_C_ON_ACCENT2] = RGB(0xb9, 0xba, 0xc2),
        [UI_C_PH_BG] = RGB(0xd5, 0xd6, 0xdd),    [UI_C_PH_FG] = RGB(0x8b, 0x8c, 0x94),
        /* the fast-scroll letter: ink box, off-white letter */
        [UI_C_LETTER_TOP] = RGB(0x2a, 0x2b, 0x33), [UI_C_LETTER_BOTTOM] = RGB(0x1b, 0x1c, 0x22),
        [UI_C_LETTER_FG] = RGB(0xf8, 0xf8, 0xf8),
    },
    [UI_THEME_GITHUB_LIGHT] = {  /* GitHub Light: dark text on off-white */
        [UI_C_BG] = RGB(0xf6, 0xf8, 0xfa),       [UI_C_TEXT] = RGB(0x1f, 0x23, 0x28),
        [UI_C_TEXT2] = RGB(0x57, 0x60, 0x6a),    [UI_C_MUTED] = RGB(0x6e, 0x77, 0x81),
        [UI_C_SEL] = RGB(0xd0, 0xd7, 0xde),      [UI_C_SEL_TEXT] = RGB(0x1f, 0x23, 0x28),
        [UI_C_SEL_TEXT2] = RGB(0x57, 0x60, 0x6a),
        [UI_C_TRACK] = RGB(0xea, 0xee, 0xf2),    [UI_C_PANEL] = RGB(0xff, 0xff, 0xff),
        [UI_C_ARTIST] = RGB(0x09, 0x69, 0xda),   [UI_C_ALBUM] = RGB(0x57, 0x60, 0x6a),
        [UI_C_FILL] = RGB(0x26, 0x4f, 0x78),
        [UI_C_BAR_LINE] = RGB(0xd0, 0xd7, 0xde),
        [UI_C_OUTLINE] = RGB(0x8c, 0x95, 0x9f),
        [UI_C_EDGE] = RGB(0x8c, 0x95, 0x9f),
        [UI_C_STATUS_FG] = RGB(0x1f, 0x23, 0x28),
        /* the palette's selection-foreground (#1F2328) is too close to its
           selection-background (#264F78) for a 9 px font, so the band takes
           the palette's cursor-text instead */
        [UI_C_ON_ACCENT] = RGB(0xf6, 0xf8, 0xfa), [UI_C_ON_ACCENT2] = RGB(0x8c, 0x95, 0x9f),
        [UI_C_PH_BG] = RGB(0xd0, 0xd7, 0xde),    [UI_C_PH_FG] = RGB(0x6e, 0x77, 0x81),
        /* the fast-scroll letter: dark box, off-white letter */
        [UI_C_LETTER_TOP] = RGB(0x24, 0x29, 0x2f), [UI_C_LETTER_BOTTOM] = RGB(0x1f, 0x23, 0x28),
        [UI_C_LETTER_FG] = RGB(0xf6, 0xf8, 0xfa),
    },
};

int ui_theme(void)
{
    int t = global_settings.newui_theme;
    return (t >= 0 && t < UI_THEME_COUNT) ? t : UI_THEME_DARK;
}

bool ui_light(void)
{
    const int t = ui_theme();
    return t == UI_THEME_LIGHT || t == UI_THEME_ONE_LIGHT ||
           t == UI_THEME_GITHUB_LIGHT;
}

unsigned ui_col(enum ui_col c)
{
    return themes[ui_theme()][c];
}

unsigned ui_accent(void)
{
    return theme_accents[ui_theme()];
}

bool ui_minimal(void)
{
    return global_settings.newui_style == 1;
}

unsigned ui_sel_bg(void)
{
    return ui_minimal() ? UI_COL_SEL : ui_accent();
}

/* Is a band this colour light enough that the theme's text on it would wash
 * out? Every theme's accent is deep, so this only matters for a band the
 * screens build themselves. */
static bool band_is_light(unsigned c)
{
    return RGB_UNPACK_RED(c) + RGB_UNPACK_GREEN(c) + RGB_UNPACK_BLUE(c) > 3 * 140;
}

/* The text to draw on a band of `band`: the theme's on-accent pair, unless
 * the band is bright, in which case its own background reads better. */
unsigned ui_text_on(unsigned band)
{
    return band_is_light(band) ? UI_COL_BG : UI_COL_ON_ACCENT;
}

/* the same, for secondary text on the band */
unsigned ui_text_on2(unsigned band)
{
    return band_is_light(band) ? UI_COL_TRACK : UI_COL_ON_ACCENT2;
}

/* Text on an accent band; Minimal uses the theme's own selection colours. */
unsigned ui_sel_text(void)
{
    return ui_minimal() ? UI_COL_SEL_TEXT : ui_text_on(ui_accent());
}

unsigned ui_sel_text2(void)
{
    if (ui_minimal())
        return UI_COL_SEL_TEXT2;
    return ui_text_on2(ui_accent());
}

/* the iPod's glossy bar, from a lighter accent at the top to the accent
 * at the bottom */
unsigned ui_sel_top(void)
{
    unsigned c = ui_accent();
    int r = RGB_UNPACK_RED(c), g = RGB_UNPACK_GREEN(c), b = RGB_UNPACK_BLUE(c);
    return LCD_RGBPACK(r + (255 - r) * 2 / 5, g + (255 - g) * 2 / 5,
                       b + (255 - b) * 2 / 5);
}

/* Where the accent is a fill or an icon rather than a band (progress bars,
 * the CPU and battery plots, the status bar's play glyph): Nord's accent, the
 * deep selection blue, is too close to the dark tracks it sits on, so there
 * the accent's lighter end is used. */
unsigned ui_accent_highlight(void)
{
    return theme_accent_hi[ui_theme()];
}

unsigned ui_red(void)
{
    return theme_red[ui_theme()];
}

/* One selector for every list, menu and pop-up: a flat band of the
 * selection colour, edge to edge, exactly as Home draws it. No card, no
 * gloss, no border. */
void ui_selection(int x, int y, int w, int h)
{
    ui_fill_rect(x, y, w, h, ui_sel_bg());
}
