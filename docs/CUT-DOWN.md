# What this tree does not have

Everything below was removed from upstream Rockbox and is kept out. It is what
makes a single-target player small enough to fit the H2.

- **Other targets.** Every other target, CPU (ARM, ColdFire, other Ingenic
  parts), their drivers, configs and keymaps; the bootloader code and tools;
  the manual, the android and utils directories.
- **Dead hardware branches.** Preprocessor code for features the H2 lacks:
  touchscreen, remote LCD, recording, FM radio, S/PDIF, line-in/mic, button
  lights, iPod accessory and so on.
- **Plugins.** No loader, no plugin menu entries. `plugin_get_buffer()` in
  `apps/plugin.c` still returns the 2 MiB scratch buffer that core code uses.
- **The theme (skin) engine.** `apps/gui/skin_engine/`, `lib/skin_parser/`, the
  skinned status bar and lists, Theme Settings, the quick screen (its button
  now opens Shortcuts), `tools/checkwps` and the CheckWPS build type, `wps/`,
  `backdrops/`, bitmap fonts other than the UI font and the system font, and
  playback's album-art buffering for themes. Now Playing's logic still lives in
  `apps/gui/wps.c`; its drawing is `apps/newui/scr_np.c`.
- **Voice.** Spoken menus are gone: `apps/talk.h` and `apps/voice_thread.h` are
  inline no-op stubs; the call sites remain until each screen is rewritten.
- **Codecs.** Game music (SID, NSF, SPC, SAP/ASAP, AY, VTX, GBS, HES, SGC, VGM,
  KSS), MOD, RealMedia, ATRAC3, AC3, Musepack, TTA, WMA Voice, Speex and ADX.
  Kept: MP1/2/3, AAC/HE-AAC, ALAC, FLAC, Vorbis, Opus, WAV/W64/AIFF, APE,
  WavPack, WMA/WMA Pro, Shorten, DSD, AU, VOX, SMAF.

The core (`apps`, `firmware`, `lib`, `uisimulator`, Bluedroid excluded) is about
**296k lines** of code, against roughly 1.6M upstream.
