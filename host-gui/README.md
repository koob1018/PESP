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

Select the base-station USB serial port in the GUI and click `Connect`. On
macOS it usually looks like `/dev/cu.usbmodemXXXX`.

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
- Runtime Configuration: product controls for tamper sensitivity, monitoring rate, environmental limits, and protection mode.
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
| Protection Mode | `SERVICE_MODE=0/1`, `ENABLE_INTERRUPT=0/1` |
| Light Limit | `SET_LIGHT_HIGH=<raw>` |
| Temperature Limit | `SET_TEMP_HIGH=<C>` |
| Humidity Limit | `SET_HUMIDITY_HIGH=<percent>` |

Presets:

- High sensitivity: threshold `600`
- Medium sensitivity: threshold `1000`
- Low sensitivity: threshold `1800`
- Fast monitoring: `500 ms`
- Normal monitoring: `2000 ms`
- Slow monitoring: `5000 ms`

## Data Source

The GUI reads the base-station USB CDC serial port. The base station reads the
sensor node over the framed UART protocol and publishes GUI-compatible live
sample lines plus compact diagnostics.
