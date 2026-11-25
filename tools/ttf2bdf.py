#!/usr/bin/env python3
"""
ttf2bdf - turn a TrueType font into a Rockbox .bdf, no dependencies.

tools/convttf needs FreeType; this is the fall-back for a plain stdlib
Python. It reads the glyf/loca/cmap tables, quadratically decodes the
outline contours, scanline fills them at a chosen pixel size and writes
X11 BDF, which tools/convbdf then turns into a Rockbox .fnt.

Only simple (non-composite) glyphs are supported, which is enough for
the small numeric faces we rasterise by hand.

    python3 tools/ttf2bdf.py -c '0123456789:-' -p 9 -W 6 -A 9 -D 0 \
        -o fonts/newui/09-Digital7.bdf fonts/newui/digital-7-mono.ttf
"""

import argparse
import struct
import sys


def tables(data):
    """The sfnt table directory as {tag: bytes}.

    Entries are 12 bytes (tag, offset, length); some tools write 16 with a
    checksum after the tag, so both are tried and the sane one wins.
    """
    count = struct.unpack(">H", data[4:6])[0]
    for entry in (12, 16):
        found = {}
        ok = True
        for i in range(count):
            base = 12 + entry * i
            rec = data[base:base + entry]
            if len(rec) < entry:
                ok = False
                break
            tag = rec[:4]
            off, length = struct.unpack(">II", rec[entry - 8:entry])
            if off + length > len(data) or not tag.strip(b"\0"):
                ok = False
                break
            found[tag] = data[off:off + length]
        if ok and all(k in found for k in (b"head", b"cmap", b"glyf",
                                           b"loca", b"maxp")):
            return found
    raise ValueError("cannot read the table directory")


def u16(d, o):
    return struct.unpack(">H", d[o:o + 2])[0]


def i16(d, o):
    return struct.unpack(">h", d[o:o + 2])[0]


def char_glyphs(cmap, chars):
    """Character code -> glyph index, from the Unicode cmap subtable."""
    num = u16(cmap, 2)
    best = None
    for i in range(num):
        base = 4 + 8 * i
        if base + 8 > len(cmap):
            break
        platform, encoding, sub = struct.unpack(">HHI", cmap[base:base + 8])
        if (platform, encoding) in ((3, 1), (0, 4), (0, 3)):
            best = sub
        elif best is None and platform == 3:
            best = sub
    if best is None:
        raise ValueError("no usable cmap subtable")

    fmt = u16(cmap, best)
    if fmt != 4:
        raise ValueError("only format 4 cmap subtables are supported (got %d)"
                         % fmt)

    nseg = u16(cmap, best + 6) // 2
    ends = [u16(cmap, best + 14 + 2 * i) for i in range(nseg)]
    starts = [u16(cmap, best + 16 + 2 * nseg + 2 * i) for i in range(nseg)]
    deltas = [i16(cmap, best + 16 + 4 * nseg + 2 * i) for i in range(nseg)]
    rangeoff = [u16(cmap, best + 16 + 6 * nseg + 2 * i) for i in range(nseg)]

    out = {}
    for ch in chars:
        for s in range(nseg):
            if not (starts[s] <= ch <= ends[s]):
                continue
            if rangeoff[s]:
                # the glyph array is addressed relative to the
                # idRangeOffset entry itself
                pos = (best + 16 + 6 * nseg + 2 * s
                      + rangeoff[s] + 2 * (ch - starts[s]))
                gid = u16(cmap, pos)
                if gid == 0xFFFF:
                    break
                gid = (gid + deltas[s]) & 0xFFFF
            else:
                gid = (ch + deltas[s]) & 0xFFFF
            if gid:
                out[ch] = gid
            break
    return out


def glyph_bytes(glyf, loca, gid, long_index):
    if long_index:
        start = struct.unpack(">I", loca[4 * gid:4 * gid + 4])[0]
        end = struct.unpack(">I", loca[4 * gid + 4:4 * gid + 8])[0]
    else:
        start = u16(loca, 2 * gid) * 2
        end = u16(loca, 2 * gid + 2) * 2
    return glyf[start:end]


