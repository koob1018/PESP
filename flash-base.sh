#!/usr/bin/env bash
set -euo pipefail

FIRMWARE="${1:-base-station/build/zephyr/zephyr.uf2}"
BASE_SER="${BASE_SER:-3CFF0B54ACC69F66}"
SENSOR_PORT="${SENSOR_PORT:-/dev/cu.usbmodem1101}"
PICOTOOL="${PICOTOOL:-picotool}"

if [ ! -f "$FIRMWARE" ]; then
  echo "Firmware not found: $FIRMWARE"
  exit 1
fi

if ! command -v "$PICOTOOL" >/dev/null 2>&1; then
  echo "picotool not found. Set PICOTOOL=/path/to/picotool or install/add it to PATH."
  exit 1
fi

echo "[1/5] Checking picotool..."
"$PICOTOOL" version

echo "[2/5] Requesting base-station BOOTSEL..."
if "$PICOTOOL" info -a --ser "$BASE_SER" >/dev/null 2>&1; then
  echo "Base-station is already in BOOTSEL."
else
  if [ ! -e "$SENSOR_PORT" ]; then
    echo "Sensor serial port not found: $SENSOR_PORT"
    echo "Set SENSOR_PORT=/dev/cu.usbmodemXXX if macOS assigned a different port."
    exit 1
  fi
  python3 - "$SENSOR_PORT" <<'PY'
import serial
import sys
import time

port = sys.argv[1]
with serial.Serial(port, 115200, timeout=0.2, write_timeout=1) as ser:
    ser.dtr = True
    ser.rts = True
    ser.write(b"BASE_BOOTSEL\n")
    ser.flush()
    time.sleep(0.2)
PY
fi

echo "[3/5] Waiting for base-station BOOTSEL: $BASE_SER"
for _ in $(seq 1 30); do
  if "$PICOTOOL" info -a --ser "$BASE_SER" >/dev/null 2>&1; then
    "$PICOTOOL" info -a --ser "$BASE_SER"
    break
  fi
  sleep 0.5
done

if ! "$PICOTOOL" info -a --ser "$BASE_SER" >/dev/null 2>&1; then
  echo "Base-station did not enter BOOTSEL."
  echo "If this is the first firmware with auto-BOOTSEL support, put base-station into BOOTSEL manually once and rerun."
  exit 1
fi

echo "[4/5] Loading firmware: $FIRMWARE"
"$PICOTOOL" load -v "$FIRMWARE" --ser "$BASE_SER"

echo "[5/5] Rebooting base-station..."
"$PICOTOOL" reboot --ser "$BASE_SER"

echo "Done."
