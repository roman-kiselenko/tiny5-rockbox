# New UI: design notes

**Status: built.** Everything below describes the UI as it is on the device.
The plan started on 2026-10-15 as an iPod classic homage — the same 320×240
panel, Cover Flow, the silver status bar. **We no longer follow the iPod's
design.** What came out is its own interface: text-first, one monospace font,
flat bands for selection, drawn shapes instead of bitmaps, and album covers as
the only images. Where old comments in `apps/newui/` still say "as on the
iPod", they are leftovers from the first draft, not a design rule. The Q2 Pod
spec was dropped with it.

Deltas from the original plan, in one place:

| Draft (2026-10) | Now |
|---|---|
| Anti-aliased sans-serif (Inter / Source Sans 3 / Nimbus Sans) | **Tiny5 Mono**, 9 px, one face for the whole UI |
| Text rows 27 px, art rows 54 px | **16 px** rows, **48 px** art rows, 24 px two-line rows |
| Rounded selection card with 6 px corners (Minimal) | Flat band, no corners, no gloss |
| Home: menu on the left, album cover on the right | Full-width text list; the cover panel was removed (CPU cost) |
| Now Playing: big art + text column, page dots | Console-style readout: text lines, cover in the corner, one progress band |
| Battery as an icon | Battery as a number, `37%`, red under 20%, `+` while charging |
| Quick screen | Removed; its button opens Shortcuts |
| Silver gradient status bar with a hairline | The status bar is the page's own colour: 22 px of the background, only its glyphs differ |
| CJK text via Noto Sans CJK | Deferred: the faces are no longer installed, only `fonts/newui/mkfonts.sh` remains to make them |

---

## 1. Approach

**The new UI is part of the core, not plugins.** The plugin loader is gone from
this tree; putting the main screens behind a plugin API boundary would separate
them from the playback, database, settings and list code they constantly touch.
New code lives in `apps/newui/`.

**Existing menus are restyled, not rewritten.** `apps/gui/list.c` and
`apps/gui/bitmap/list.c` draw every list — the file browser, the database
browser, playlists, every settings menu — in the new style. Only Home, Now
Playing, Cover Flow, the System screens and the pop-ups have their own code.

**The theme engine is gone** (phase 8, done): `apps/gui/skin_engine/`,
`lib/skin_parser/`, `wps/`, backdrops, Theme Settings, CheckWPS. Now Playing's
*logic* still lives in `apps/gui/wps.c`; its *drawing* is `apps/newui/scr_np.c`.
Voice (`apps/talk.h`, `apps/voice_thread.h`) is a no-op stub.

**Test aids instead of plugins:**

| Aid | What it is | Where |
|---|---|---|
| UI gallery | Debug-menu screen ("New UI gallery") drawing each widget and screen with fake data | Simulator and device |
| Simulator script runner | `RB_SCRIPT` key script, `RB_SHOTDIR` screenshots — scripted presses, PNG per step | Simulator, headless with `SDL_VIDEODRIVER=dummy` |
| Frame timer | `ui_time.c`, a µs counter for measuring frame cost | Device |

---

## 2. Design system (320×240)

### Screen grid

```
y   0 ┌────────────────────────────────────────┐
      │ H2                12:41      BT  37%  │  status bar, 22 px
y  22 ├────────────────────────────────────────┤
      │                                        │
      │          content area 320 × 218        │
      │                                        │
y 240 └────────────────────────────────────────┘
```

- Side margin **12 px**, gap between elements **8 px**.
- Background is black in the Dark theme, white in the Light theme, deep
  blue-black in Nord, navy in Gotham, plum purple in Rose Pine, midnight blue in
  Tokyo Night, off-white in One Light and GitHub Light.
- The screen has square corners; nothing is inset.

### Colours

All eight themes share the layout; only the palette changes (`ui_style.c`).
Each theme carries its own accent — there is no accent setting: Dark and Light
use graphite, the rest their selection-background.

