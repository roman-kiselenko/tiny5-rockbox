# Working on the code

Build and install step by step: [README](README) — this file is about the
build types, testing changes, and releases.

## Build types

The MIPS cross-compiler is built once with `tools/rockboxdev.sh`
(`RBDEV_TARGET=i`) and lives in `~/rockbox-toolchain`. Configure outside the
source tree, one directory per build:

```bash
../tools/configure --target=erosqnative --type=n   # firmware
../tools/configure --target=erosqnative --type=s   # simulator (libsdl2-dev)
../tools/configure --target=erosqnative --type=w   # warble, the codec test tool
```

`--type=a<opts>` builds advanced variants: `l` logf, `i` logf to serial,
`d` debug, `u` USB serial, `m` small C library, `x` LTO. Profiling (`p`) does
not build on MIPS, and it is broken upstream too.

## Testing a change in the simulator

The simulator runs the same code with the same screens, and can be driven from
a key script: point `RB_SCRIPT` at a script and `RB_SHOTDIR` at a directory,
run it headless, and it writes a PNG per step.

```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  RB_SCRIPT=../ci/smoke.script RB_SHOTDIR=./shots timeout 60 ./rockboxui
```

Commands are `wait`, `press`, `hold`, `wheel`, `usb`, `headset`, `shot` and
`quit` (`uisimulator/common/sim_script.c`). Its virtual SD card is the
`simdisk/` directory in the build directory — put test music there. Create the
shot directory first: the script writes with host `fopen`, and the simulator
just prints "cannot write" if it is missing.

Simulator keys: arrows = scroll wheel / previous / next, Enter = centre,
Backspace = back, Insert = menu, PgUp/PgDn = volume, Esc = power.

### The screenshots in docs/screens

`ci/screens.sh` regenerates them. From the simulator build directory:

```bash
../ci/screens.sh          # one run per theme
```

It walks every screen with `ci/screens.script` (Home, Now Playing three ways,
Cover Flow, the file browser, Settings and a submenu, the System menu, System
Status, Battery Usage), then `ci/scale2x.py` doubles the 320×240 shots to the
640×480 the docs use, writing `docs/screens/<theme>/`. The theme is set in
`simdisk/.rockbox/config.cfg`, so adding a palette needs no change to the
script.

The lock screen is not in the set: entering it needs a key press while the keys
are locked, and the press that wakes the screen also ends it, so a headless run
never catches it displayed.

Settings in `simdisk/.rockbox/config.cfg` are read exactly as on the player,
by their config names: `theme: nord`, `selection style: minimal`.

## Fonts

`tools/convbdf` turns a `.bdf` into a Rockbox `.fnt`; it reproduces the
committed Tiny5 face byte for byte:

```bash
cc -O2 -o /tmp/convbdf tools/convbdf.c
/tmp/convbdf -f fonts/newui/08-Tiny5Mono.bdf     # writes 08-Tiny5Mono.fnt
```

`tools/convttf` reads a TrueType font with FreeType (needs libfreetype-dev) and
is what `fonts/newui/mkfonts.sh` uses for the CJK faces.

Where FreeType is not available, **`tools/ttf2bdf.py`** rasterises a TrueType
font with the stdlib only: it reads `glyf`/`loca`/`cmap`, decodes the outline
contours, fills them at 4× the target size (so thin features survive) and
writes X11 BDF, which `convbdf` then rotates into Rockbox's bitmap format. The
seven-segment numeric face comes from it:

```bash
python3 tools/ttf2bdf.py -c '0123456789:-' -p 9 -W 6 -A 9 -D 0 \
    -o fonts/newui/09-Digital7.bdf fonts/newui/digital-7-mono.ttf
/tmp/convbdf -f fonts/newui/09-Digital7.bdf      # -> fonts/newui/09-Digital7.fnt
```

Both are committed, so a build needs no conversion step. The name is the glyph
height in pixels, per the rules in [fonts/README](../fonts/README).

## Marks from the logo

The clef that stands for an album with no cover and the boot splash are drawn,
not blitted. `tools/svg2poly.py` (stdlib only, like `ttf2bdf.py`) flattens the
SVG paths in `docs/logo` into polygons, normalises them into a 1024 box and
prints the geometry the tree ships:

```bash
python3 tools/svg2poly.py docs/logo/rockbox-clef.svg --paths path24 \
    > apps/newui/clef_geometry.h
python3 tools/svg2poly.py docs/logo/rockbox-icon.svg \
    > apps/newui/logo_geometry.h
python3 tools/svg2poly.py docs/logo/rockbox-icon.svg --preview 80   # check it
```

`apps/newui/ui_mark.c` fills each path even-odd (a ring inside another is a
hole), paths in the order the drawing has them, sampling every pixel row on
four sub-scanlines for anti-aliasing. Regenerating a header is a one-command
change; no screen code is touched.

## The commit gate

A change is good when the fresh builds pass — firmware, simulator and warble —
with no new warnings. For pure cleanups, `mipsel-elf-size rockbox.elf` and the
`.codec` sizes must be unchanged; only the version string may differ. Every
screen is checked in the simulator screenshots before it goes on the device;
that is how `docs/screens/` is produced.

## Releases and CI

Pushing a tag `vX.Y.Z` (or running it from the Actions tab with a tag input)
runs `.github/workflows/release.yml`: it builds `--type=n`, renames
`rockbox.zip` to `rockbox-h2-vX.Y.Z.zip` and uploads it to the tag's release,
creating the release if the tag has none. The description comes from
**`docs/releases/<tag>.md`** — one notes file per tag, listed in
[RELEASE_NOTES.md](RELEASE_NOTES.md). A description typed by hand is left alone;
the workflow only rewrites a body that is empty or that it wrote itself. Write
the notes before tagging, since the workflow checks out the tag. The release
build runs `VERSION="$TAG" make`, so `rbversion.h` — and the version the player
shows — is the tag rather than `rNNNNNNNNNN-YYMMDD`.

`.github/workflows/ci.yml` builds firmware, simulator and warble on every push
to `master`; the simulator smoke test is `ci/smoke.script`, and its screenshots
are uploaded as the `sim-screenshots` artifact.
