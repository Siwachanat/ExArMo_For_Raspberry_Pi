#!/bin/bash
# Raspberry Pi / Linux launcher (system Python from pi/install_pi.sh, no .venv)
cd "$(dirname "$0")"
exec python3 ExArMoLab_v2.9.py "$@"
