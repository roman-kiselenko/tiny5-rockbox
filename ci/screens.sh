#!/bin/sh
# Regenerate docs/screens for the two palettes the README shows: dark and
# light. Set THEMES to shoot others.
#
# Run from the simulator build directory, after building the simulator:
#   ../ci/screens.sh
#
# It writes the raw shots to shots/<theme>/ and the 2x PNGs that go in the
# repo to docs/screens/<theme>/. The theme is set through the simulator's
# config file, exactly as on the player.

set -e

SRC=${SRC:-"$(dirname "$0")/.."}
SIM=${SIM:-./rockboxui}
# Two palettes are kept in the docs: one dark, one light. Override for a
# one-off look, e.g. THEMES="nord gotham" ./ci/screens.sh
THEMES=${THEMES:-"dark light"}

# The shots are only meaningful with a test library: at least one album whose
# cover decodes (cover.{jpg,jpeg,bmp} next to the track, or an embedded JPEG) so
# Now Playing has art to show.
if [ ! -f simdisk/.rockbox/config.cfg ] || ! find simdisk/music -name "*.mp3" 2>/dev/null | head -1 | grep -q .; then
    echo "no music in simdisk/music - put a test album there first" >&2
    exit 1
fi

for theme in $THEMES; do
    mkdir -p "shots/$theme" "$SRC/docs/screens/$theme"
    # start from nothing, or shots from an earlier set of screens survive
    rm -f "shots/$theme"/*.png "$SRC/docs/screens/$theme"/*.png
    printf 'theme: %s\n' "$theme" > simdisk/.rockbox/config.cfg
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        RB_SCRIPT="$SRC/ci/screens.script" \
        RB_SHOTDIR="./shots/$theme" \
        timeout 120 "$SIM" >/dev/null
    python3 "$SRC/ci/scale2x.py" "shots/$theme" "$SRC/docs/screens/$theme"
    echo "$SRC/docs/screens/$theme: $(ls "$SRC/docs/screens/$theme" | wc -l) shots"
done
