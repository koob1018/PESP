#!/usr/bin/env bash
set -euo pipefail

FIRMWARE="${1:-}"
PICOTOOL="${PICOTOOL:-picotool}"

if [ -z "$FIRMWARE" ]; then
  echo "Usage: ./flash-pico.sh path/to/firmware.uf2 [picotool-device-selection...]"
  echo "   or: ./flash-pico.sh path/to/firmware.elf [picotool-device-selection...]"
  echo
  echo "Example for a specific USB serial:"
  echo "  ./flash-pico.sh build/zephyr/zephyr.uf2 --ser FA63A4D78760442D"
  exit 1
fi

shift
DEVICE_SELECTION=("$@")

if [ ! -f "$FIRMWARE" ]; then
  echo "Firmware not found: $FIRMWARE"
  exit 1
fi

if ! command -v "$PICOTOOL" >/dev/null 2>&1; then
  echo "picotool not found. Set PICOTOOL=/path/to/picotool or install/add it to PATH."
  exit 1
fi

echo "[1/4] Checking picotool..."
"$PICOTOOL" version

echo "[2/4] Rebooting Pico into BOOTSEL..."
if [ ${#DEVICE_SELECTION[@]} -gt 0 ]; then
  "$PICOTOOL" reboot "${DEVICE_SELECTION[@]}" -f -u
else
  "$PICOTOOL" reboot -f -u
fi

sleep 3

echo "[3/4] Loading firmware: $FIRMWARE"
if [ ${#DEVICE_SELECTION[@]} -gt 0 ]; then
  "$PICOTOOL" load "$FIRMWARE" "${DEVICE_SELECTION[@]}"
else
  "$PICOTOOL" load "$FIRMWARE"
fi

echo "[4/4] Rebooting Pico..."
if [ ${#DEVICE_SELECTION[@]} -gt 0 ]; then
  "$PICOTOOL" reboot "${DEVICE_SELECTION[@]}"
else
  "$PICOTOOL" reboot
fi

echo "Done."
