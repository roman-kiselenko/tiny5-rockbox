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

#include "config.h"
#include "font.h"
#include "rbpaths.h"
#include "settings.h"
#include "ui.h"

/* Faces serve the whole UI, chosen at runtime by the newui_font setting:
 * the default Tiny5 Mono (a 9 px pixel font, SIL OFL 1.1,
 * https://github.com/Gissio/font_Tiny5Mono) and the alternate Kode Mono
 * Regular (fonts/newui/14-KodeMono-Regular.fnt). Both are installed by
 * buildzip from fonts/newui, and glyphs load from the card on demand into
 * the font's cache, so their size costs disk, not RAM. Each face is one
 * size for all five slots below; keeping the enum lets the sizes diverge
 * again later (see fonts/newui/mkfonts.sh for upscaled faces).
 *
 * The faces are expected to be loaded, so they are kept in a table keyed by
 * the setting; ui_font() picks the active family's id for the slot asked. */
#define UI_FONT_FAMILY_COUNT 2

enum ui_font_family
{
    UI_FONT_FAMILY_TINY5,      /* the default */
    UI_FONT_FAMILY_KODEMONO,   /* Kode Mono Regular */
};

static const char * const family_files[UI_FONT_FAMILY_COUNT] = {
    [UI_FONT_FAMILY_TINY5]    = FONT_DIR "/08-Tiny5Mono.fnt",
    [UI_FONT_FAMILY_KODEMONO] = FONT_DIR "/14-KodeMono-Regular.fnt",
};

static int font_ids[UI_FONT_FAMILY_COUNT][UI_FONT_COUNT];
static bool loaded;

/* The selected family, clamped to the range (a stale setting value falls
 * back to the default) */
static int ui_active_family(void)
{
    int f = global_settings.newui_font;
    if (f < 0 || f >= UI_FONT_FAMILY_COUNT)
        f = UI_FONT_FAMILY_TINY5;
    return f;
}

/* Pin the UI font to the family the user selected. It updates both the
 * firmware ui_font preference (what font_get(FONT_UI) returns) and the
 * screen's stored ui font (global_status.font_id[SCREEN_MAIN]), which
 * getuifont(), screen_helper_setfont(FONT_UI) and ui_resolve_font all read.
 * Screens that draw through the generic list/viewport or with lcd_puts
 * settle on that font, so updating it keeps every screen follow the
 * setting, not just those drawing with ui_font(). */
static void ui_pin_font(void)
{
    if (!loaded)
        return;
    int f = ui_font(UI_FONT_ROW);
    if (f > FONT_SYSFIXED)
        global_status.font_id[SCREEN_MAIN] = f;
    set_ui_font(f);
}

void ui_font_init(void)
{
    if (loaded)
        return;
    for (int i = 0; i < UI_FONT_FAMILY_COUNT; i++)
    {
        for (int j = 0; j < UI_FONT_COUNT; j++)
        {
            int id = font_load(family_files[i]);
            font_ids[i][j] = id >= 0 ? id : FONT_SYSFIXED;
        }
    }
    loaded = true;
    ui_pin_font();
}

int ui_resolve_font(int font)
{
    return font == FONT_UI ? global_status.font_id[SCREEN_MAIN] : font;
}

int ui_font(enum ui_font f)
{
    /* before boot is far enough for the fonts (the disk may not even be
       mounted), draw with the system font rather than fail to load them */
    return loaded ? font_ids[ui_active_family()][f] : FONT_SYSFIXED;
}

/* Re-pin the UI font to the family now selected (call after newui_font
 * changes) so every screen follows the setting */
void ui_font_apply(void)
{
    ui_pin_font();
}
