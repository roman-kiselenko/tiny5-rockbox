# HiFi Walker H2 Rockbox — `tiny5-rockbox`

A stripped-down Rockbox tree for one player, the **HiFi Walker H2** (a rebrand
of the AIGO Eros Q native; Ingenic X1000, MIPS), with a code-drawn UI and
Bluetooth audio. The UI overhaul is **built**, not planned.

Repository: https://github.com/roman-kiselenko/tiny5-rockbox (branch `master`).
It is based on https://github.com/dismal-ad/h2-rockbox, the original H2 port
(forked from upstream Rockbox at commit `212bb63ee4`, 145 commits of history).
The published release v1.1 of the base repo is 12 commits behind `master`: it
still uses the Noto Sans CJK UI font and has no System Status / Battery Usage
screens.

## Device

- Target `erosqnative` (configure target 247), one firmware for all hardware
  revisions; the firmware reads the revision from the bootloader at runtime.
  The owner's unit is **hw4**.
- The official Rockbox bootloader is used. **This tree cannot build bootloaders.**
- LCD 320×240, 16 bpp colour. 32 MB RAM. 2 MiB plugin buffer (used as scratch
  memory by core code). No touchscreen.
- Buttons: POWER, MENU, BACK, PLAY, PREV/NEXT, VOL_UP/VOL_DOWN, and a scroll
  wheel (`BUTTON_SCROLL_BACK`/`BUTTON_SCROLL_FWD`). Keymap: `apps/keymaps/keymap-erosq.c`.
- Power locks the keys and blanks the display from any screen (`action.c`
  softlock → `apps/newui/scr_lock.c`); one press wakes it after the backlight
  timeout, held Power shuts the player off.

## Build

Toolchain (MIPS cross-compiler) is installed in `~/rockbox-toolchain`.

```bash
export PATH=$HOME/rockbox-toolchain/bin:$PATH

# firmware (outside the source tree, e.g. ../build-erosq)
../src/tools/configure --target=erosqnative --type=n && make -j8 && make zip

# simulator (../build-sim); its virtual SD card is simdisk/ (holds test music)
../src/tools/configure --target=erosqnative --type=s && make -j8 && make install && ./rockboxui
```

Other build types still supported: `--type=w` (warble codec test tool),
`--type=a<opts>` advanced builds (`l` logf, `i` logf to serial, `d` debug,
`u` USB serial, `m` small C lib, `x` LTO). Profiling (`p`) does not build on
MIPS (also broken upstream).

The UI's font is committed as `.fnt` (`fonts/newui/08-Tiny5Mono.fnt`, from
`08-Tiny5Mono.bdf` via `convbdf`), so a normal build needs no FreeType. Times
and counts use a second committed face, `fonts/newui/09-Digital7.fnt` (Digital-7,
seven-segment, 208 bytes), generated from `digital-7-mono.ttf` by the stdlib-only
`tools/ttf2bdf.py` and then `convbdf`; `ui_text_num()` in `apps/newui/ui_draw.c`
picks it for runs of `0-9`, `:` and `-`. The Noto CJK faces are not committed;
`make -C tools convttf` (needs libfreetype-dev) plus `fonts/newui/mkfonts.sh`
generates them when a CJK build is started.

Simulator keys: arrows = scroll wheel / prev / next, Enter = play/select,
Backspace = back, Insert = menu, PgUp/PgDn = volume, Esc = power.

Install on the player: unzip `rockbox.zip` to the SD card root (it contains
`.rockbox/`). The owner's card keeps settings and Bluetooth pairings in
`.rockbox/` (`config.cfg`, `bt_config.conf`, `bt_devices.txt`): unzip over the
existing folder (`unzip -o`) and check those files are unchanged. The archive
is ~2.6 MB: firmware, codecs, Tiny5 Mono (452 KB) and the Digital-7 numerals
(208 B). An older install still
has `.rockbox/fonts/newui-*.fnt` on the card (the Noto CJK faces, 27 MB) — they
are no longer installed and can be deleted.

