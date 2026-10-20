#!/bin/bash
# ExArMo control panel (PyQt5). On a Pi use the 100 Hz telemetry rate for live plots.
cd "$(dirname "$0")/../gui"
exec python3 ExArMoGui_v2.9.py "$@"
