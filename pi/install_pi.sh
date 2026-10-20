#!/bin/bash
# ExArMo v2.9.4 — one-time setup on Raspberry Pi 5 (Raspberry Pi OS 64-bit, desktop).
# Run from anywhere:   bash ~/exarmo/pi/install_pi.sh
# Uses the system Python + apt packages (no .venv needed on the Pi).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo "== ExArMo folder: $ROOT"

echo "== 1/5 Python packages (apt)"
sudo apt update
sudo apt install -y python3-tk python3-serial python3-numpy python3-pandas \
                    python3-matplotlib python3-scipy python3-pyqt5 python3-pyqtgraph \
                    python3-pil python3-pil.imagetk dfu-util fonts-dejavu-core git
# fonts for the touch app (optional — it falls back to Piboto / DejaVu if these are missing)
sudo apt install -y fonts-inter || true
sudo apt install -y fonts-jetbrains-mono || true

echo "== 2/5 USB permissions (serial port + DFU flashing without sudo)"
sudo usermod -aG dialout,plugdev "$USER"
sudo tee /etc/udev/rules.d/49-exarmo-stm32.rules >/dev/null <<'RULES'
# MKS XDrive Mini (STM32F405): USB serial (CDC) and DFU bootloader
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="df11", MODE="0666", GROUP="plugdev"
KERNEL=="ttyACM*", ATTRS{idVendor}=="0483", MODE="0666", GROUP="dialout"
RULES
sudo udevadm control --reload-rules && sudo udevadm trigger

echo "== 3/5 Launchers executable"
chmod +x "$ROOT"/pi/*.sh "$ROOT"/gui/*.sh 2>/dev/null || true

echo "== 4/5 Desktop icon"
mkdir -p "$HOME/Desktop" "$HOME/.local/share/applications"
sed "s|@ROOT@|$ROOT|g" "$ROOT/pi/ExArMoTouch.desktop" > "$HOME/Desktop/ExArMoTouch.desktop"
cp "$HOME/Desktop/ExArMoTouch.desktop" "$HOME/.local/share/applications/ExArMoTouch.desktop"
chmod +x "$HOME/Desktop/ExArMoTouch.desktop"
gio set "$HOME/Desktop/ExArMoTouch.desktop" metadata::trusted true 2>/dev/null || true

echo "== 5/5 Options"
read -r -p "Start ExArMo Touch automatically after login? [y/N] " a
if [[ "$a" =~ ^[Yy] ]]; then
  mkdir -p "$HOME/.config/autostart"
  cp "$HOME/Desktop/ExArMoTouch.desktop" "$HOME/.config/autostart/ExArMoTouch.desktop"
  echo "   autostart ON  (remove ~/.config/autostart/ExArMoTouch.desktop to turn it off)"
fi
read -r -p "Turn off screen blanking (screen stays on during therapy)? [Y/n] " b
if [[ ! "$b" =~ ^[Nn] ]]; then
  sudo raspi-config nonint do_blanking 1 2>/dev/null && echo "   screen blanking OFF" || echo "   (set it in raspi-config → Display)"
fi

echo
echo "Done. LOG OUT and back in (or reboot) once so the USB permissions apply."
echo "Then tap the ExArMo Touch icon on the desktop, or run:  $ROOT/gui/run_touch.sh"