| Use | Dark | Light | Nord | Gotham | Rose Pine | Tokyo Night | One Light | GitHub Light |
|---|---|---|---|---|---|---|---|---|
| Background | `#000000` | `#FFFFFF` | `#0B0E14` | `#0C1014` | `#1F1D2E` | `#1A1B26` | `#F8F8F8` | `#F6F8FA` |
| Primary text | `#FFFFFF` | `#141414` | `#BFBDB6` | `#99D1CE` | `#E0DEF4` | `#C0CAF5` | `#2A2B33` | `#1F2328` |
| Secondary text | `#AAAAAA` | `#5E5E5E` | `#9B9B93` | `#6FA6A3` | `#B0ADC0` | `#A9B1D6` | `#6B6C75` | `#57606A` |
| Muted text | `#8C8C8C` | `#8C8C8C` | `#686868` | `#4E5166` | `#908CAA` | `#414868` | `#BBBBBB` | `#6E7781` |
| Selection (Minimal) | `#EEEEEC` / `#171717` | `#D6D6D6` / `#141414` | `#1E232B` / `#BFBDB6` | `#195466` / `#99D1CE` | `#26233A` / `#E0DEF4` | `#414868` / `#C0CAF5` | `#BBBBBB` / `#2A2B33` | `#D0D7DE` / `#1F2328` |
| Selection (Classic) | `#6E6E6E` | `#6E6E6E` | `#1B3A5B` | `#0A3749` | `#2F2C40` | `#283457` | `#2A2B33` | `#264F78` |
| Text on the band | white | white | `#BFBDB6`; dark on a bright accent | `#99D1CE` | `#E0DEF4` | `#C0CAF5` | `#F8F8F8` | `#F6F8FA` |
| Accent as a fill (progress, plots, icons) | `#6E6E6E` | `#6E6E6E` | `#76889C` | `#6C8791` | `#82808C` | `#7E859A` | `#2A2B33` | `#264F78` |
| Progress track / fill | `#1C1C1C` / `#EEEEEC` | `#DCDCDC` / `#6E6E6E` | `#141920` / `#BFBDB6` | `#14181F` / `#99D1CE` | `#2A2740` / `#E0DEF4` | `#232733` / `#C0CAF5` | `#DEDEE3` / `#2A2B33` | `#D0D7DE` / `#264F78` |
| Progress track | `#444448` | `#8C8C90` | `#39424D` | `#2B3841` | `#453F5C` | `#414868` | `#8A8B93` | `#D0D7DE` |
| Pop-up panel | `#262626` | `#F2F2F2` | `#1E232B` | `#14181F` | `#26233A` | `#15161E` | `#F2F2F4` | `#FFFFFF` |
| Cover placeholder | `#1C1C1C` / `#8C8C8C` | `#5C5C62` / `#D4D4D8` | `#1E232B` / `#686868` | `#14181F` / `#4E5166` | `#26233A` / `#908CAA` | `#15161E` / `#414868` | `#D5D6DD` / `#8B8C94` | `#D0D7DE` / `#6E7781` |
| Battery warning red | `#F4624C` | `#F4624C` | `#EA6C73` | `#C23127` | `#EB6F92` | `#F7768E` | `#DE3D35` | `#CF222E` |

Nord is the **Nord** palette (alacet/nord): `#0B0E14` `#BFBDB6` `#1B3A5B`
`#1E232B` `#686868` `#C7C7C7`. The palette's other colours (`#EA6C73`
`#7FD962` `#F9AF4F` `#53BDFA` `#CDA1FA` `#90E1C6`) are not drawn by the UI —
the accent comes from the theme, not from a picker.

Gotham is the **Gotham** palette: `#0C1014` `#99D1CE` `#0A3749` `#195466`
`#4E5166` `#33859E` `#C23127` `#2AA889` `#EDB443`. Its palette has no surface
lighter than the background, so the panels, tracks and the Minimal band use
the palette's blue `#195466` and grey `#4E5166`, with `#14181F` and `#2B3841`
mixed from the background for the softer surfaces.

