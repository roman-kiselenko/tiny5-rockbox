#!/bin/sh
# Fonts for the new UI. Two parts:
#
# 1. Tiny5 Mono - the font the UI actually uses, for every slot.
#    SIL OFL 1.1, https://github.com/Gissio/font_Tiny5Mono. A 9 px pixel font
#    covering Latin, Greek, Cyrillic and IPA (no CJK or Hangul). Its .fnt is
#    committed, so a normal build converts nothing; convbdf needs no scaling,
#    the font is used at its native size.
#
# 2. Noto Sans CJK JP (SIL OFL 1.1) - Latin,
#    Greek, Cyrillic and Chinese, Japanese and Korean (kana, Han in Japanese
#    shapes, Hangul). The UI used these before Tiny5 and they are not
#    committed any more: this part is the seed for a future
#    Chinese/Japanese/Korean build. Run it, then point the UI_FONT_* slots in
#    apps/newui/ui_font.c at the newui-*.fnt it writes (buildzip installs
#    every .fnt in this directory). Needs tools/convttf (make -C tools
#    convttf, requires libfreetype-dev) and the Noto CJK fonts: the
#    Ubuntu/Debian package fonts-noto-cjk, or NOTO=<dir> with
#    NotoSansCJK-Regular.ttc and NotoSansCJK-Bold.ttc.
#
# -t 0: the JP face of the collection. -TA/-TD trim the font's tall line
# spacing to what the glyphs use (accents, Han and Hangul still fit).
# -e: the two- and three-em dashes are left out; they are 2-3 times wider
# than any other glyph and every glyph cache entry is sized to the widest.
# Glyphs load from the card on demand, so the size of these files (several
# MB each) costs disk space, not RAM.
set -e
cd "$(dirname "$0")"

if command -v convbdf >/dev/null; then
    convbdf -f 08-Tiny5Mono.bdf     # writes 08-Tiny5Mono.fnt here
else
    echo "mkfonts: convbdf not found, keeping the committed 08-Tiny5Mono.fnt"
fi

NOTO=${NOTO:-/usr/share/fonts/opentype/noto}
if [ ! -f "$NOTO/NotoSansCJK-Regular.ttc" ] || [ ! -f "$NOTO/NotoSansCJK-Bold.ttc" ]; then
    echo "mkfonts: Noto Sans CJK not found ($NOTO), skipping the CJK faces"
    echo "         install fonts-noto-cjk or set NOTO=<dir> with the .ttc files"
    exit 0
fi

C="../../tools/convttf -t 0 -TA 10 -TD 20 -e 0x2E3A -e 0x2E3B"
# text: everything the font has in the Basic Multilingual Plane
$C -p 14 -s 32 -l 65535 -o newui-14.fnt     $NOTO/NotoSansCJK-Regular.ttc  # small: times, counts
$C -p 18 -s 32 -l 65535 -o newui-18.fnt     $NOTO/NotoSansCJK-Regular.ttc  # secondary: artist, values
$C -p 20 -s 32 -l 65535 -o newui-20bold.fnt $NOTO/NotoSansCJK-Bold.ttc     # rows, the UI font
$C -p 22 -s 32 -l 65535 -o newui-22bold.fnt $NOTO/NotoSansCJK-Bold.ttc     # titles
# big letter / numbers: ASCII only, which needs less room than Han
$C -TA 25 -TD 30 -p 40 -s 32 -l 126 -o newui-40bold.fnt $NOTO/NotoSansCJK-Bold.ttc
