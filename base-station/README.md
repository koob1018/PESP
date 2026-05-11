# base-station

Zephyr firmware for the room-monitoring base station demo.

## Current purpose
- Communicate with the sensor node over UART0 using framed packets.
- Configure force threshold, sampling interval, and interrupt enable state through the Zephyr driver API.
- Poll `READ_ALL` and `READ_FORCE` for live data.
- Monitor the dedicated event input on GPIO15.
- Fetch and clear latched sensor-node events with `READ_EVENT` and `CLEAR_EVENT`.
- Keep the final image headless on USB; verify the board-to-board behavior through the sensor-node console.

## Source layout
- `src/main.c`: small Zephyr demo application that starts and services the sensor-node driver.
- `src/driver/`: active Zephyr driver-style abstraction for UART framing, GPIO15 event handling, polling, runtime configuration, and latest-value caching.

## Runtime diagnostics
- The final base-station image has USB CDC disabled for stable macOS runtime/flashing behavior.
- Runtime behavior is verified from the sensor-node USB console, where base commands appear as `[link-rx]` lines.
- Expected command flow after base reboot: `SET_FORCE_THRESHOLD`, `SET_SAMPLING_RATE`, `ENABLE_INTERRUPT`, `READ_CONFIG`, then alternating `READ_ALL` and `READ_FORCE`.
- Expected event flow after pressing the FSR: sensor-node logs `[event] latched ...`, then receives `READ_EVENT`, receives `CLEAR_EVENT`, and logs `[event] cleared`.

## Flashing
- Automatic flashing has been verified with `./flash-base.sh`. Run:

```bash
cd /Users/yiwenxu/Projects/coursework/PESP
./flash-base.sh
```

- The script sends `BASE_BOOTSEL` to the sensor-node USB serial port, sensor-node forwards a maintenance frame over UART0, and base-station enters BOOTSEL before `picotool load`.
- Equivalent direct commands once base-station is already in BOOTSEL:

```bash
picotool load -v /Users/yiwenxu/Projects/coursework/PESP/base-station/build/zephyr/zephyr.uf2 --ser 3CFF0B54ACC69F66
picotool reboot --ser 3CFF0B54ACC69F66
```

## Notes
- Keep this endpoint Zephyr only.
- Wiring details are recorded in `../docs/hardware.md`.
- Protocol details are recorded in `../docs/protocol.md`.
