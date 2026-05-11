# Protocol / 通信协议

This project currently uses a framed UART protocol at 115200 8-N-1. The sensor-node still accepts newline-terminated text commands as a debug fallback.
当前项目使用带帧头的 115200 8-N-1 UART 协议。sensor-node 仍保留换行文本命令作为调试后门。

## Transport / 传输

- Physical link: UART0 between base-station and sensor-node.
- 物理链路：base-station 与 sensor-node 之间使用 UART0。
- Frame format: `0xA5 0x5A CMD LEN PAYLOAD CHECKSUM`.
- 帧格式：`0xA5 0x5A CMD LEN PAYLOAD CHECKSUM`。
- `CHECKSUM` is XOR over `CMD`, `LEN`, and every payload byte.
- `CHECKSUM` 为 `CMD`、`LEN` 与所有 payload 字节的 XOR。
- Payloads are ASCII strings so serial debug output remains readable.
- Payload 使用 ASCII 字符串，便于串口调试阅读。
- Resynchronization: receivers search for the next `0xA5 0x5A` header if a frame is incomplete or invalid.
- 重新同步：如果帧不完整或校验失败，接收端继续寻找下一个 `0xA5 0x5A` 帧头。

## Commands / 命令

- `0x01 PING`: returns `PONG`.
- `0x10 READ_FORCE`: returns current FSR ADC value and event latch state.
- `0x11 READ_ALL`: returns environmental readings, BME680 gas resistance, light, force, and event latch state.
- `0x12 READ_ENVIRONMENT`: returns BME680 temperature, humidity, pressure, and gas resistance.
- `0x13 READ_LIGHT`: returns VEML7700 raw ambient light value.
- `0x14 READ_EVENT`: returns latched FSR event details.
- `0x15 CLEAR_EVENT`: clears the latched event and lowers the interrupt line.
- `0x16 READ_CONFIG`: returns current sampling interval, force threshold, and interrupt enable state.
- `0x20 SET_SAMPLING_RATE`: payload is ASCII milliseconds, for example `2000`.
- `0x21 SET_FORCE_THRESHOLD`: payload is ASCII raw ADC threshold, for example `1000`.
- `0x22 ENABLE_INTERRUPT`: payload is ASCII `0` or `1`.
- `0x7E BASE_BOOTSEL`: maintenance frame with payload `BOOTSEL`; sensor-node can forward this from its USB console so base-station enters the ROM BOOTSEL loader for automatic reflashing.

## Response Examples / 响应示例

```text
PONG
FORCE=27,EVENT=0
TEMP=27.77,HUM=15.43,PRESS=1021.46,GAS=180000,LIGHT=4733,FORCE=31,EVENT=0
TEMP=25.03,HUM=14.96,PRESS=1002.91,GAS=176702,LIGHT=4685,FORCE=27,EVENT=0
EVENT=1,FORCE=3880,EVENT_FORCE=3892,AGE_MS=120,THRESH=1000
OK,CLEAR_EVENT
OK,FORCE_THRESHOLD=1000
```

Response frames set bit `0x80` on the command ID. For example, `READ_ALL` command `0x11` returns response type `0x91`.
响应帧会在命令 ID 上设置 `0x80`。例如 `READ_ALL` 命令 `0x11` 的响应类型是 `0x91`。

## Event Flow / 事件流程

1. Sensor-node samples the FSR regularly.
2. If the raw ADC value crosses `FORCE_THRESHOLD`, sensor-node latches an event.
3. If interrupts are enabled, sensor-node drives `GPIO15` high.
4. Base-station detects `GPIO15`, sends `READ_EVENT`, logs the event, then sends `CLEAR_EVENT`.
5. Sensor-node clears the latch and drives `GPIO15` low.

## Notes / 备注

- `PRESS` is reported in hPa/mbar over UART. Local sensor-node debug prints BME680 pressure in Pa.
- `PRESS` 在 UART 响应中使用 hPa/mbar；sensor-node 本地调试日志中 BME680 气压使用 Pa。
- `GAS` is reported in ohms when the BME680 heater is stable and the gas-valid bit is set; otherwise the payload uses `GAS=ERR`.
- `GAS` 在 BME680 heater stable 且 gas-valid bit 有效时以 ohm 输出；否则 payload 使用 `GAS=ERR`。
- Text commands such as `READ_ALL\n` remain accepted by sensor-node for manual debugging, but base-station uses framed commands by default.
- sensor-node 仍接受 `READ_ALL\n` 这类文本命令用于手动调试，但 base-station 默认使用 framed commands。
- Sending `BASE_BOOTSEL\n` to the sensor-node USB console asks sensor-node to forward the `0x7E` maintenance frame to base-station.
- 向 sensor-node USB console 发送 `BASE_BOOTSEL\n` 会让 sensor-node 向 base-station 转发 `0x7E` 维护帧。
