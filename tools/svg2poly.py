#!/usr/bin/env python3
"""svg2poly.py - turn an SVG glyph into polygons the firmware can fill.

    ./svg2poly.py docs/logo/rockbox-clef.svg > apps/newui/clef_geometry.h
    ./svg2poly.py docs/logo/rockbox-icon.svg  > apps/newui/logo_geometry.h

Reads the <path> elements of an SVG, flattens every curve into line
segments, normalises the shape into a GRID x GRID box (aspect kept, the
longer side filling the box), and prints a C header with the even-odd
filled outline, painted path by path in the order the SVG has them:

    NAME_POINTS  int16_t name_pts[NAME_POINTS][2]   x,y, y down, 0..GRID
    NAME_RINGS   uint16_t name_ring_off[NAME_RINGS+1] first point of each ring
    NAME_PATHS   uint16_t name_path_first[NAME_PATHS+1] first ring of each path
    NAME_PATHS   uint32_t name_path_col[NAME_PATHS]   fill, 0xRRGGBB

A path's rings are filled together (a ring inside another is a hole), and
paths are layered in document order, so a mark with several colours keeps
its look. Paths without a fill are taken as black.

Stdlib only. Options:
    --grid N     normalisation box (default 1024)
    --tol U      curve subdivision tolerance in user units (default: one
                 GRID unit of the shape's larger side)
    --paths IDs  comma separated ids of the <path> elements to keep
    --name X     prefix of the emitted names (default: clef)
    --preview W  print an ASCII render of the result instead of C
"""

import argparse
import math
import re
import sys


def tokens(d):
    """[('c', [numbers...]), ...] from a path's d attribute."""
    out = []
    for m in re.finditer(r"([A-Za-z])|([-+]?[0-9]*\.?[0-9]+(?:e[-+]?[0-9]+)?)", d):
        t = m.group(1) or m.group(2)
        if t.isalpha():
            out.append([t, []])
        elif out:
            out[-1][1].append(float(t))
    return out


