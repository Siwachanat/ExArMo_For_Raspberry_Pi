#!/bin/bash
# ExArMo Touch v2.9.4 — Raspberry Pi / Linux launcher (system Python, see pi/install_pi.sh)
#   EXARMO_WINDOWED=1 ./run_touch.sh   → start in a window instead of fullscreen
#   EXARMO_SCALE=0.8  ./run_touch.sh   → force a size (default: fits the screen)
cd "$(dirname "$0")"
exec python3 ExArMoTouch_v2.9.4.py "$@"
