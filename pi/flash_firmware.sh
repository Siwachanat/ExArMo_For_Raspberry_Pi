#!/bin/bash
# Flash firmware v2.9z to the MKS XDrive Mini from the Pi.
#   1. Put the board in DFU mode: hold BOOT0, tap RESET (or power it with BOOT0 held), release.
#   2. bash pi/flash_firmware.sh
# If PlatformIO is installed (VS Code + PlatformIO extension) it builds and uploads.
# Otherwise it flashes an already-built firmware.bin with dfu-util.
FW="$(cd "$(dirname "$0")/../firmware/mks_xdrive_mini_firmware" && pwd)"
PIO="$(command -v pio || echo "$HOME/.platformio/penv/bin/pio")"
if [ -x "$PIO" ]; then
  cd "$FW" && exec "$PIO" run -t upload
fi
BIN="$FW/.pio/build/genericSTM32F405RG/firmware.bin"
[ -f "$BIN" ] || BIN="$FW/firmware.bin"
[ -f "$BIN" ] || { echo "No PlatformIO and no firmware.bin found. Build on the PC and copy firmware.bin to $FW/"; exit 1; }
dfu-util -l | grep -q "0483:df11" || { echo "Board not in DFU mode (hold BOOT0, tap RESET)."; exit 1; }
exec dfu-util -a 0 -s 0x08000000:leave -D "$BIN"
