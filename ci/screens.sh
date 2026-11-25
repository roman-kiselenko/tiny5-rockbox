#!/bin/sh
# Regenerate docs/screens for every theme.
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
THEMES="dark light nord gotham rose-pine tokyo-night one-light github-light"

for theme in $THEMES; do
    mkdir -p "shots/$theme" "$SRC/docs/screens/$theme"
    printf 'theme: %s\n' "$theme" > simdisk/.rockbox/config.cfg
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        RB_SCRIPT="$SRC/ci/screens.script" \
        RB_SHOTDIR="./shots/$theme" \
        timeout 120 "$SIM" >/dev/null
    python3 "$SRC/ci/scale2x.py" "shots/$theme" "$SRC/docs/screens/$theme"
    echo "$SRC/docs/screens/$theme: $(ls "$SRC/docs/screens/$theme" | wc -l) shots"
done
