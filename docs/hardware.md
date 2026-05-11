# Hardware / 硬件连接

This file records the current wiring assumptions used by the firmware.
本文件记录当前固件使用的硬件连接假设。

## Board / 开发板

- Sensor node: RP2350A / Pico 2 target using Pico SDK.
- Base station: RP2350A / Pico 2 target using Zephyr.

## UART Link / UART 通信

- Baud rate: `115200`
- Format: `8-N-1`
- Sensor-node UART: `uart0`
- Base-station UART: `uart0`
- Sensor-node `GPIO0` TX connects to base-station UART RX.
- Sensor-node `GPIO1` RX connects to base-station UART TX.
- Both boards must share GND.

## Sensor Node Sensors / Sensor Node 传感器

- FSR analog input: `GPIO26` / `ADC0`
- BME680 I2C bus: `i2c1`
- VEML7700 I2C bus: `i2c1`
- I2C SDA: `GPIO6`
- I2C SCL: `GPIO7`
- VEML7700 address: `0x10`
- BME680 addresses probed: `0x76`, `0x77`

## Event Interrupt Line / 事件中断线

- Sensor-node event output: `GPIO15`
- Base-station event input: `GPIO15`
- Active state: high
- Behavior: sensor-node drives the line high when the FSR event latch is set, and low after `CLEAR_EVENT`.

## FSR Event Defaults / FSR 事件默认值

- Default threshold: `1000` raw ADC units
- Hysteresis: `50` raw ADC units
- Event sample interval: `20 ms`

## Bring-Up Checklist / 上电检查

1. Connect GND between both boards.
2. Cross UART TX/RX between the boards.
3. Connect `GPIO15` to `GPIO15` for the event interrupt line.
4. Confirm BME680 and VEML7700 share `GPIO6/GPIO7` I2C with pull-ups.
5. Confirm FSR voltage divider output goes to `GPIO26`.
