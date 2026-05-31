# Smart Exhibit Case Guardian

Two-board embedded systems coursework project for a smart exhibit-case guardian.

The system uses:

- `sensor-node`: Pico SDK firmware that reads BME680, VEML7700, and FSR sensors.
- `base-station`: Zephyr firmware that wraps the remote sensor node behind a driver-style API.
- `host-gui`: local browser dashboard for live case status, readings, trends, alarms, and runtime configuration.

## Quick Start: Host GUI

Use this path when both firmware images are already flashed and the base-station
USB serial port is connected to the computer.

```bash
git clone https://github.com/koob1018/PESP.git
cd PESP

python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

python3 host-gui/app.py --no-autoconnect --port 8765
```

Open:

```text
http://127.0.0.1:8765
```

Then select the base-station serial port in the GUI and click `Connect`. On
macOS the port usually looks like `/dev/cu.usbmodemXXXX`.

If you already know the serial port, you can autoconnect:

```bash
python3 host-gui/app.py --serial /dev/cu.usbmodemXXXX --port 8765
```

Without `--serial`, the app tries to auto-detect the base-station USB serial
port first, then falls back to a Pico/USB modem serial port.

## Repository Layout

```text
base-station/   Zephyr base-station firmware and sensor-node driver
sensor-node/    Pico SDK sensor-node firmware
host-gui/       Python stdlib + pyserial local browser dashboard
tests/          Protocol frame tests
```

## Firmware Build Notes

Build artifacts are intentionally not committed. Each developer should rebuild
locally.

### Sensor Node

Install the Pico SDK and ARM embedded toolchain, then set `PICO_SDK_PATH`.

```bash
cd sensor-node
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -G Ninja
cmake --build build
```

The output firmware is:

```text
sensor-node/build/sensor_node.uf2
```

### Base Station

Install Zephyr and source the Zephyr environment so `ZEPHYR_BASE` is set.

```bash
cd base-station
west build -b rpi_pico2/rp2350a/m33 .
```

The output firmware is:

```text
base-station/build/zephyr/zephyr.uf2
```

## Flashing

Generic Pico flashing script:

```bash
./flash-pico.sh path/to/firmware.uf2
```

Base-station helper:

```bash
SENSOR_PORT=/dev/cu.usbmodemXXXX ./flash-base.sh
```

`flash-base.sh` asks the sensor-node to forward a BOOTSEL request to the
base-station, then loads the base-station firmware with `picotool`.

## More Detail

- Host GUI: `host-gui/README.md`
- Sensor node firmware: `sensor-node/README.md`
- Base station firmware: `base-station/README.md`
