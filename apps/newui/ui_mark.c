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

/* The two marks cut from the Rockbox logo: the clef that stands for an
 * album with no cover, and the icon the player boots with.
 *
 * tools/svg2poly.py flattens the SVGs in docs/logo into the geometry headers
 * included below: points in a GRID x GRID box, grouped into rings, grouped
 * into painted paths, each path with its own fill. A path is filled even-odd
 * (a ring inside another is a hole) and the paths are painted in the order
 * the drawing has them. Every pixel row is sampled on four sub-scanlines, so
 * the marks stay crisp at any size and no bitmap is shipped.
 *
 * List rows redraw their placeholder on every wheel step, so the clef's
 * coverage is cached in RAM once per size. The icon is drawn straight on the
 * LCD, its alpha swept by the boot animation to fade it in. */

#include "config.h"
#include <string.h>
#include "lcd.h"
#include "misc.h"
#include "ui.h"

#include "clef_geometry.h"
#include "logo_geometry.h"

#define CLEF_FILL   75 /* the clef glyph takes this percent of the square */
#define LOGO_FILL  100
#define MARK_SUBS    4 /* sub-scanlines per pixel row */
#define MARK_MAX_X  64 /* enough crossings for one scanline of the outline */

struct mark {
    const int16_t (*pts)[2];
    const uint16_t *ring;  /* ring r spans ring[r] .. ring[r + 1] */
    const uint16_t *path;  /* path p's rings: path[p] .. path[p + 1] */
    const uint32_t *col;   /* the fill of each path, 0xRRGGBB */
    int paths, points, grid, fill;
};

static const struct mark clef_mark = {
    clef_pts, clef_ring_off, clef_path_first, clef_path_col,
    clef_PATHS, clef_POINTS, clef_GRID, CLEF_FILL
};
static const struct mark logo_mark = {
    logo_pts, logo_ring_off, logo_path_first, logo_path_col,
    logo_PATHS, logo_POINTS, logo_GRID, LOGO_FILL
};

/* The scaled outline of the path being filled: one edge per point, each
 * ring closed, in 1/256 px relative to the square's top left. */
struct edge { long x0, y0, x1, y1; };
static struct edge edges[logo_POINTS];
static int nedges;
static int cov[UI_ART_MAX]; /* coverage of each column of the current row */
static long px0, px1, py0, py1; /* the path's box, 1/256 px from the corner */

static void scale_path(const struct mark *m, int p, int size)
{
    long sc = (long)size * m->fill * 256 / (m->grid * 100);
    long gx0 = m->pts[0][0], gx1 = gx0, gy0 = m->pts[0][1], gy1 = gy0;
    for (int i = 1; i < m->points; i++)
    {
        if (m->pts[i][0] < gx0) gx0 = m->pts[i][0];
        if (m->pts[i][0] > gx1) gx1 = m->pts[i][0];
        if (m->pts[i][1] < gy0) gy0 = m->pts[i][1];
        if (m->pts[i][1] > gy1) gy1 = m->pts[i][1];
    }
    /* centred on the mark's own bounding box, which need not touch the
     * edges of the normalised grid */
    long ox = (size * 256 - (gx1 - gx0) * sc) / 2 - gx0 * sc;
    long oy = (size * 256 - (gy1 - gy0) * sc) / 2 - gy0 * sc;

    nedges = 0;
    px0 = px1 = py0 = py1 = 0;
    for (int r = m->path[p]; r < m->path[p + 1]; r++)
    {
        const int first = m->ring[r], last = m->ring[r + 1];
        for (int i = first; i < last; i++)
        {
            const int j = i + 1 < last ? i + 1 : first;
            edges[nedges].x0 = m->pts[i][0] * sc + ox;
            edges[nedges].y0 = m->pts[i][1] * sc + oy;
            edges[nedges].x1 = m->pts[j][0] * sc + ox;
            edges[nedges].y1 = m->pts[j][1] * sc + oy;
            if (edges[nedges].x0 < px0) px0 = edges[nedges].x0;
            if (edges[nedges].x1 > px1) px1 = edges[nedges].x1;
            if (edges[nedges].y0 < py0) py0 = edges[nedges].y0;
            if (edges[nedges].y1 > py1) py1 = edges[nedges].y1;
            nedges++;
        }
    }
    if (nedges == 0)
        px0 = px1 = py0 = py1 = 0;
}

/* The pixel rows and columns the path can touch */
static int row0(void) { return MAX(0, (int)((py0 + 255) / 256)); }
static int row1(int size) { return MIN(size - 1, (int)((py1 - 1) / 256)); }
static int col0(void) { return MAX(0, (int)(px0 / 256)); }
static int col1(int size) { return MIN(size - 1, (int)((px1 - 1) / 256)); }

/* Where yy (1/256 px) crosses the outline, sorted; the crossings come in
 * pairs, between which the path is filled. */
