# Smart Exhibit Case Guardian Host GUI

Local browser dashboard for the PESP two-board demo. The GUI presents the system
as a smart museum display-case guardian instead of a plain serial data viewer.

## Run

```bash
cd PESP
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python3 host-gui/app.py --no-autoconnect --port 8765
```

Then open:

```text
http://127.0.0.1:8765
```

Select the sensor-node USB serial port in the GUI and click `Connect`. On macOS
it usually looks like `/dev/cu.usbmodemXXXX`.

Without `--no-autoconnect`, the app tries to auto-detect a Pico/USB modem
serial port. Use `--serial /dev/cu.usbmodemXXXX` to force a specific port.

## Product View

The dashboard shows:

- Case status: `Protected`, `Attention`, or `Tamper Alert`.
- Preservation environment: BME680 temperature, humidity, pressure, and gas/VOC proxy status.
- Light exposure: VEML7700 raw light classified as `Dark`, `Normal`, or `Bright`.
- Tamper monitoring: FSR raw value, runtime threshold, event latch, and last interrupt time.
- Recent trends: fixed-scale three-minute charts for temperature, humidity, light, and force.
- Alarm Log: armed tamper events from the current GUI session.
- Runtime Configuration: product controls for tamper sensitivity, monitoring rate, protection mode, and FSR baseline calibration.
- Collapsed debug diagnostics and raw serial output for troubleshooting.

## Status Rules

- `Tamper Alert`: protection mode is `Armed` and the sensor node has a latched event or an interrupt occurred in the last 15 seconds.
- `Attention`: serial data is missing/stale, the serial link is disconnected, or the BME680 environment stream is unavailable.
- `Protected`: the serial stream is fresh, environment data is available, and no armed tamper event is active.

Light exposure does not trigger the case alarm in this demo because the GUI uses
raw VEML7700 values rather than calibrated lux.

Gas readiness does not trigger `Attention` by itself. `GAS=ERR` is displayed as
warming/not ready when temperature, humidity, and pressure are still available.

## Runtime Configuration Mapping

The product controls are mapped onto the existing sensor-node runtime settings:

| GUI control | Device setting |
| --- | --- |
| Tamper Sensitivity | `SET_FORCE_THRESHOLD=<raw>` |
| Monitoring Rate | `SET_SAMPLING_RATE=<ms>` |
| Protection Mode | `ENABLE_INTERRUPT=0/1` |

Presets:

- High sensitivity: threshold `600`
- Medium sensitivity: threshold `1000`
- Low sensitivity: threshold `1800`
- Fast monitoring: `500 ms`
- Normal monitoring: `2000 ms`
- Slow monitoring: `5000 ms`

`Calibrate` samples the current FSR value for 5 seconds, uses the median as the
baseline, and recommends `baseline + 600` clamped to `300-3500`. The
recommendation is shown to the user and is only applied after pressing
`Apply Recommendation` and then `Apply Runtime Config`.

## Data Source

The GUI reads the sensor-node USB console. The base-station remains USB-headless
during normal operation; its Zephyr driver activity is visible through the
framed command logs printed by the sensor-node.
