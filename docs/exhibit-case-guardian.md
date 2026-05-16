# Smart Exhibit Case Guardian Design

## Concept

The project is presented as a smart exhibit-case guardian for museum display
cases, archive cabinets, lab sample cabinets, or other protected enclosures.
The system monitors preservation conditions and detects touch or movement
events around the protected object.

This framing makes the existing hardware roles explicit:

| Module | Product role |
| --- | --- |
| BME680 | Preservation environment: temperature, humidity, pressure, and gas/VOC proxy |
| VEML7700 | Light exposure context for sensitive objects |
| FSR | Touch, pressure, movement, or door-contact signal |
| GPIO15 interrupt | Immediate tamper notification from sensor-node to base-station |
| UART framed protocol | Reliable command/response channel between the two boards |
| Zephyr driver | Base-station abstraction for the remote sensor-node device |
| Host GUI | Operator dashboard for state, alarms, diagnostics, and runtime configuration |

## Dashboard Behavior

The GUI uses three product-level case states:

- `Protected`: fresh serial data, environment stream available, and no armed tamper event.
- `Attention`: serial link missing/stale or BME680 environment stream unavailable.
- `Tamper Alert`: protection mode is armed and a latched event or recent interrupt is present.

`Tamper Alert` remains visible in the GUI for 15 seconds after an interrupt so a
demo cannot miss the event if the base-station clears the hardware latch quickly.

The main dashboard keeps raw serial output and driver diagnostics collapsed by
default so the operator sees case state, current readings, trends, alarms, and
runtime controls first.

## Alarm Log

The Alarm Log records armed tamper events from the current GUI session. It does
not export data and it does not store events after the GUI restarts.

Service Mode intentionally suppresses product-level alarms. The sensor-node may
still produce raw event/debug output, but the GUI keeps those details in the raw
serial diagnostics instead of the main Alarm Log.

## Light Exposure

The VEML7700 value is shown as raw light, not lux. The GUI classifies it as:

- `Dark`: raw value below `800`
- `Normal`: raw value from `800` through `3000`
- `Bright`: raw value above `3000`

Light exposure does not trigger `Attention` or `Tamper Alert` in this demo. It
is preservation context, not a calibrated alarm source.

Trend charts use fixed ranges instead of auto-scaling to the current window:
temperature `0-50 C`, humidity `0-100%`, light raw `0-4095`, and force raw
`0-4095`. This prevents low-level readings from visually filling the whole
chart.

## Environment and Gas

If the BME680 environment stream is unavailable, the case enters `Attention`.
If temperature, humidity, and pressure are available but gas is not ready, the
dashboard shows gas as warming/not ready without changing the case status.

The gas value is treated as a VOC proxy because the firmware reports BME680 gas
resistance in ohms only when the heater is stable and the gas-valid bit is set.

## Runtime Configuration

The runtime configuration panel exposes product controls while keeping the
underlying device settings visible:

| Product control | Device command |
| --- | --- |
| Tamper Sensitivity | `SET_FORCE_THRESHOLD=<raw>` |
| Monitoring Rate | `SET_SAMPLING_RATE=<ms>` |
| Protection Mode | `ENABLE_INTERRUPT=0/1` |

Sensitivity presets:

- High: threshold `600`
- Medium: threshold `1000`
- Low: threshold `1800`
- Custom: raw threshold input

Monitoring rate presets:

- Fast: `500 ms`
- Normal: `2000 ms`
- Slow: `5000 ms`
- Custom: raw sampling interval input

`Calibrate Baseline` samples FSR raw values for 5 seconds, takes the median as
the unloaded baseline, then recommends `baseline + 600` clamped to `300-3500`.
The recommendation is not written to the board until the user applies it.

## Demo Flow

1. Start the host GUI and connect to the sensor-node USB serial port.
2. Show `Protected` with live environment, light, force, and trend data.
3. Change the light level and show `Dark`, `Normal`, or `Bright` without raising an alarm.
4. Press the FSR while protection mode is `Armed`.
5. The sensor-node latches the event and drives GPIO15 high.
6. The base-station Zephyr driver reads the event through the framed UART protocol.
7. The GUI shows `Tamper Alert`, logs the event, and keeps the alert visible for 15 seconds.
8. Switch to `Service Mode` to show that maintenance touches do not become product-level alarms.