static int crossings(long yy, long *xs)
{
    int n = 0;
    for (int i = 0; i < nedges && n < MARK_MAX_X; i++)
    {
        long y0 = edges[i].y0, y1 = edges[i].y1;
        if ((y0 <= yy && y1 > yy) || (y1 <= yy && y0 > yy))
            xs[n++] = edges[i].x0 + (yy - y0) * (edges[i].x1 - edges[i].x0)
                                    / (y1 - y0);
    }
    for (int i = 1; i < n; i++) /* the list is nearly sorted already */
    {
        long v = xs[i];
        int j = i - 1;
        while (j >= 0 && xs[j] > v)
        {
            xs[j + 1] = xs[j];
            j--;
        }
        xs[j + 1] = v;
    }
    return n;
}

/* How much of each column of pixel row `row` the path covers, 0..256;
 * only columns c0..c1 can be touched, so only those are cleared */
static void row_coverage(int c0, int c1, int row)
{
    long xs[MARK_MAX_X];
    for (int i = c0; i <= c1; i++)
        cov[i] = 0;

    for (int s = 0; s < MARK_SUBS; s++)
    {
        long yy = (long)row * 256 + s * (256 / MARK_SUBS) + 256 / (2 * MARK_SUBS);
        int n = crossings(yy, xs);
        for (int i = 0; i + 1 < n; i += 2)
        {
            long a = xs[i], b = xs[i + 1];
            int k0 = MAX(c0, (int)(a / 256)), k1 = MIN(c1, (int)((b - 1) / 256));
            for (int k = k0; k <= k1; k++)
            {
                long lo = MAX(a, (long)k * 256), hi = MIN(b, (long)(k + 1) * 256);
                if (hi > lo)
                    cov[k] += hi - lo;
            }
        }
    }
    for (int i = c0; i <= c1; i++)
        cov[i] /= MARK_SUBS;
}

/* --- the clef: the mark of an album with no cover --- */

static uint16_t clef_glyph[UI_COVER * UI_COVER]; /* coverage, for size <= UI_COVER */
static int clef_glyph_size;

static void clef_glyph_build(int size)
{
    if (clef_glyph_size == size)
        return;
    scale_path(&clef_mark, 0, size);
    const int r0 = row0(), r1 = row1(size), c0 = col0(), c1 = col1(size);
    memset(clef_glyph, 0, size * size * sizeof(clef_glyph[0]));
    for (int row = r0; row <= r1; row++)
    {
        row_coverage(c0, c1, row);
        for (int col = c0; col <= c1; col++)
            clef_glyph[row * size + col] = cov[col];
    }
    clef_glyph_size = size;
}

/* The clef centred in the size x size square at x,y, blended over what is
 * already there. */
void ui_clef_draw(int x, int y, int size, unsigned color)
{
    if (size <= 0 || size > UI_ART_MAX)
        return;

    if (size <= UI_COVER)
    {
        clef_glyph_build(size);
        for (int row = 0; row < size; row++)
            for (int col = 0; col < size; col++)
            {
                int a = clef_glyph[row * size + col];
                if (a)
                    ui_blend_pixel(x + col, y + row, color, a);
            }
        return;
    }

    scale_path(&clef_mark, 0, size);
    for (int row = row0(); row <= row1(size); row++)
    {
        row_coverage(col0(), col1(size), row);
        for (int col = col0(); col <= col1(size); col++)
            if (cov[col])
                ui_blend_pixel(x + col, y + row, color, cov[col]);
    }
}

/* The clef in a size x size native-pixel buffer: the glyph over bg. */
void ui_clef_fill(fb_data *dst, int size, unsigned bg, unsigned color)
{
    if (size <= 0 || size > UI_ART_MAX)
        return;
    const fb_data bgp = FB_SCALARPACK(bg);
    for (int i = 0; i < size * size; i++)
        dst[i] = bgp;

    scale_path(&clef_mark, 0, size);
    for (int row = row0(); row <= row1(size); row++)
    {
        row_coverage(col0(), col1(size), row);
        fb_data *d = dst + row * size;
        for (int col = col0(); col <= col1(size); col++)
            if (cov[col])
                d[col] = FB_SCALARPACK(ui_mix(bg, color, cov[col]));
    }
}

/* --- the Rockbox icon: the boot splash --- */

/* The icon, size px across, its top-left corner at x,y, painted over what is
 * already there at alpha 0..256: 0 is invisible, 256 solid. The boot
 * animation sweeps alpha from 0 to 256, which is a fade-in. */
void ui_logo_draw(int x, int y, int size, int alpha)
{
    if (size <= 0 || size > UI_ART_MAX || alpha <= 0)
        return;
    if (alpha > 256)
        alpha = 256;

    for (int p = 0; p < logo_mark.paths; p++)
    {
        const uint32_t c = logo_mark.col[p];
        const unsigned color = LCD_RGBPACK(c >> 16, (c >> 8) & 0xff, c & 0xff);
        scale_path(&logo_mark, p, size);
        for (int row = row0(); row <= row1(size); row++)
        {
            row_coverage(col0(), col1(size), row);
            for (int col = col0(); col <= col1(size); col++)
                if (cov[col])
                    ui_blend_pixel(x + col, y + row, color,
                                   cov[col] * alpha / 256);
        }
    }
}
