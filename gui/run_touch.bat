@echo off
REM ExArMo Touch v2.9.4 on Windows (to preview the touch layout). Uses gui\.venv like run_rehab.bat.
cd /d "%~dp0"
if not exist .venv\Scripts\python.exe (
  python -m venv .venv && .venv\Scripts\python -m pip install --upgrade pip && .venv\Scripts\python -m pip install -r requirements.txt
)
set EXARMO_WINDOWED=1
.venv\Scripts\python ExArMoTouch_v2.9.4.py
