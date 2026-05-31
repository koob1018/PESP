# base-station

Zephyr firmware for the room-monitoring base station demo.

## Current purpose
- Communicate with the sensor node over UART0 using framed packets.
- Expose a USB CDC serial endpoint for the host GUI.
- Forward GUI runtime commands to the sensor node through the Zephyr driver API.
- Configure force threshold, sampling interval, light/temperature/humidity limits, interrupt enable state, and service mode.
- Poll `READ_ALL` and `READ_FORCE` for live data.
- Monitor the dedicated event input on GPIO15.
- Fetch and clear latched sensor-node events with `READ_EVENT` and `CLEAR_EVENT`.
- Emit GUI-compatible sample lines so the host app only needs to connect to the base station.

## Source layout
- `src/main.c`: small Zephyr demo application that starts and services the sensor-node driver.
- `src/driver/`: active Zephyr driver-style abstraction for UART framing, GPIO15 event handling, polling, runtime configuration, and latest-value caching.

## Runtime diagnostics
- The host GUI connects to the base-station USB CDC serial port.
- Driver traffic appears as `[tx]`, `[data]`, `[force]`, `[event]`, and `[config]` lines on that port.
- Expected command flow after base reboot: force threshold, sampling interval, light/temperature/humidity limits, service mode, interrupt enable, `READ_CONFIG`, then alternating `READ_ALL` and `READ_FORCE`.
- Expected event flow after pressing the FSR: sensor-node latches an event, GPIO15 goes active, base-station sends `READ_EVENT`, then clears the event with `CLEAR_EVENT`.

## Flashing
- Automatic flashing has been verified with `./flash-base.sh`. Run:

```bash
cd PESP
./flash-base.sh
```

- The script sends `BASE_BOOTSEL` to the sensor-node USB serial port, sensor-node forwards a maintenance frame over UART0, and base-station enters BOOTSEL before `picotool load`.
- Equivalent direct commands once base-station is already in BOOTSEL:

```bash
picotool load -v base-station/build/zephyr/zephyr.uf2 --ser 3CFF0B54ACC69F66
picotool reboot --ser 3CFF0B54ACC69F66
```

## Notes
- Keep this endpoint Zephyr only.
- Host PC talks to the base station over USB serial.
- Base station talks to the sensor node over the board-to-board UART frame protocol.