Rose Pine is the **Rose Pine** palette: `#1F1D2E` `#E0DEF4` `#2F2C40` `#26233A`
`#EB6F92` `#31748F` `#F6C177` `#9CCFD8` `#C4A7E7` `#EBBCBA` `#908CAA`. The UI
draws with background, foreground, selection, surface, muted and blush; rose,
foam, gold, ice and plum stay unused except the plum end of the fast-scroll
letter. `#B0ADC0` and `#453F5C` are mixed from the foreground and the surface.

Tokyo Night is the **Tokyo Night** palette: `#1A1B26` `#C0CAF5` `#283457`
`#15161E` `#F7768E` `#9ECE6A` `#E0AF68` `#7AA2F7` `#BB9AF7` `#7DCFFF` `#A9B1D6`
`#414868`. Its palette 0, `#15161E`, is darker than the background, so panels
and the no-cover placeholder sit recessed rather than raised; `#232733` for the
progress track is mixed from the background. Green, gold and cyan stay unused.

One Light is the **One Light** palette: `#F8F8F8` `#2A2B33` `#BBBBBB` `#DE3D35`
`#3E953A` `#D2B67B` `#2F5AF3` `#A00095`. It is the second light theme: the page
is off-white and the selection is inverted — a band of ink with off-white text.
`#6B6C75` and `#DEDEE3` are mixed from the ink and the background; green, gold
and magenta stay unused.

GitHub Light is the **GitHub Light** palette: `#F6F8FA` `#1F2328` `#264F78`
`#CF222E` `#116329` `#4D2D00` `#0969DA` `#8250DF` `#1B7C83` `#6E7781` `#57606A`
`#8C959F`. Its selection-foreground, `#1F2328`, is too close to its
selection-background `#264F78` to read at 9 px, so text on the accent band takes
the palette's cursor-text `#F6F8FA` instead. The Minimal band, the tracks and the
placeholder use GitHub's border grey `#D0D7DE`, mixed from the background and the
palette's greys; green, orange, purple and teal stay unused.

### Font

