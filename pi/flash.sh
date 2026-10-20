#!/bin/bash
# Flash the XDrive Mini from the Pi with dfu-util (no PlatformIO needed on the Pi).
#   1. Build on the PC:  .pio/build/genericSTM32F405RG/firmware.bin
#   2. Copy it here, put the board in DFU mode: hold BOOT, press RESET, release BOOT
#   3. bash pi/flash.sh firmware.bin
set -e
BIN="${1:-firmware.bin}"
[ -f "$BIN" ] || { echo "usage: $0 <firmware.bin>   (file not found: $BIN)"; exit 1; }
if ! lsusb | grep -qi "0483:df11"; then
  echo "No STM32 in DFU mode found. Hold BOOT, press RESET, release BOOT, then run again."
  exit 1
fi
sudo dfu-util -a 0 -s 0x08000000:leave -D "$BIN"
echo "Flashed. Press RESET on the board if it does not restart by itself."