def contours(glyf, loca, gid, long_index):
    """Closed polygons of one simple glyph, in font units, y upwards."""
    g = glyph_bytes(glyf, loca, gid, long_index)
    if not g:
        return []
    ncont = i16(g, 0)
    if ncont <= 0:
        raise ValueError("glyph %d is empty or composite "
                         "(this tool only handles simple glyphs)" % gid)

    ends = [u16(g, 10 + 2 * i) for i in range(ncont)]
    npts = ends[-1] + 1
    p = 10 + 2 * ncont
    p += 2 + i16(g, p)                       # skip the hinting instructions

    flags = []
    while len(flags) < npts:
        f = g[p]
        p += 1
        if f & 0x08:                        # repeat the low bits N+1 times
            flags.extend([f & ~0x08] * (g[p] + 1))
            p += 1
        else:
            flags.append(f)

    # The deltas are packed as all the x's, then all the y's.
    xfmt = ">"
    yfmt = ">"
    for f in flags:
        if f & 0x02:
            xfmt += "B"
        elif not (f & 0x10):
            xfmt += "h"
        if f & 0x04:
            yfmt += "B"
        elif not (f & 0x20):
            yfmt += "h"
    xs = struct.unpack(xfmt, g[p:p + struct.calcsize(xfmt)])
    p += struct.calcsize(xfmt)
    ys = struct.unpack(yfmt, g[p:p + struct.calcsize(yfmt)])

    pts = []
    x = y = 0
    i = j = 0
    for f in flags:
        if f & 0x02:                        # 8-bit delta, bit 4 is its sign
            x += xs[i] if f & 0x10 else -xs[i]
            i += 1
        elif not (f & 0x10):                # 16-bit delta
            x += xs[i]
            i += 1
        if f & 0x04:                        # 8-bit delta, bit 5 is its sign
            y += ys[j] if f & 0x20 else -ys[j]
            j += 1
        elif not (f & 0x20):
            y += ys[j]
            j += 1
        pts.append((x, y, f & 1))

    polys = []
    last = 0
    for e in ends:
        polys.append(curve(pts[last:e + 1]))
        last = e + 1
    return polys


def curve(pts):
    """Replace the off-curve points with samples of their quadratics."""
    out = []
    n = len(pts)
    i = 0
    while i < n:
        x, y, on = pts[i]
        out.append((x, y))
        if on:
            i += 1
            continue
        # a run of off-curve points: each is the control point of a
        # quadratic between the on-curve points around it
        j = i
        while j < n and not pts[j][2]:
            cx, cy = pts[j][0], pts[j][1]
            nx, ny = pts[(j + 1) % n][0], pts[(j + 1) % n][1]
            px, py = pts[j - 1][0], pts[j - 1][1]
            for s in range(1, 16):
                t = s / 16.0
                out.append(((1 - t) * (1 - t) * px + 2 * (1 - t) * t * cx +
                            t * t * nx,
                            (1 - t) * (1 - t) * py + 2 * (1 - t) * t * cy +
                            t * t * ny))
            j += 1
        i = j
    return out


SS = 4                             # rasterise SSx the target size and average,
                                   # so a feature thinner than a pixel survives
def fill(polys, w, h, scale, ox, oy):
    """Scanline fill at SS x the resolution, box-filtered to pixels;
    returns h rows (top row first) of 0/1."""
    W, H = w * SS, h * SS
    grid = [0] * (W * H)
    s = scale * SS
    ox, oy = ox * SS, oy * SS
    edges = []
    for poly in polys:
        for (ax, ay), (bx, by) in zip(poly, poly[1:] + poly[:1]):
            ax = ax * s + ox
            ay = ay * s + oy
            bx = bx * s + ox
            by = by * s + oy
            if ay == by:
                continue
            if ay > by:
                ax, ay, bx, by = bx, by, ax, ay
            edges.append((ax, ay, bx, by))

    for row in range(H):
        yc = row + 0.5                  # middle of the sub-pixel row
        hits = sorted(ax + (bx - ax) * (yc - ay) / (by - ay)
                      for ax, ay, bx, by in edges if ay <= yc < by)
        for k in range(0, len(hits) - 1, 2):
            lo = int(hits[k] + 0.5)
            hi = int(hits[k + 1] + 0.5)
            for c in range(max(0, lo), min(W, hi)):
                grid[row * W + c] = 1

    out = []
    # A pixel is inked when at least a third of it is covered: at these sizes
    # a segment is well under a pixel wide, and a stricter rule loses the thin
    # features (the colon's dots, the hyphen's ends).
    limit = SS * SS / 3
    for row in range(h):                      # row 0 is the top pixel row
        line = []
        for c in range(w):
            n = 0
            for dy in range(SS):
                base = (H - 1 - (row * SS + dy)) * W + c * SS
                n += sum(grid[base:base + SS])
            line.append(1 if n > limit else 0)
        out.append(line)
    return out


