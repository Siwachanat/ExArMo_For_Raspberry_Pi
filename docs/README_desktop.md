# ExArMo v2.9.4 — detailed guide for Windows / macOS (firmware v2.9z)

> Short overview: [../README.md](../README.md) · Raspberry Pi: [../README_PI.md](../README_PI.md)

> **New in v2.9.4:** `gui/ExArMoTouch_v2.9.4.py` — the ExArMo Rehab app laid out for a **10-inch touchscreen on a
> Raspberry Pi 5** (1280×800 or 1024×600), plus Pi setup scripts in `pi/`. Firmware is unchanged (v2.9z).
> **Raspberry Pi setup → see [README_PI.md](../README_PI.md).** Everything below still works on Windows / Mac.


> **Current firmware is v2.9z** (v2.9 + manual zero + thermistor + soft stop + driver wake-up). See the top of
> CHANGELOG.md. Quick use: put the arm on the bottom stop → **0 here** → **Find limits** → mode → **STOP** lowers
> the arm to 0° and releases (press STOP again to release immediately).

Everything needed to flash the ExArMo 1-DOF elbow exoskeleton and run it from any
Windows PC or Mac. Nothing here depends on the rest of the project folder.

```
ExArMo_Release_v2.9.4/
├── README.md                     ← GitHub overview
├── docs/README_desktop.md        ← this file (detailed Windows / macOS guide)
├── README_PI.md                  ← ★ Raspberry Pi 5 + touchscreen: install, run, update, flash
├── pi/                           install_pi.sh · update.sh · flash_firmware.sh · ExArMoTouch.desktop
├── CHANGELOG.md                  ← what changed in v2.9 and why (every number has its experiment)
├── firmware/mks_xdrive_mini_firmware/   PlatformIO project · firmware v2.9z (STM32F405, SimpleFOC 2.4.0)
│   ├── platformio.ini            board, DFU upload, pinned libraries
│   └── src/main.cpp              the whole firmware (one file)
├── gui/
│   ├── ExArMoTouch_v2.9.4.py     ★ ExArMo Touch — same app as Rehab, sized for a 10" touchscreen (Pi) · run_touch.sh
│   ├── run_*_pi.sh               Raspberry Pi launchers for every app (system Python, no .venv)
│   ├── ExArMoRehab_v2.9.3.py     ★ ExArMo Rehab — patient/therapist app: Passive · Assistive · Resistive · Games, live arm view (tkinter)
│   ├── run_rehab.bat / .command  double-click to start ExArMo Rehab
│   ├── ExArMoGui_v2.9.py         research control panel (PyQt5): all parameters, live plots, E08 protocol timer
│   ├── run_gui.bat / .command    double-click to start the control panel
│   ├── ExArMoLab_v2.9.py         ExArMo Lab (PyQt5): automated experiments on the full device — gravity ID,
│   │                             plant ID (J, B, τ_f), estimator/transparency stats, thermal checks
│   ├── run_lab.bat / .command    double-click to start ExArMo Lab
│   ├── exarmolab.py              ExArMo Lab · Motor Tester g7-A (tkinter): the lab-rig tabs (KV, KT, Heat, Capstan,
│   │                             Thermal, Arm board, Cal, E9) — now also runs on the REHAB firmware v2.9 (current
│   │                             mode emulated, see docs/lab_procedures.md §6)
│   ├── run_motortester.bat / .command   double-click to start the motor tester
│   └── requirements.txt          Python packages (both apps share one gui/.venv)
├── tools/
│   ├── ExArMoLogAnalyzer_v2.4.py session CSV → paper figures (Overview, Estimators, Adaptation & Noise, Tracking, Stats)
│   ├── analyze_exo_session.py    session CSV → metrics table + 6-panel PNG (tracking, estimator noise, interaction torque, B_v, thermal)
│   └── run_analyzer.bat / .command
└── docs/
    ├── E08_rerun_procedure.md    step-by-step for the §4.8 estimator comparison on the full device
    ├── lab_procedures.md         ExArMo Lab: gravity / plant-ID / transparency / thermal procedures + the thermal IDLE fix
    └── firmware_v2.9_constants.md  the constants table (K_t,j, N, R, L, KV, dead-zone, thermal) with sources
```

## 1. Flash the firmware  (once per board)

Needs: VS Code + the **PlatformIO IDE** extension (or `pip install platformio` for the CLI),
a USB cable to the MKS XDrive Mini, and the board in **DFU mode** (hold BOOT, tap RESET, release BOOT).
Windows also needs the WinUSB driver for the STM32 DFU device (use **Zadig** once: select
"STM32 BOOTLOADER" → WinUSB).

```
cd firmware/mks_xdrive_mini_firmware
pio run -t upload            # or: PlatformIO sidebar → Project Tasks → Upload
```
The first build downloads the three libraries from GitHub (internet needed once); later builds are offline.
Pinned: Simple FOC 2.4.0, Arduino-FOC-drivers @ 61751ea. SimpleCANio is not pinned (no known-good commit);
if it ever fails to build, comment out the three CAN lines in `main.cpp` (`#include "SimpleCANio.h"`,
`CANio can(...)`, `CANCommander commandc(...)` and the two `commandc.` calls) — CAN is not used by the GUI.

After flashing, reconnect USB (the board re-enumerates as a serial port, 921600 baud).

## 2. Run the software

Three programs talk to the same firmware — use one at a time (one serial port, one program):

