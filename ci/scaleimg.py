#!/usr/bin/env python3
"""Nearest-neighbour upscale of the simulator's 320x240 PNG shots.

Usage: ./ci/scale2x.py <srcdir> <dstdir> [factor]

Pure stdlib: the simulator writes plain RGB PNGs and the docs want them bigger
than 320x240 so the 9 px UI font is readable. Each pixel becomes a
factor x factor block; the LCD is 16 bpp, so the upscale is exact (no
interpolation to invent). Default factor 2.
"""
import os
import struct
import sys
import zlib


def read_png(path):
    data = open(path, "rb").read()
    pos, idat, w, h, bit, colour = 8, b"", 0, 0, 8, 2
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            w, h, bit, colour = struct.unpack(">IIBB", body[:10])
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    if bit != 8 or colour not in (2, 6):
        raise ValueError("%s: unsupported PNG (%d bit, colour %d)" % (path, bit, colour))

    channels = 3 if colour == 2 else 4
    stride = w * channels
    raw = zlib.decompress(idat)
    out = bytearray()
    prev = bytearray(stride)
    i = 0
    for _ in range(h):
        filt = raw[i]
        i += 1
        line = bytearray(raw[i:i + stride])
        i += stride
        for x in range(stride):
            a = line[x - 1] if x > 0 else 0
            b = prev[x]
            c = line[x - 2] if x > 1 else 0
            if filt == 1:
                line[x] = (line[x] + a) & 0xFF
            elif filt == 2:
                line[x] = (line[x] + b) & 0xFF
            elif filt == 3:
                line[x] = (line[x] + (a + b) // 2) & 0xFF
            elif filt == 4:
                pa, qa, qb, qc = a + b - c, abs(a - b - c), abs(b - c), abs(c - a)
                line[x] = (line[x] + (a if qa <= qb <= qc else b if qb <= qc else c)) & 0xFF
        prev = line
        out += line[:w * 3]
    return w, h, out


def write_png(path, w, h, rgb):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += rgb[y * w * 3:(y + 1) * w * 3]
    # IHDR is 13 bytes: width, height, bit depth, colour type, compression,
    # filter method, interlace. The last three were missing here, which made
    # the PNGs fail strict decoders.
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)

    def chunk(kind, body):
        return (struct.pack(">I", len(body)) + kind + body
                + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 6)))
        f.write(chunk(b"IEND", b""))


def scale_img(src, dst, factor):
    w, h, px = read_png(src)
    out = bytearray()
    for y in range(h):
        base = y * w * 3
        row = bytearray(w * 3 * factor)
        for x in range(w):
            r, g, b = px[base + 3 * x:base + 3 * x + 3]
            row[x * 3 * factor:(x + 1) * 3 * factor] = bytes((r, g, b)) * factor
        out += row * factor
    write_png(dst, w * factor, h * factor, out)


if __name__ == "__main__":
    if len(sys.argv) not in (3, 4):
        print(__doc__)
        sys.exit(1)
    src, dst = sys.argv[1], sys.argv[2]
    factor = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    os.makedirs(dst, exist_ok=True)
    n = 0
    for name in sorted(os.listdir(src)):
        if name.endswith(".png"):
            scale_img(os.path.join(src, name), os.path.join(dst, name), factor)
            n += 1
    print("wrote %d %dx screenshots to %s" % (n, factor, dst))
