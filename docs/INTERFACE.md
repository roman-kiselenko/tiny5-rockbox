# The interface

A summary of what is on screen and how it is driven. The design in full — the
grid, colours, fonts, rows, motion and every screen — is in
[newui/PLAN.md](newui/PLAN.md).

Everything on screen is drawn by hand in C (`apps/newui/`): Home, Now Playing,
Cover Flow, the file and database browsers, playlists, the settings menus, the
System screens, the lock screen, pop-ups, the on-screen keyboard, the USB
screen, the status bar and the boot animation. It is a text-first layout in a
9 px monospace font: 16 px rows, a 22 px status bar in the page's own colour,
nothing but the covers as graphics, and no rules or borders — bands are
separated by colour only, the way a terminal does it.

## Screens

- **Home** is the root screen: Now Playing (or resume), Music (the tag
  database), Cover Flow, Files, Playlists, Bluetooth, Settings, System.
- **Cover Flow**: albums as covers in a row, the centre one facing you and the
  others turned towards it with a reflection, animated by the wheel. Centre
  opens the album; a song starts the album from it. Music keeps playing while
  you browse.
- **Album art in lists**: rows with covers are two lines (name plus artist or
  album). Covers are decoded once and kept as pre-scaled thumbnails in
  `.rockbox/newui/thumbs`, with a small RAM cache — JPEG decoding on the fly is
  too slow on the X1000.
- **Now Playing**: the track as text (title, artist, album, "3 of 12 · shuffle",
  codec + bitrate + sample rate), the cover in the corner, and one progress
  line at the bottom. Changing the volume shows a volume band in place of the
  progress for a moment. Press Centre twice (or hold it) to scrub with the
  wheel.
- **Scrolling**: the wheel speeds up as you spin it, and a large letter shows
  while fast-scrolling a list.
- **Looks**: Dark, Light, Nord, Gotham, Rose Pine, Tokyo Night, One Light or
  GitHub Light theme (the same layout, `Settings › General › Display`), Classic
  or Minimal selection, and an optional status bar clock. The accent is part of
  the theme, not a separate choice. The status bar shows
  the play state, clock, Bluetooth and the battery as a number (red under 20%,
  with a `+` while charging).

## Controls

| Key | Lists and menus | Now Playing |
|---|---|---|
| Wheel | Move the selection (faster the harder you spin) | Volume |
| Centre (PLAY) | Open the selection | Play/pause; a second press, or a hold, scrubs with the wheel |
| Prev / Next | One item | Previous / next track; held, seek |
| Back | Up one level; held, jump to Now Playing | Back to where you came from |
| Menu | The menu | The on-play menu |
| Vol+ / Vol− | Volume | Volume |
| Power | Locks the keys and turns the display off, from any screen; one press wakes it | the same |

## Fonts and languages

The UI is **Tiny5 Mono** (9 px tall, SIL OFL 1.1) —
[Gissio/font_Tiny5Mono](https://github.com/Gissio/font_Tiny5Mono) — so a row is
16 px and the readout screens fit the panel. It carries Latin, Greek, Cyrillic
and IPA, so Russian and other Cyrillic text display, using Rockbox's normal
language files and codepages. (The v1.1 build of the base repo still draws
everything in Noto Sans CJK, the UI font before the switch to Tiny5.)

The face is committed as `.fnt` (`fonts/newui/08-Tiny5Mono.fnt`, converted from
`08-Tiny5Mono.bdf` with `convbdf`), so a normal build needs no FreeType.

**Times and counts use seven-segment numerals.** The `UI_FONT_NUM` slot is
**Digital-7** (`fonts/newui/09-Digital7.fnt`, 9 px tall, 6 px advance, digits
`0-9`, `:` and `-` only), and `ui_text_num()` draws runs of those characters
with it while everything around them stays Tiny5: the status bar clock and
battery number, Now Playing's elapsed and remaining times, Cover Flow's track
lengths, the lock-screen clock. Both faces are 9 px on the same baseline, so a
mixed string like `37%` needs no special alignment.

**CJK is not supported yet.** The Noto Sans CJK JP faces (14/18/20/22/40 px,
SIL OFL 1.1) from an earlier iteration of the UI are no longer in the tree —
they were 27 MB on the card and nothing used them. `fonts/newui/mkfonts.sh`
still generates them from Noto Sans CJK with `tools/convttf` (needs
libfreetype-dev) as the starting point for a Chinese/Japanese/Korean build.
Fonts accepted in this tree follow the rules in [fonts/README](../fonts/README).