| | **ExArMo Rehab** `run_rehab` | **Control panel** `run_gui` | **ExArMo Lab** `run_lab` |
|---|---|---|---|
| For | therapy sessions, demos, patients/therapists | manual experiments, tuning, E08 timer | automated identification & validation runs on the device |
| Shows | 4 therapy modes, games, live arm avatar, effort gauge | every firmware parameter, live plots of all 3 estimators | same plots + live estimator/transparency statistics |
| Does | — | — | Gravity stations → g, φ · passive sweeps → J, B, τ_f · mode protocol → B_int, power · thermal IDLE check & stall test |
| Logs | `gui/logs/exo_*.csv` (+ `.meta.txt`), ≈280 Hz | same format, + E08 stage markers | `gui/logs/lab_*.csv` + `lab_<experiment>_*.json` results |
| Remembers | gravity g, park angle, Iq limit, dead-zone, … in `gui/rehab_settings.json` (pushed to the board on Connect) | — (press "Send all parameters") | — ("Apply" buttons send fitted values; "Send all parameters") |

**ExArMo Lab:** Connect → Thermal tab "IDLE check" → **Home** → Gravity tab "Run gravity stations" (nobody
touching) → "Apply fitted g, φ" → Plant ID tab "Run plant-ID sweeps" → "Apply" → user in the arm, RUN →
Estimator tab "Run mode protocol". See `docs/lab_procedures.md`.

**Motor tester (`exarmolab.py`, `run_motortester`):** the original lab-rig program (KV, KT, Heat, Capstan, Thermal,
E9 ground truth, arm sensor board). It detects which firmware answers: with the LabTest firmware it is unchanged;
with the rehab firmware v2.9 every lab command is translated (current mode = LAB velocity mode stalled at a ramped
current limit — the shaft must be blocked; HOLD = passive hold at the current angle after a rehab homing; Z/A/K are
not sent). Temperature from the rehab board is the firmware's **model** `T_est`, labelled as such — it is not a
thermistor. Details and limits: `docs/lab_procedures.md` §6.

**ExArMo Rehab:** Connect → **Find limits (H)** → pick a mode card. Tune *Gravity support g* once on the
Assistive page with the arm support attached (dead-zone 0 while tuning, then back to 0.15) — the value is saved
and shared by Assistive and Resistive. The sidebar shows the board check (`fw v2.9 ✓`), the joint angle and the
estimated motor temperature; red STOP is always on screen.

### Control panel

* **Windows:** double-click `gui/run_rehab.bat` or `gui/run_gui.bat`. First run creates `gui/.venv` and installs the
  packages (needs Python 3.9+ from python.org with "Add to PATH" ticked; ~2 min).
* **macOS:** double-click `gui/run_rehab.command` / `gui/run_gui.command` (if macOS refuses: right-click → Open
  once, or `bash gui/run_rehab.command` in Terminal). Use the **python.org** Python — ExArMo Rehab needs tkinter,
  which Homebrew's Python lacks unless you `brew install python-tk`.
* Manual: `pip install -r gui/requirements.txt` then `python gui/ExArMoGui_v2.9.py`.

In the GUI: pick the port → **Connect** → the console must show the boot line
`READY v2.9 (Ktj 0.775, R 0.47, dz 0.15, thermal derate).` and, after **Status**,
`... Ktj=0.7750 N=9.05 R=0.470 L=0.250mH dz=0.150 ... fw=v2.9`.
If `fw=v2.9` is missing the board still has old firmware — go back to step 1.

Then: **1. Home** (arm finds both hard stops and parks) → **2. Admittance** → push the arm.
Logs go to `gui/logs/exo_YYYYMMDD_HHMMSS.csv` + `.meta.txt` (all parameters, markers).
Note: at `D2` the board actually delivers ≈ 280 frames/s (print cost of 13 floats) — quote the measured rate.

## 3. Analyse a recording

`tools/run_analyzer.bat` / `.command` (or the GUI's "Analyze last recording" button) opens the
LogAnalyzer with figure tabs; File → Open any `exo_*.csv`. Export figures from its toolbar.

`python tools/analyze_exo_session.py gui/logs/exo_*.csv --g 0.60 --phi 0` prints the metrics table
(tracking RMS, estimator noise/agreement/lag, interaction-torque fit τ_m−G = B·ω + C·sgn(ω) and net motor
power, B_v law, thermal-model sanity) and writes `*_analysis.png`.

## 4. Before the §4.8 / E08 session

**Check the thermal estimator first** (ExArMo Lab → Thermal → IDLE check). On 18 Oct `T_est` read 26 130 °C
before RUN was pressed and the Iq limit was derated to 3 A for the whole session: the integrator runs on the
measured current while the driver is high-Z in IDLE. The fix is in `docs/lab_procedures.md` §0.

Read `docs/E08_rerun_procedure.md`. The one thing that must be done on the device and cannot be
precomputed: **re-tune the gravity coefficient g with the finished arm support** (Admittance,
Assist 0, dead-zone 0, release the arm at several angles until it stays put), then set the
dead-zone back to 0.15 N·m. Write the motor serial in the GUI note field.

## Serial protocol (for scripts)

Telemetry `D,t_ms,state,theta,theta_ref,vel,iq_meas,tau_m,tau_s,tau_d,tau_dob,Bv,iq_cmd,T_est` (angles rad, torques N·m).
Commands: `H` home · `O1` run · `O0` idle · `E0/1/2` estimator · `A<f>` assist · `G<rad>` passive goto ·
`D<ms>` telemetry period (`D2` = 500 Hz) · `L` status · `P<letter><value>` parameters (letters listed in `main.cpp` header).

## Safety

Emergency stop = cut motor power. Software: Iq clamp 6 A, thermal derate to 3 A above 70 °C (estimated),
3-s park timeout, range fault → motor off, virtual walls at the capstan limits. The capstan drive is
backdrivable: the wearer can always overpower the device.
