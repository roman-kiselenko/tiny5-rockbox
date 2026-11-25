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

Settings in `simdisk/.rockbox/config.cfg` are read exactly as on the player,
by their config names: `theme: nord`, `selection style: minimal`.

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
creating the release if the tag has none. The text of `docs/RELEASE_NOTES.md`
becomes the release description.

`.github/workflows/ci.yml` builds firmware, simulator and warble on every push
to `master`; the simulator smoke test is `ci/smoke.script`, and its screenshots
are uploaded as the `sim-screenshots` artifact.
