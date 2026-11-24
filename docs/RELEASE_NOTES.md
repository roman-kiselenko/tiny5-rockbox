# H2 Rockbox v0.0.1 — the Tiny5 Mono interface

Rockbox for the **HiFi Walker H2** (AIGO Eros Q native, Ingenic X1000, 320×240):
a UI drawn by hand in C, album covers throughout, and Bluetooth audio. This is
the first packaged build of the `tiny5-rockbox` tree.

**Archive:** `rockbox-h2-v0.0.1.zip` — unzip to the root of the SD card (it
contains `.rockbox/`).

## What's in it

- **A code-drawn interface** (`apps/newui/`): Home, Now Playing, Cover Flow,
  file and music-database browsers, playlists, settings menus, System screens,
  pop-ups, on-screen keyboard, lock screen, USB screen, boot animation. No
  theme/skin engine, no WPS files, no bitmap icons.
- **Tiny5 Mono** (9 px, SIL OFL 1.1) for the whole UI: 16 px rows, a 22 px
  status bar. Latin, Greek and Cyrillic — Russian and other Cyrillic tags
  display correctly. Chinese/Japanese/Korean is not supported yet.
- **Cover Flow**: albums as covers that slide with the scroll wheel, with
  reflections and perspective. Covers are decoded once and cached on the card
  (`.rockbox/newui/thumbs`) — decoding JPEGs while scrolling is too slow on the
  X1000.
- **Album art in lists** and on Now Playing: title, artist, album, "3 of 12",
  codec · bitrate · sample rate, progress band. The wheel sets the volume;
  Centre twice (or held) scrubs.
- **Looks**: Dark or Light theme, Classic or Minimal selection, six accent
  colours, optional status bar clock.
- **Bluetooth audio** (A2DP + AVRCP): paired headphones play/pause, skip and
  change volume. Hardware revision **4 (hw4)** only.
- **Battery and power**: nothing is drawn while the screen is off, the LCD
  sleeps instead of polling, the Bluetooth chip sleeps between packets, the DAC
  and headphone amp are off when streaming over Bluetooth. System ›
  **System Status** (CPU, memory, storage, I/O, database, uptime) and System ›
  **View Battery Usage** (level against hours since the last charge).
- Power locks the keys and blanks the display from any screen; press again to
  wake it.

## Install

1. The player needs the **official Rockbox bootloader** for the Eros Q native
   (Rockbox Utility, or a stock Rockbox build once). This tree does not build
   bootloaders.
2. Unzip to the SD card root. To update in place and keep your settings and
   pairings: `unzip -o rockbox-h2-v0.0.1.zip -d /path/to/sdcard` — `config.cfg`,
   `bt_config.conf` and `bt_devices.txt` are not in the archive.
3. Eject the card and reboot.

Most of the 15 MB is fonts. The interface only needs Tiny5 Mono (452 KB); the
Noto Sans CJK faces (~27 MB on the card) are kept for a future CJK build and can
be deleted from `.rockbox/fonts/`.

## Build it yourself

```bash
sudo apt install build-essential git perl zip unzip wget flex bison texinfo \
                 autoconf automake libtool libtool-bin
git clone https://github.com/roman-kiselenko/tiny5-rockbox.git
cd tiny5-rockbox
RBDEV_PREFIX=$HOME/rockbox-toolchain RBDEV_TARGET=i tools/rockboxdev.sh   # once
export PATH=$HOME/rockbox-toolchain/bin:$PATH
mkdir build && cd build
../tools/configure --target=erosqnative --type=n
make -j$(nproc) && make zip
```

## Known limits

- CJK text is not rendered yet.
- Bluetooth works on hw4 only; other revisions still run the firmware.
- No plugins (the loader is removed), no themes, and codecs this player will not
  use were dropped (MOD, game music, RealMedia, AC3, Speex...).
- New UI strings are English only; other languages fall back to English.

## Credits

Based on [dismal-ad/h2-rockbox](https://github.com/dismal-ad/h2-rockbox), a
cut-down fork of upstream [Rockbox](https://www.rockbox.org/) (commit
`212bb63ee4`). Bluetooth from
[thiagoelg/h2-rockbox-bt](https://github.com/thiagoelg/h2-rockbox-bt) (Thiago
Lugli, on Skye's Bluedroid port). Font:
[Tiny5 Mono](https://github.com/Gissio/font_Tiny5Mono), SIL OFL 1.1. GPL v2, like
Rockbox.
