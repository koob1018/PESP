# sensor-node

Pico SDK firmware for the room-monitoring sensor node.

## Current purpose
- Read FSR force data from ADC0 / GPIO26.
- Read BME680 temperature, humidity, pressure, and gas resistance over I2C.
- Read VEML7700 ambient light over I2C.
- Respond to framed UART commands from the Zephyr base station, with newline text commands retained for manual debugging.
- Detect FSR threshold events and latch them until cleared.
- Drive the dedicated event interrupt line on GPIO15.

## Key commands
- USB maintenance commands:
  - `I2C_SCAN`
  - `BME_PROBE`
  - `BME_GAS_DEBUG`
  - `BASE_BOOTSEL`
- Framed UART commands:
- `READ_ALL`
- `READ_FORCE`
- `READ_ENVIRONMENT`
- `READ_LIGHT`
- `READ_EVENT`
- `CLEAR_EVENT`
- `SET_FORCE_THRESHOLD=<raw>`
- `SET_SAMPLING_RATE=<ms>`
- `ENABLE_INTERRUPT=<0|1>`

## Notes
- Keep this endpoint Pico SDK only.
- `READ_ALL` and `READ_ENVIRONMENT` report `GAS=<ohms>` when the BME680 gas measurement is valid, otherwise `GAS=ERR`.
- Wiring details are recorded in `../docs/hardware.md`.
- Protocol details are recorded in `../docs/protocol.md`.