## Verifying changes

A change is good when the fresh builds pass: firmware, simulator, warble
(plus a logf build for debug-path changes). `../newui-tools/check.sh` is the
commit gate (firmware zip + simulator install, no new warnings);
`../newui-tools/simrun.sh <script> <shotdir>` runs the simulator headless from
a key script and writes screenshots (see `uisimulator/common/sim_script.c`),
and every screen is checked there before it goes on the device; `ci/screens.sh`
(+ `ci/screens.script`, `ci/scaleimg.py`) regenerates `docs/screens/` — Home and
Now Playing, dark and light, upscaled 4×; `THEMES` for other palettes; `../newui-tools/loc.py [commit]` counts code lines. For pure cleanups,
`mipsel-elf-size rockbox.elf` and the `.codec` sizes must be unchanged. Only the version string may differ. When running the script
runner by hand, create the shot directory first: `RB_SHOTDIR` is written to
with host `fopen`, and the sim just prints "cannot write" if it is missing.

## What was removed from upstream (and must not come back by accident)

- All other targets, CPUs (ARM, ColdFire, other Ingenic), their drivers, configs
  and keymaps; bootloader code and tools; manual, android, utils.
- Dead preprocessor branches for features the H2 lacks (touchscreen, remote
  LCD, recording, FM radio, S/PDIF, line-in/mic, button lights, iPod accessory...).
- Codecs: game music (SID, NSF, SPC, SAP/ASAP, AY, VTX, GBS, HES, SGC, VGM,
  KSS), MOD, RealMedia, ATRAC3, AC3, Musepack, TTA, WMA Voice, Speex, ADX.
  Kept: MP1/2/3, AAC/HE-AAC, ALAC, FLAC, Vorbis, Opus, WAV/W64/AIFF, APE,
  WavPack, WMA/WMA Pro, Shorten, DSD, AU, VOX, SMAF.
- Voice (spoken menus): `apps/talk.h` and `apps/voice_thread.h` are **inline
  no-op stubs**; call sites remain and can be deleted as screens are rewritten.
- Plugins: no plugins, no loader and no menu entries for them;
  `plugin_get_buffer()` (apps/plugin.c) still returns the 2 MiB scratch buffer.
- The theme (skin) engine and everything around it (phase 8 of the new UI,
  done): `apps/gui/skin_engine/`, `lib/skin_parser/`, the skinned status bar and
  lists, Theme Settings and the theme settings, the quick screen (its button
  opens Shortcuts), `tools/checkwps` and the CheckWPS build type, `wps/`,
  `backdrops/`, bitmap fonts except the UI font and the system font, and
  playback's album art buffering for themes.

Code size: about **296k** lines of core code (`apps`, `firmware`, `lib`,
`uisimulator`, Bluedroid excluded) against roughly 1.6M upstream.

## Bluetooth