One font, **Tiny5 Mono** — [Gissio/font_Tiny5Mono](https://github.com/Gissio/font_Tiny5Mono),
SIL OFL 1.1, 9 px tall, 7 px ascent, 5 px cap height, 6 px per character. `ui_font()` has five slots — small, body, row, title, big — all pointing
at the same face today; the enum stays so the sizes can diverge later without
touching the callers.

The font is installed by `buildzip.pl` from `fonts/newui/`; glyph bitmaps are
loaded from the card into the font cache on demand, so the file size costs disk
space, not RAM. Icons are glyphs, not bitmaps: `⏵ ⏸ ⏹ ⏩ ⏪ ⏺ 🔒`, `BT` for
Bluetooth, `>` for rows that open something.

CJK (Chinese, Japanese, Korean) is not in the UI yet. The Noto Sans CJK JP
faces (14/18/20/22/40 px) that the UI used before Tiny5 Mono are no longer
committed: 27 MB on the card for a script nobody ran. `fonts/newui/mkfonts.sh`
and `tools/convttf` remain as the starting point for a CJK build.

### Rows

| Row | Height | Fits | Contents |
|---|---|---|---|
| Text row (`UI_ROW_H`) | 16 px | 13 in the content area | Label, optional grey value on the right, `>` on rows that open something |
| Two-line row (`UI_LINE2_ROW_H`) | 24 px | 9 | Name, grey second line |
| Art row (`UI_ART_ROW_H`) | 48 px | 4 | A `UI_THUMB` (48 px) cover filling the row, name + grey second line after it |

Selection is one flat band in the selection colour, edge to edge, exactly as
Home draws it — no card, no border, no gloss (`ui_selection()`). Long text is
shortened with `…`; only the selected row's text scrolls. A 3 px scrollbar sits
on the right when a list is longer than the page. Covers down the left edge of a
list touch each other. Nothing in the UI is ruled: rows, bars and panels are
distinguished by their colour only, the way a terminal does it.

### Motion

| Motion | As built |
|---|---|
| Wheel speed-up | Steps closer than 80 ms apart in the same direction count as one spin; after 300 ms of spinning each step moves 2 rows, after 800 ms 4, after 1.5 s 8. Only lists of more than 16 items speed up |
| Fast-scroll letter | While the step is above 1, the selected row's first letter (leading "The ", "A ", "An " skipped) is drawn as a 72 px rounded box in the middle of the list; it disappears 400 ms after the last fast step |
| Page change | Slide left/right: the screen below the status bar is copied before the level changes, then 6 frames slide it away. Skipped if the next draw is more than 500 ms away |
| Cover Flow | 90 ms for the side covers to reach the flow layout, 150 ms for the centre cover to turn to face you; capped at 30 frames/s while music plays |

---

## 3. Controls

| Control | Lists and menus | Now Playing |
|---|---|---|
| Wheel | Move selection (speeds up as you spin) | Volume |
| Centre (`PLAY`) | Open the selection | Play/pause |
| Centre twice, or held | — | Scrub: the wheel seeks, the bar shows a cursor block |
| Prev / Next | One item (held: repeat) | Previous / next track; held, seek |
| Back | Up one level; held, jump to Now Playing | Back to the screen it opened from |
| Menu | The menu; held, the context menu for the selection | The on-play menu (queue, playlist, shuffle, repeat, delete, properties) |
| Vol+ / Vol− | Volume | Volume |
| Power | Lock the keys and turn the display off, from any screen. Press again to wake (one press is enough after the backlight timeout); held, power off |

---

## 4. Screens

Measurements are (x, y, width × height).

### 4.1 Status bar (every screen)

The status bar is 22 px of the page's own background: no gradient, no band, no
rule under it. Only its text and icons are drawn, in the theme's status colour.
Without a title it is: play-state glyphs on the left, the clock centred,
Bluetooth and the battery on the right. Screens that name themselves (Home,
Cover Flow, Now Playing…) put the title on the left and the clock moves to the
centre only while there is room for both, otherwise it tucks in against the
icons. The bar can be narrowed (Home draws it only over its menu). Battery:
`37%`, `+` while charging, red
under 20%, blinking when critically low; the clock is a setting.

### 4.2 Home

The start screen: full-width rows, `UI_ROW_H`, same pitch as every other list.

```
┌────────────────────────────────────────┐
│ H2                12:41      BT  37%  │
├────────────────────────────────────────┤
│ Now Playing                            │   only while a track is loaded
│ Music                                › │   or resumable
│ Cover Flow                           › │
│ Files                                › │
│ Playlists                            › │
│ Bluetooth                            › │
│ Settings                             › │
│ System                               › │
│                                        │
│ 1246 tracks · 41 h 12 min            │   the database, stuck to the bottom
└────────────────────────────────────────┘
```

Along the bottom edge, in the theme's secondary grey, the music database: how
many tracks it holds and their total playing time (`1246 tracks · 41 h 12 min`).
The total is summed from the database
once and cached, rescanned when the database is rescanned or rebuilt, and Home
refreshes the line as a build finishes. While the database is not searchable the
ready yet [3/12]`), and when it holds nothing, `No music in the database`.

Back on Home returns to the first row. Boot: the Rockbox icon fades in over 2 s
(§4.4) into the bottom-left corner with the firmware version above it, holds a
second, and the first screen is then drawn (Settings › General ›
Startup/Shutdown › Skip Startup Animation to skip the fade).

### 4.3 Music (library)

`tagtree.c` + `tagnavi.config` in the new list style: Artists, Albums, Songs,
Genres, Composers, Recently Added, Most Played, Update Database. Albums use art
rows (48 px cover, album, grey artist); an album's tracks are text rows with the
duration on the right. Untagged files fall back to the file name without its
extension. Empty tags show as nothing rather than `""`. Long lists get the
fast-scroll letter.

### 4.4 Cover Flow

Albums from the database as covers in a row, drawn column by column with
perspective texture mapping, a reflection under them, album and artist below.

```
┌────────────────────────────────────────┐
│ Cover Flow            12:41      BT 37%│
│  ╱╲ ╱╲ ┌────────────┐ ╱╲ ╱╲           │   centre 128 px at (96, 20)
│  ╱╲ ╱╲ │  centre    │ ╱╲ ╱╲           │   4 covers each side, the
│  ╱╲ ╱╲ │  cover     │ ╱╲ ╱╲           │   nearest 41 px wide, 29 px
│        └────────────┘                 │   apart, full brightness
│        ░░░ reflection ░░░             │
│        The Dark Side of the Moon      │
│              Pink Floyd               │
└────────────────────────────────────────┘
```

- Centre cover **128 px**, the nearest side cover 41 px wide (its inner edge
  touches the centre cover), then 29 px strips; 4 per side visible.
- Wheel, Prev/Next: one album per step, animated; faster spinning flows the
  covers before they settle.
- Centre opens the album's songs in a panel growing out of the cover
  (26 px margins, a two-line accent header, 16 px rows with durations);
  picking a song starts the album from it. Back returns to the covers.
- Covers are decoded once and saved on the card in `.rockbox/newui/thumbs` as
  raw native pixels: 150 px for Cover Flow, 48 px (`UI_THUMB`) for list rows,
  plus a 1-byte "no art" marker. RAM cache: 24 thumbnails and 16 covers.
- An album with no cover is marked with the **Rockbox clef**. Its outline comes
  from the logo: `tools/svg2poly.py` flattens `docs/logo/rockbox-clef.svg` to
  polygons (200 points in four rings, normalised into a 1024 box) and commits
  them as `apps/newui/clef_geometry.h`; `ui_mark.c` fills them even-odd, the
  glyph taking 75% of the square and centred on its own bounding box, anti-
  aliased by sampling each pixel row on four sub-scanlines, and keeps the
  coverage per size so a list of rows without art redraws cheaply. It replaces
  the hand-drawn quaver; no bitmap is shipped. A cover in a list's *title* row
  sits in the edge column when the rows below have covers of their own (the
  album page), and at the text margin when they do not (the context menu).
- The same rasteriser draws the **boot splash**: `docs/logo/rockbox-icon.svg`
  becomes `apps/newui/logo_geometry.h` (9 painted paths, 17 rings, 570 points,
  each path with its own fill - black, yellow, the grey-blue rock). At half the
  screen height (120 px) it sits in the bottom-left corner and fades in,
  opacity 0 to 1 easing over 2 s; the two rows above it are the Rockbox build
  revision and, in grey, the Tiny5 release tag. `tools/version.sh --release`
  resolves the tag — HEAD's tag on a release build, otherwise the release the
  notes table says is next — and it reaches the code as `RBRELEASE` in
  `rbversion.h`. The 230 KB boot bitmap is gone.
- Missing covers are built in the background while nothing moves, the visible
  ones first, then outwards from the centre. While the covers are moving only a
  couple are read per frame (one while music plays), and all this work pauses
  when the screen is off or the music needs the CPU.

### 4.5 Now Playing

Centred: the cover in the middle, the track as text under it, one progress
band at the bottom.

```
┌────────────────────────────────────────┐
│ Now Playing           12:41      BT 37%│
│                                        │
│              ┌──────┐                  │  cover, centred, as big as the
│              │ cover│                  │  room the text leaves, up to
│              └──────┘                  │  120 px (aspect kept)
│            Come Together               │  title
│            The Beatles                 │  artist   (grey)
│            Abbey Road                  │  album    (grey)
│            3 of 12 · shuffle           │  position and mode (grey)
│            FLAC · 921 kbps · 44.1 kHz  │  the file (muted)
│ ───────────────────────────────────────│  rule
│ 0:15 ███████-------------       -6:05  │  8 px band, times either side
└────────────────────────────────────────┘
```

Every line is centred, and so is the cover. An album with no art keeps the
cover's square, marked with the Rockbox clef, so the readout sits in the same
place either way. The band stays full width - the times at either end make it
the screen's ruler.

- Changing the volume replaces the band with a volume readout for 1.5 s.
- Scrubbing (Centre twice, or held): the band becomes the scrub bar with a 4 px
  cursor block; the wheel seeks, Centre or 3 s of idle ends it.
- Pages of Now Playing (track info, big art) are not separate screens; the
  extra data is on the readout lines and the on-play menu.

### 4.6 Pop-ups, yes/no, progress

A dimmed screen and a rounded panel (radius 12, 16 px padding). Yes/no and
short choice lists (up to six options) are drawn as rows inside the panel;
longer lists fall back to a full-screen list. Text is word-wrapped to the panel
width. The progress bar under a splash — the one the disk and database scans
show — is a flat 6 px band of track with the fill over it, no pill ends.

### 4.7 Settings

`settings_list.c` and `apps/menus/` unchanged in behaviour, drawn by the new
list widget: values in grey on the right of the row. New in Settings › General
› Display: **theme** (Dark, Light, Nord, Gotham, Rose Pine, Tokyo Night, One
Light, GitHub Light), **selection style** (Classic accent band / Minimal band),
**status bar clock**. The accent comes from the theme. Theme Settings and the Bluetooth submenu are
gone — Bluetooth is a Home item.

### 4.8 System screens

- **System › System Status**: a live text readout, redrawn once a second — CPU
  load with a bar, memory, storage, I/O, the database, uptime.
- **System › View Battery Usage**: the battery level against hours since the
  last charge, as a full-screen chart with a header. The log (a point every 5%,
  sampled every 30 s) is kept in `.rockbox/newui/battery.txt`.
- **System › Debug**: Rockbox's debug menu, plus the UI gallery, CPU idle
  statistics and the temporary 1008 / 252 MHz switch for measuring power.

### 4.9 Other screens

| Screen | As built |
|---|---|
| Lock screen | Power locks the keys and blanks the display from any screen; the lock screen shows the clock, the date and the playing track as text, centred. Power or double-press Centre unlocks |
| Text entry | Wheel-driven character strip, text in a field at the top |
| USB connected | Full-width status bar, USB glyph, "Connected", battery |
| Boot / shutdown | The Rockbox icon, code-drawn from the logo, faded in bottom-left with the Rockbox revision and the Tiny5 tag / "Goodbye" |
| Splash, yes/no, database progress | The rounded panel of §4.6 |

---

## 5. Code structure

```
apps/newui/
  ui.h          design tokens: sizes, colours, the API
  ui_style.c    the seven palettes, the two selection styles, the accents
  ui_font.c     loading the UI font
  ui_draw.c     primitives: rects, gradients, rounded corners, alpha, text
                with ellipsis, the drill chevron
  ui_status.c   the status bar
  ui_wheel.c    wheel speed-up, the fast-scroll letter
  ui_transition.c  slide between list levels
  ui_art.c      the playing track's cover, decoded and cached
  ui_thumb.c    album covers at 48 and 150 px, on the card and in RAM
  ui_popup.c    panels, yes/no, choice lists, wrapped text
  ui_keyboard.c text entry
  ui_gallery.c  the debug-menu UI gallery
  ui_time.c     µs counter for frame timing
  scr_home.c    Home
  scr_np.c      Now Playing's drawing (its logic stays in gui/wps.c)
  scr_coverflow.c  Cover Flow and its album sheet
  scr_stats.c   System Status
  scr_battery.c battery log and chart
  scr_lock.c    the lock screen
apps/gui/list.c + apps/gui/bitmap/list.c  every list, restyled
apps/root_menu.c   Home and screen switching
```

Connects to: `tagtree.c`/`tagcache.c` (library), `playback.c` events,
`recorder/albumart.c`, `jpeg_load.c`, `resize.c`, `settings_list.c`,
`viewport.c`, `statusbar.c`, `onplay.c`.

---

## 6. Phases

| Phase | Goal | Status |
|---|---|---|
| 0. Tooling | Scripted simulator screenshots, font build, gallery, frame timer | Done |
| 1. Foundations | Drawing primitives, the status bar | Done |
| 2. Lists | New style for every list, menu and browser | Done |
| 3. Home | Home as the start screen, transitions | Done |
| 4. Now Playing | Console-style Now Playing, volume band, scrub | Done |
| 5. Library | Database browsing with art rows | Done |
| 6. Cover Flow | Covers, cache, animation, album sheet | Done |
| 7. Pop-ups | Yes/no, splash, keyboard, USB, lock, boot | Done; the visualiser is not |
| 8. Removal | Theme engine, skin parser, WPS, backdrops, voice, quick screen | Done |
| 9. Polish | Transitions (done), accents (done), translations of the new `LANG_NEWUI_*` strings (open), power work (ongoing, see POWER.md) | Open |

---

## 7. Testing

- **Simulator first.** Every screen is built and checked in `build-sim` with a
  key script (`RB_SCRIPT`) writing PNGs to `RB_SHOTDIR`, run headless. The
  screenshots in `docs/screens/` are made that way, one run per theme,
  by `ci/screens.sh`.
- **The gallery** draws each widget with fake data, on the device as well as in
  the simulator, so fonts, colours and row heights can be checked on real
  hardware.
- **Speed** is measured on the device with `ui_time.c` and the CPU-idle
  readout; the builds must compile for firmware, simulator and warble with no
  new warnings.

---

## 8. Risks, and where they ended up

| # | Risk | Outcome |
|---|---|---|
| 1 | Drawing speed on the X1000 | Resolved. Measured in the gallery: clearing the screen 0.64 ms, a full `lcd_update()` 15.4 ms (65 fps). Frame delivery, not drawing, is the limit; Cover Flow is capped at 30 fps while music plays, and small changes use `lcd_update_rect()` |
| 2 | Anti-aliased fonts need FreeType and a large file | Sidestepped: Tiny5 Mono is a 9 px bitmap font, 452 KB, no conversion needed at build time. The 27 MB of Noto CJK faces the UI used to carry are gone |
| 3 | The theme engine touched 37 files | Removed (phase 8) without breaking the menus; the list widget is the only restyling point left |
| 4 | Album art: JPEG decode is slow, libraries without art look empty | Solved by the thumbnail cache (`ui_thumb.c`) and a placeholder only where a cover is genuinely missing |
| 5 | Database dependency for Cover Flow | Album lists are built in one pass over the database when Cover Flow opens, and reused while the database is unchanged. No database format change was needed |
| 6 | Memory (32 MB) | Cover caches are static: 24 × 48² and 16 × 150² pixels (~830 KB), Cover Flow's album/track pools ~290 KB. Fonts are cached per glyph, not wholesale |
| 7 | Upstream drift | Accepted: this is a fork. New UI code stays in `apps/newui/` so conflicts stay local |
| 8 | Translations | The new strings are English-only (`LANG_NEWUI_*`); other languages fall back |

---

## 9. Decisions

1. **No iPod-style design.** The classic iPod's silver bars, rounded selection
   cards and page dots are not the target any more; the interface is a compact,
   text-first design sized by one 9 px monospace font. Cover Flow stayed because
   it works, not because the iPod had it.
2. **Font**: Tiny5 Mono for every UI slot (was Noto Sans CJK JP, before that
   Nimbus Sans, before that Source Sans 3). Glyphs load from the card, so file
   size costs disk, not RAM.
3. **CJK**: deferred, not abandoned — `fonts/newui/mkfonts.sh` stays for a
   later build; the generated faces are not committed until one exists.
4. **Back** always goes to the parent screen, including from Now Playing.
5. **Default look**: Dark theme, Classic selection (the accent band); the other
   palettes and Minimal are options. The accent is never chosen separately — it
   belongs to the theme.
6. **Power** locks the keys and turns the display off rather than sleeping the
   player, from any screen; one press wakes it.
7. **Bluetooth is a Home item**, not a settings submenu.
8. **Repo**: this tree (`tiny5-rockbox`) is built on top of
   `dismal-ad/h2-rockbox`, which is the base H2 port.
