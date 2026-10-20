# ExArMo on Raspberry Pi 5 + 10-inch touchscreen (v2.9.4)

The Pi runs **ExArMo Touch** (`gui/ExArMoTouch_v2.9.4.py`): the same Rehab app as on the PC — Passive,
Assistive, Resistive, Games, "0 here", Find limits, soft STOP — sized for a 1280×800 or 1024×600 touchscreen.
The board connects to the Pi by USB exactly as it does to the PC.

## 1. Get the folder onto the Pi

### Recommended: GitHub (private repo) — best when you will keep changing the code
**On the PC (once):**
1. Install Git for Windows (git-scm.com) if VS Code says Git is missing.
2. VS Code → *File → Open Folder* → `ExArMo_Release_v2.9.4` → *Source Control* (Ctrl+Shift+G) →
   **Publish to GitHub** → choose **private**. (Sign in to GitHub when asked.)
   `.gitignore` already leaves out `.venv`, `.pio` builds, logs and `rehab_settings.json`.

**On the Pi (once)** — in a terminal on the Pi, or from the PC with `ssh <user>@<pi-name>.local`:
```bash
ssh-keygen -t ed25519 -C "exarmo-pi5"        # press Enter 3×
cat ~/.ssh/id_ed25519.pub                     # copy this line
```
GitHub → your repo → *Settings → Deploy keys → Add deploy key* → paste → Save
(or *your profile → Settings → SSH and GPG keys* if the Pi should push too — tick "Allow write access" for a deploy key).
```bash
git clone git@github.com:<your-user>/<repo>.git ~/exarmo
bash ~/exarmo/pi/install_pi.sh
```

**Every update after that:** PC → change → *Commit* → *Sync/Push*. Pi → `bash ~/exarmo/pi/update.sh`
(= `git pull`). One command, no copying, and you always know which version is on the Pi.

### Alternative: USB flash drive — fine for a one-time copy
Copy the whole `ExArMo_Release_v2.9.4` folder (without `gui/.venv` and `firmware/…/.pio`) to the drive,
then on the Pi copy it to `~/exarmo` and run `bash ~/exarmo/pi/install_pi.sh`.
Every later change has to be copied again by hand. (Google Drive / OneDrive work the same way.)

## 2. Install (once) — `bash ~/exarmo/pi/install_pi.sh`
Installs python3-tk, pyserial, numpy, pandas, matplotlib, scipy, PyQt5, pyqtgraph and dfu-util with **apt**
(no `.venv` on the Pi — don't copy the PC's `.venv`, it is Windows-only), gives your user access to the USB
serial port and DFU, puts an **ExArMo Touch** icon on the desktop, and asks about autostart and screen blanking.
**Log out and back in once afterwards** (the serial-port permission needs it).
Installed before the new look (rounded buttons)? Add the extras once:
`sudo apt install python3-pil python3-pil.imagetk fonts-inter fonts-jetbrains-mono`

## 3. Run
* Tap **ExArMo Touch** on the desktop, or `~/exarmo/gui/run_touch.sh`.
* Starts fullscreen. **Advanced → ✕ Quit** closes it; **⛶ Window / Full** (or F11 / Esc with a keyboard) toggles.
* The app opens on the **Get ready** page: it connects to the board by itself (`/dev/ttyACM0`; if not, pick the
  port and tap **Connect**). Then put the arm on the bottom stop → **0 here** → **Find limits**.
  When homing is done, **Continue → choose therapy** turns green — the therapy modes are locked until then.
* STOP in a mode = lower softly to 0°. If the USB is unplugged, the app goes back to Get ready.
* Short screen (1024×600): the slider list scrolls — drag on the labels or the slim bar on the right;
  START / STOP stay visible below the sliders.
* Other apps on the Pi: `gui/run_rehab_pi.sh` (desktop layout), `run_gui_pi.sh`, `run_lab_pi.sh`, `run_motortester_pi.sh`.
* Options: `EXARMO_WINDOWED=1 ./run_touch.sh` (window), `EXARMO_SCALE=0.8 ./run_touch.sh` (force size).
* Logs go to `gui/logs/`, tuned sliders to `gui/rehab_settings.json` (stays on the Pi, not in git).

## 4. Flash firmware from the Pi (optional — flashing from the PC still works)
* **VS Code + PlatformIO on the Pi:** open `firmware/mks_xdrive_mini_firmware`, put the board in DFU mode
  (hold BOOT0, tap RESET), click *Upload* — same as on the PC.
* **Or** `bash ~/exarmo/pi/flash_firmware.sh` (uses PlatformIO if installed, otherwise flashes
  `firmware.bin` with dfu-util — build it on the PC and copy `.pio/build/genericSTM32F405RG/firmware.bin`
  into `firmware/mks_xdrive_mini_firmware/`).

## Troubleshooting
| Problem | Fix |
|---|---|
| `Permission denied: /dev/ttyACM0` | you did not log out/in after install (or run `groups` — must list `dialout`) |
| No `/dev/ttyACM0` | USB cable is power-only, or the board is in DFU mode (press RESET without BOOT0) |
| Everything too big / too small | `EXARMO_SCALE=0.7 ./run_touch.sh` (try 0.55 – 0.9) |
| Touch taps land in the wrong place | Pi menu → Preferences → Screen Configuration → set the touchscreen to the right display/rotation |
| `No module named tkinter` | `sudo apt install python3-tk` |
| Screen goes black during therapy | `sudo raspi-config` → Display → Screen Blanking → No |