Ported from https://github.com/thiagoelg/h2-rockbox-bt (branch `h2-bt`, git
remote `h2bt`): the 19 code commits from `4e3333fd06` (Skye's Bluedroid port)
through `bde005189c` (Thiago Lugli's manager/menu/settings), cherry-picked with
`-x`. Their README, GitHub workflow and manual commits were not taken.

- Bluedroid (ESP-IDF 6.0.1 Bluetooth stack) in `lib/bluedroid/`: A2DP source
  with SBC and AVRCP — headphone play/pause, next and previous work. Host side
  only: the H2's Broadcom BCM4343A1 is driven over UART (HCI H4) by
  `firmware/target/mips/ingenic_x1000/erosqnative/hci-erosqnative.c`, with its
  patchram firmware in `BCM4343A1_*.c`. **hw4 only.**
- `HAVE_BLUETOOTH` and `BT_HEAP_SIZE` (256 KiB, TLSF heap) are in
  `config/erosqnative.h`. Audio goes out through a second PCM sink,
  `PCM_SINK_BLUETOOTH`.
- App side: `apps/bluetooth/bluetooth.c` (manager thread, paired devices in
  `/.rockbox/bt_devices.txt`), reached from **Home**, not from the settings menu
  (`apps/menus/bluetooth_menu.c`). Settings: `bt_enabled` (default off),
  `bt_high_quality`, `bt_volume_sync`. With `BT_DEBUG` it logs to `/bt_log.txt`.
- Simulator: `uisimulator/common/hci-sim.c` talks to a host serial HCI device.
- Porting notes: `uart_rx_timeout()` was re-applied by hand on top of
  upstream's DMA UART TX driver (`af9a81f5f6`), which their branch predates.
  That driver keeps one set of state for all UARTs, so only one UART can be in
  use at a time.
- Bluedroid was carved to this configuration (commit "bluedroid: remove code
  this configuration never builds"): conditionals that are constant across the
  firmware, simulator and `BT_DEBUG` builds were resolved and 159 unused files
  deleted, 202k -> 99k code lines with identical object code. Its six config
  headers (`sdkconfig.h`, `bt_user_config.h`, `bluedroid_user_config.h`,
  `bt_target.h`, `dyn_mem.h`, `bt_common.h`) were left as is; to enable a
  disabled Bluedroid feature, restore the code from before that commit.
- Cost: firmware text +425 KB. Tested on the owner's H2 (hw4): Bluetooth audio
  works.

## Battery and power work

See `docs/newui/POWER.md` for the plan, measurements and what is left.
Done: no drawing while the screen is off; the LCD waits for the end of a frame
on interrupts instead of polling; the Broadcom controller's sleep mode is
enabled (BT_WAKE asserted only while sending); the DAC and headphone amp are
off when playing over Bluetooth. X1000 CPU-idle statistics are always built in
(System › Debug › CPU idle) and there is a temporary 1008 / 252 MHz switch in
the target debug menu. Not done: CPU frequency scaling (the biggest remaining
gain).

Battery usage is logged (a point every 5%, sampled every 30 s) to
`.rockbox/newui/battery.txt` and shown as a chart in System › View Battery
Usage (`apps/newui/scr_battery.c`).

## Releases

Pushing a tag `vX.Y.Z` (or running it from the Actions tab with a tag input)
triggers `.github/workflows/release.yml`: it builds `--type=n`, renames
`rockbox.zip` to **`rockbox-vX.Y.Z.zip`** (the `h2` prefix was dropped from
v0.0.3), and uploads it to the tag's release, creating the release if the tag
has none. **Each tag has its own notes
file**, `docs/releases/<tag>.md`, which becomes that release's description;
`docs/RELEASE_NOTES.md` indexes them. Write the notes before the tag (the
workflow checks out the tag). The next release is **v0.0.3**. The splash prints two
versions: the Rockbox build revision (`RBVERSION`) and, in grey, the repository
tag as `Tiny5 UI: vX.Y.Z`. `tools/version.sh --release` resolves the tag (HEAD's
tag, or the release the RELEASE_NOTES table says is next) and `genversion.sh`
writes it to `rbversion.h` as `RBRELEASE`; the release workflow no longer
overrides `VERSION`, so the two lines stay distinct.
`.github/workflows/ci.yml` builds firmware + simulator + warble on every push
to `master`; the simulator smoke test is `ci/smoke.script`, and its screenshots
are uploaded as the `sim-screenshots` artifact.

## Tools

`../newui-tools/` (outside the repo): `check.sh`, `simrun.sh`, `loc.py`,
`ifeval.py` (per-build #if evaluation), `macrovals.py` (resolve macro values per
build), `prune.py` (find unused files). Their docstrings explain usage.

## The UI: decisions

The design, as built — grid, colours, fonts, rows, motion, every screen, and
how it differs from the original plan — is in `docs/newui/PLAN.md`.

1. **Code-drawn UI, no themes.** The UI is drawn directly in C
   (`apps/newui/`); the theme engine is gone.
2. **Not an iPod-style design.** The iPod classic and the Q2 Pod spec were the
   starting reference and are no longer followed: text-first screens, one
   monospace font, flat selection bands, drawn shapes instead of bitmaps. Cover
   Flow stayed because it works, not because the iPod had it. Some comments in
   `apps/newui/` still say "as on the iPod" — they are leftovers.
3. **Font: Tiny5 Mono** (9 px, SIL OFL 1.1,
   https://github.com/Gissio/font_Tiny5Mono) for every UI slot; rows are 16 px,
   art rows 48 px, two-line rows 24 px, status bar 22 px. Latin, Greek,
   Cyrillic and IPA only. `UI_FONT_NUM` is the exception: Digital-7 seven-segment
   numerals, digits/`:`/`-` only, for times and counts.
4. **CJK is deferred, not dropped.** Tiny5 Mono and the Digital-7 numerals are
   the only fonts installed. `fonts/newui/mkfonts.sh` (with `tools/convttf`) is kept
   as the seed for a Chinese/Japanese/Korean build; the Noto faces themselves are
   no longer in the tree.
5. **Cover Flow over the music database.**
   - Database: `apps/tagcache.c/.h` (on-disk format, `TAGCACHE_MAGIC`, files
     `database_*.tcd`), `apps/tagtree.c` + `apps/tagnavi.config` (browse menus).
     Any on-disk format change **must bump `TAGCACHE_MAGIC`** so old databases
     are rebuilt instead of misread.
   - Album art: `apps/recorder/albumart.c` (finds cover files),
     `apps/recorder/jpeg_load.c`, `bmp.c`, `resize.c` (decode + scale).
     `HAVE_ALBUMART` and `HAVE_JPEG` are enabled.
   - `jpeg_load.c` reads baseline and progressive JPEGs. A progressive one is
     decoded scan by scan into a 2 MiB static coefficient buffer
     (`JPEG_PROG_BUF_SIZE`), keeping only the coefficients the downscaling
     IDCT reads and the per-block nonzero mask the refinement scans need, in
     stripes of MCU rows — at most `JPEG_PROG_MAX_PASSES` (4) passes over the
     file. Covers too big for that buffer fail with -13 and show the
     placeholder.
   - The "no art" mark is the **Rockbox clef**, not a bitmap:
     `tools/svg2poly.py` flattens `docs/logo/rockbox-clef.svg` into the
     committed `apps/newui/clef_geometry.h`, and `apps/newui/ui_mark.c`
     (`ui_clef_draw`, `ui_clef_fill`) fills it even-odd, anti-aliased by
     sampling each pixel row on four sub-scanlines, with the coverage cached
     per size so list rows redraw cheaply. The same file draws the boot
     splash from `docs/logo/rockbox-icon.svg` (`logo_geometry.h`,
     `ui_logo_draw`): the icon at half the screen height in the bottom-left
     corner, faded in over 2 s with two rows above it: `Rockbox:` the build
     revision and, in grey, `Tiny5 UI:` the repository tag — the 230 KB boot
     bitmap is gone.
   - Decoding JPEGs while scrolling is too slow on the X1000: covers are
     decoded once and saved pre-scaled on the card
     (`/.rockbox/newui/thumbs`, 48 px for list rows = `UI_THUMB`, 150 px for
     Cover Flow = `UI_COVER`, or a one-byte "no art" marker holding
     `NOART_VERSION`), with a RAM cache of 24 thumbs and 16 covers
     (`apps/newui/ui_thumb.c`). `NOART_VERSION` is bumped whenever the
     decoders learn to read more covers, so the ones they could not read are
     tried again. Background cover building pauses when the screen is off or
     music is playing.
6. **Look settings** (`Settings › General › Display`): `newui_theme`
   (dark, light, nord, gotham, rose-pine, tokyo-night, one-light, github-light),
   `newui_style` (classic accent band / minimal band), `newui_clock`. The
   accent is derived from the theme, there is no accent setting. Plus
   `skip_startup_anim`.
7. **Home carries the collection.** Along its bottom edge, in the theme's
   secondary grey, the tag database's size: `1246 tracks · 41 h 12 min`. The
   total of `tag_length` is summed once and cached in `apps/newui/scr_home.c`,
   keyed on the database's entry count, scan progress and commit step, and Home
   redraws the line when it changes; while the database is committing the line
   says so instead. Pop-up progress bars are flat bands, not pills, and the
   yes/no rows use the same flat selection band as every list.
8. Everything is built and checked in the simulator at 320×240 before the
   device, driven by the scroll wheel.

## Where the UI code lives

- `apps/newui/`: the new UI, `ui.h` being the design tokens and API.
  - screens: `scr_home.c`, `scr_np.c` (Now Playing, driven by `gui/wps.c`),
    `scr_coverflow.c` (+ its album sheet), `scr_stats.c` (System Status),
    `scr_battery.c` (Battery Usage chart), `scr_lock.c`
  - widgets: `ui_status.c` (status bar), `ui_popup.c` (panels, yes/no, choices),
    `ui_keyboard.c` (text entry), `ui_gallery.c` (debug-menu UI gallery)
  - drawing and behaviour: `ui_draw.c` (rects, gradients, rounded corners,
    alpha, text with ellipsis, the drill chevron), `ui_style.c` (themes,
    selection styles, accents), `ui_font.c`, `ui_art.c` (current album cover),
    `ui_mark.c` (the Rockbox clef, the mark of an album with no cover, and the
    boot icon),
    `ui_thumb.c` (cached covers), `ui_wheel.c` (wheel ramp, fast-scroll letter),
    `ui_transition.c` (slide between list levels), `ui_time.c` (µs frame timer)
- `apps/root_menu.c`: screen switching under Home and the start screen;
  `apps/main.c`: the boot animation
- `apps/menus/`, `apps/menu.c`: settings menus
- `apps/gui/list.c` + `apps/gui/bitmap/list.c`: every list, restyled in one place
- `apps/gui/viewport.c`: the status bar / full-screen stack;
  `apps/gui/statusbar.c`: feeds it
- `apps/tree.c`, `apps/filetree.c`: file browser; `apps/tagtree.c`: database browser
- `apps/gui/wps.c`: Now Playing's logic (drawn by `newui/scr_np.c`);
  `apps/onplay.c`: context menu
- `apps/settings_list.c`: all settings definitions
- `firmware/drivers/lcd-16bit*.c`, `firmware/export/lcd.h`: LCD drawing API

## References

- `docs/newui/PLAN.md` — the design as built, with the deltas from the original
  iPod-era draft.
- `docs/newui/POWER.md` — battery review, measurements, remaining steps.
- `docs/INTERFACE.md` (screens, controls, fonts), `docs/BLUETOOTH.md` (the
  Bluedroid port and driver), `docs/CUT-DOWN.md` (what was removed from
  upstream), `docs/DEVELOPING.md` (build types, simulator testing, releases).
  The README is a short introduction that links to these.
- `docs/README` — build and install in detail; `fonts/README` — the rules for
  fonts in this tree; `docs/UISIMULATOR` is upstream's generic simulator doc.
- **`tutorial.md`** — a long Rockbox API tutorial (LCD drawing, drawmodes,
  text, buttons and tick timing, menus, yes/no, keyboard input, file I/O,
  `configfile`, battery/backlight/RTC, the quick-reference card at the end).
  Written for iPod Video **plugins**, so the calls appear as `rb->lcd_draw()`
  etc.; in this tree the same APIs are the core ones — `lcd_draw()`,
  `lcd_set_foreground()`, `font_getstringsize()`, `button_get_timeout()`,
  `HZ` ticks — with no plugin wrapper. Use it as a reference for Rockbox API
  signatures and idioms when writing UI or screen code; there is no plugin
  loader here, so nothing in it can actually run.
