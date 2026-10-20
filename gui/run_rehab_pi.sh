#!/bin/bash
# Raspberry Pi / Linux launcher (system Python from pi/install_pi.sh, no .venv)
cd "$(dirname "$0")"
exec python3 ExArMoRehab_v2.9.3.py "$@"
