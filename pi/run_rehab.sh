#!/bin/bash
# ExArMo Rehab v2.9.4 — touch GUI (fullscreen on screens narrower than 1400 px).
# Escape leaves fullscreen, F11 toggles it, Advanced → "Exit app" quits.
cd "$(dirname "$0")/../gui"
exec python3 ExArMoRehab_v2.9.4.py "$@"