def flatten_cubic(p0, p1, p2, p3, tol, out, depth=0):
    """Append the polyline of a cubic, subdividing until it is within tol
    of the chord."""
    dx, dy = p3[0] - p0[0], p3[1] - p0[1]
    ln = math.hypot(dx, dy)
    if ln <= 1e-9 or depth > 20:
        out.append(p3)
        return
    d1 = abs((p1[0] - p0[0]) * dy - (p1[1] - p0[1]) * dx) / ln
    d2 = abs((p2[0] - p0[0]) * dy - (p2[1] - p0[1]) * dx) / ln
    if d1 <= tol and d2 <= tol:
        out.append(p3)
        return
    a = ((p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2)
    b = ((p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2)
    c = ((p2[0] + p3[0]) / 2, (p2[1] + p3[1]) / 2)
    d = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
    e = ((b[0] + c[0]) / 2, (b[1] + c[1]) / 2)
    f = ((d[0] + e[0]) / 2, (d[1] + e[1]) / 2)
    flatten_cubic(p0, a, d, f, tol, out, depth + 1)
    flatten_cubic(f, e, c, p3, tol, out, depth + 1)


def rings(path, tol):
    """The subpaths of a d attribute, as lists of (x, y)."""
    subs, cur, start, c = [], [], (0.0, 0.0), (0.0, 0.0)
    for cmd, args in tokens(path):
        kind, rel = cmd.upper(), cmd.islower()
        if kind in "M":
            if cur:
                subs.append(cur)
            x, y = args[0], args[1]
            if rel:
                x, y = c[0] + x, c[1] + y
            cur = [(x, y)]
            start, c = cur[0], cur[0]
            # a moveto with more pairs means implicit lineto's
            for i in range(2, len(args), 2):
                x, y = args[i], args[i + 1]
                if rel:
                    x, y = c[0] + x, c[1] + y
                cur.append((x, y))
                c = cur[-1]
        elif kind in "L":
            for i in range(0, len(args), 2):
                x, y = args[i], args[i + 1]
                if rel:
                    x, y = c[0] + x, c[1] + y
                cur.append((x, y))
                c = cur[-1]
        elif kind == "H":
            for x in args:
                c = (c[0] + x if rel else x, c[1])
                cur.append(c)
        elif kind == "V":
            for y in args:
                c = (c[0], c[1] + y if rel else y)
                cur.append(c)
        elif kind == "C":
            for i in range(0, len(args), 6):
                a, b, cc, dd, e, f = args[i:i + 6]
                if rel:
                    a, b, cc, dd, e, f = (a + c[0], b + c[1], cc + c[0],
                                         dd + c[1], e + c[0], f + c[1])
                flatten_cubic(c, (a, b), (cc, dd), (e, f), tol, cur)
                c = (e, f)
        elif kind == "Q":
            for i in range(0, len(args), 4):
                a, b, cc, dd = args[i:i + 4]
                if rel:
                    a, b, cc, dd = a + c[0], b + c[1], cc + c[0], dd + c[1]
                flatten_cubic(c, (c[0] + 2 / 3 * (a - c[0]),
                                 c[1] + 2 / 3 * (b - c[1]),
                                 cc + 1 / 3 * (a - cc), dd + 1 / 3 * (b - dd),),
                             (cc, dd), tol, cur)
                c = (cc, dd)
        elif kind == "Z":
            cur.append(start)
            c = start
        else:
            sys.exit("%s: unsupported path command %s" % (cmd, cmd))
        if cur:
            c = cur[-1]
    if cur:
        subs.append(cur)
    return subs


def simplify(ring, tol):
    """Drop repeated and collinear points; keep the ring closed."""
    out = []
    for p in ring:
        if out and math.hypot(p[0] - out[-1][0], p[1] - out[-1][1]) <= tol:
            continue
        out.append(p)
    while len(out) > 1 and math.hypot(out[-1][0] - out[0][0],
                                      out[-1][1] - out[0][1]) <= tol:
        out.pop()          # the closing point that repeats the first
    if len(out) > 2:
        keep = []
        for i, p in enumerate(out):
            a, b = out[i - 1], out[(i + 1) % len(out)]
            dx, dy = b[0] - a[0], b[1] - a[1]
            ln = math.hypot(dx, dy)
            if ln > 1e-9 and abs((p[0] - a[0]) * dy - (p[1] - a[1]) * dx) / ln > tol:
                keep.append(p)
        # a rectangle has no corner that is not collinear with its
        # neighbours; never shrink a ring below a triangle
        if len(keep) > 2:
            out = keep
    return out


def normalise(subs, grid):
    xs = [p[0] for s in subs for p in s]
    ys = [p[1] for s in subs for p in s]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    span = max(x1 - x0, y1 - y0)
    sc = grid / span
    ox = (grid - (x1 - x0) * sc) / 2
    oy = (grid - (y1 - y0) * sc) / 2
    return [[(round((p[0] - x0) * sc + ox), round((p[1] - y0) * sc + oy))
            for p in s] for s in subs]


def preview(paths, width):
    """An ASCII render, path by path, to check what the tool found."""
    xs = [p[0] for _, s in paths for r in s for p in r]
    ys = [p[1] for _, s in paths for r in s for p in r]
    if not xs:
        sys.exit("nothing to draw")
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    cell = (x1 - x0) / width
    rows = int(width * (y1 - y0) / (x1 - x0))
    chars = {}
    for col, _ in paths:
        if col not in chars:
            chars[col] = "#@%+=*:-. "[len(chars) % 10]
    grid = [[" "] * width for _ in range(rows)]
    for col, subs in paths:
        ch = chars[col]
        edges = []
        for r in subs:
            for i in range(len(r)):
                p0, p1 = r[i], r[(i + 1) % len(r)]
                if p0[1] != p1[1]:
                    edges.append((p0, p1))
        for r in range(rows):
            yy = y0 + (r + 0.5) * (y1 - y0) / rows
            cs = []
            for p0, p1 in edges:
                if (p0[1] <= yy < p1[1]) or (p1[1] <= yy < p0[1]):
                    t = (yy - p0[1]) / (p1[1] - p0[1])
                    cs.append(p0[0] + t * (p1[0] - p0[0]))
            cs.sort()
            for c in range(width):
                a, b = x0 + c * cell, x0 + (c + 1) * cell
                cov = 0.0
                for i in range(0, len(cs) - 1, 2):
                    lo, hi = max(cs[i], a), min(cs[i + 1], b)
                    if hi > lo:
                        cov += hi - lo
                if cov > cell / 2:
                    grid[r][c] = ch
    for r in grid:
        print("".join(r))

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("svg")
    ap.add_argument("--grid", type=int, default=1024)
    ap.add_argument("--tol", type=float)
    ap.add_argument("--paths")
    ap.add_argument("--name", default="clef")
    ap.add_argument("--preview", type=int)
    a = ap.parse_args()

    src = open(a.svg).read()
    elems = re.findall(r"<path\b.*?>", src, re.S)
    kept = []
    for e in elems:
        d = re.search(r'\bd="([^"]*)"', e)
        if not d:
            continue
        i = re.search(r'\bid="([^"]*)"', e)
        if a.paths and (not i or i.group(1) not in a.paths.split(",")):
            continue
        c = re.search(r'fill:\s*#?([0-9a-fA-F]{3,6})', e) or \
            re.search(r'\bfill="\s*#?([0-9a-fA-F]{3,6})"', e)
        col = int(c.group(1), 16) if c else 0
        kept.append((d.group(1), col))
    if not kept:
        sys.exit("%s: no <path> elements" % a.svg)

    nums = [n for d, _ in kept for _, args in tokens(d) for n in args]
    vb = re.search(r'viewBox="[^" ]* [^" ]* ([0-9.]+) ([0-9.]+)"', src)
    span = max(float(vb.group(1)), float(vb.group(2))) if vb else \
        (max(nums) - min(nums) if nums else 1.0)
    # a tolerance of half a grid cell in the drawing's own units
    tol = a.tol if a.tol is not None else span / a.grid * 0.5

    paths = []
    for d, col in kept:
        rs = [s for s in (simplify(s, tol) for s in rings(d, tol)) if len(s) > 2]
        if rs:
            paths.append((col, rs))
    if not paths:
        sys.exit("%s: nothing left after flattening" % a.svg)
    flat = normalise([r for _, rs in paths for r in rs], a.grid)
    out, n = [], 0
    for col, rs in paths:
        out.append((col, flat[n:n + len(rs)]))
        n += len(rs)
    paths = out
    allr = [r for _, rs in paths for r in rs]

    if a.preview:
        preview(paths, a.preview)
        return

    total = sum(len(r) for r in allr)
    print("/* Generated by tools/svg2poly.py from %s --grid %d. Do not edit. */"
          % (a.svg.split("/")[-1] if "/" in a.svg else a.svg, a.grid))
    print("/* %d paths, %d rings, %d points, in a %d x %d box, y down. Each "
          "path is filled even-odd (a ring inside another is a hole) and the "
          "paths are painted in order. */" % (len(paths), len(allr), total, a.grid, a.grid))
    print("#define %s_GRID %d" % (a.name, a.grid))
    print("#define %s_POINTS %d" % (a.name, total))
    print("#define %s_RINGS %d" % (a.name, len(allr)))
    print("#define %s_PATHS %d" % (a.name, len(paths)))
    print("static const int16_t %s_pts[%s_POINTS][2] = {" % (a.name, a.name))
    for r in allr:
        print("    " + ", ".join("{%d,%d}" % p for p in r) + ",")
    print("};")
    off, n = [], 0
    for r in allr:
        off.append(n)
        n += len(r)
    off.append(n)
    print("static const uint16_t %s_ring_off[%s_RINGS + 1] = {%s};"
          % (a.name, a.name, ", ".join(str(o) for o in off)))
    first, n = [], 0
    for _, rs in paths:
        first.append(n)
        n += len(rs)
    first.append(n)
    print("static const uint16_t %s_path_first[%s_PATHS + 1] = {%s};"
          % (a.name, a.name, ", ".join(str(o) for o in first)))
    print("static const uint32_t %s_path_col[%s_PATHS] = {%s};"
          % (a.name, a.name, ", ".join("0x%06x" % c for c, _ in paths)))


if __name__ == "__main__":
    main()