def bitmap_lines(rows, width):
    """One BITMAP line per row: ceil(width/4) hex nibbles, leftmost column in
    the most significant bit, the last nibble zero-padded."""
    nibbles = (width + 3) // 4
    shift = nibbles * 4 - width
    out = []
    for row in rows:
        v = 0
        for c in range(width):
            v = (v << 1) | row[c]
        out.append("%0*x" % (nibbles, v << shift))
    return out


def main():
    a = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    a.add_argument("ttf")
    a.add_argument("-c", default="0123456789:", help="characters to emit")
    a.add_argument("-p", type=int, default=9, help="glyph height in pixels")
    a.add_argument("-o", help="output .bdf (default: <height>-<name>.bdf)")
    a.add_argument("-W", type=int, help="cell width (default: widest glyph)")
    a.add_argument("-A", type=int, help="ascent (default: from the outlines)")
    a.add_argument("-D", type=int, help="descent below the baseline, pixels")
    a.add_argument("-f", default="Digital-7", help="BDF font name")
    a.add_argument("-d", action="store_true", help="dump the glyphs as text")
    args = a.parse_args()

    data = open(args.ttf, "rb").read()
    t = tables(data)
    loca = t[b"loca"]
    nglyphs = u16(t[b"maxp"], 4)
    long_index = len(loca) == 4 * (nglyphs + 1)

    chars = sorted({ord(c) for c in args.c})
    gids = char_glyphs(t[b"cmap"], chars)
    missing = [chr(c) for c in chars if c not in gids]
    if missing:
        sys.exit("the font has no glyphs for: %s" % "".join(missing))

    polys = {ch: contours(t[b"glyf"], loca, gids[ch], long_index)
             for ch in chars}
    pts = [p for cont in polys.values() for poly in cont for p in poly]
    if not pts:
        sys.exit("no outlines found")

    # Scale every character by the same factor, the tallest one setting the
    # height, so the shapes keep their proportions.
    min_x = min(p[0] for p in pts)
    max_x = max(p[0] for p in pts)
    min_y = min(p[1] for p in pts)
    max_y = max(p[1] for p in pts)
    if min_y >= max_y:
        sys.exit("degenerate outlines")

    scale = float(args.p) / (max_y - min_y)
    width = args.W or int(round((max_x - min_x) * scale)) + 1
    descent = args.D if args.D is not None else int(round(-min_y * scale))
    height = args.p + descent if args.D is not None else args.p
    ascent = args.A if args.A is not None else height - descent
    # Put the glyph's bottom on the baseline, `descent` rows above the cell's
    # bottom, not at the bottom of the cell.
    ox, oy = -min_x * scale, descent - min_y * scale

    out = args.o or "%d-%s.bdf" % (args.p, args.f)
    with open(out, "w") as f:
        f.write("STARTFONT 2.1\n")
        f.write("FONT -Sizenko Alexander-Digital 7-Medium-R-Regular\n")
        f.write("SIZE %d 100 100\n" % height)
        f.write("FONTBOUNDINGBOX %d %d 0 %d\n" % (width, height, -descent))
        f.write("STARTPROPERTIES 3\n")
        f.write("FAMILY_NAME \"%s\"\n" % args.f)
        f.write("FONT_ASCENT %d\n" % ascent)
        f.write("FONT_DESCENT %d\n" % descent)
        f.write("ENDPROPERTIES\n")
        f.write("CHARS %d\n" % len(chars))
        for ch in chars:
            rows = fill(polys[ch], width, height, scale, ox, oy)
            f.write("STARTCHAR %s\n" % (chr(ch) if 33 <= ch < 127
                                        else "U+%04X" % ch))
            f.write("ENCODING %d\n" % ch)
            f.write("SWIDTH 500 0\n")
            f.write("DWIDTH %d 0\n" % width)
            f.write("BBX %d %d 0 %d\n" % (width, height, -descent))
            f.write("BITMAP\n")
            for line in bitmap_lines(rows, width):
                f.write(line + "\n")
            f.write("ENDCHAR\n")
        f.write("ENDFONT\n")

    if args.d:
        for ch in chars:
            print("  %s" % chr(ch))
            for row in fill(polys[ch], width, height, scale, ox, oy):
                print("  " + "".join("#" if v else "." for v in row))
    print("wrote %s: %d chars, cell %d x %d, ascent %d, descent %d" %
          (out, len(chars), width, height, ascent, descent))


if __name__ == "__main__":
    main()
