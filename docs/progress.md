# Progress Against Proposal / Proposal 当前进度

This file maps the proposal goals to the current implementation state.
本文件将 proposal 目标映射到当前实现状态。

## Completed / 已完成

- Dual-end project split: `sensor-node` uses Pico SDK, `base-station` uses Zephyr.
- 双端工程分离：`sensor-node` 使用 Pico SDK，`base-station` 使用 Zephyr。
- UART communication between sensor-node and base-station.
- 两端 UART 通信。
- FSR ADC reading on `GPIO26`.
- `GPIO26` 上的 FSR ADC 读取。
- VEML7700 ambient light reading on I2C.
- I2C 上的 VEML7700 环境光读取。
- BME680 temperature, humidity, pressure, and gas payload support in firmware.
- 固件中已支持 BME680 温度、湿度、气压与气体电阻 payload。
- Runtime configuration commands for sampling interval, force threshold, and interrupt enable.
- 运行时配置命令：采样间隔、FSR 阈值、中断开关。
- FSR threshold event detection with hysteresis and event latch.
- 带 hysteresis 和 latch 的 FSR 阈值事件检测。
- Dedicated event interrupt GPIO from sensor-node to base-station.
- sensor-node 到 base-station 的独立事件 GPIO 中断线。
- Base-station demo flow: configure sensor-node, poll values, react to interrupt, read event, clear latch.
- Base-station demo 流程：配置 sensor-node、轮询数据、响应中断、读取事件、清除 latch。
- Zephyr driver abstraction is active in the base-station build through `src/driver/sensor_node_driver.c/.h`.
- Zephyr driver 抽象已通过 `src/driver/sensor_node_driver.c/.h` 接回 base-station 当前构建。
- Framed UART protocol with header, command ID, payload length, ASCII payload, and XOR checksum.
- 带帧头、命令 ID、payload 长度、ASCII payload 和 XOR checksum 的 UART 帧协议。
- Basic automated protocol frame tests.
- 基础协议帧自动化测试。
- Final hardware-in-loop evidence for runtime configuration, polling, and FSR event clear flow.
- 已采集运行时配置、周期轮询与 FSR 事件清除流程的最终实板闭环证据。
- Automatic base-station flashing path through sensor-node USB command forwarding and base-station UART maintenance frame.
- 已加入通过 sensor-node USB 命令转发和 base-station UART 维护帧实现的 base-station 自动烧录路径。
- `./flash-base.sh` verified end-to-end: requests base BOOTSEL through sensor-node, checks base chipid, loads UF2, verifies flash, and reboots base-station.
- `./flash-base.sh` 已完成端到端验证：经 sensor-node 请求 base 进入 BOOTSEL、检查 base chipid、烧录 UF2、校验 flash，并重启 base-station。
- Minimal sensor-node serial diagnostics for demo evidence, including `[link-rx]`, `[config]`, `[sample]`, and `[event]`.
- 保留极简 sensor-node 串口诊断用于演示证据，包括 `[link-rx]`、`[config]`、`[sample]` 和 `[event]`。

## Partially Complete / 部分完成

- BME680 hardware evidence: firmware support is present, but the final board capture returned `env=ERR`; the current run did not verify live BME680 environmental readings.
- BME680 硬件证据：固件支持已具备，但 final 实板采集返回 `env=ERR`；当前运行未验证到实时 BME680 环境读数。

## Remaining / 剩余工作

- Optional: check BME680 wiring/module readiness if the final live demo must show non-`ERR` environmental values.
- 可选：如果最终现场演示必须显示非 `ERR` 的环境读数，检查 BME680 接线或模块状态。
- Prepare final report/presentation material from the proposal, implementation notes, and captured serial logs.
- 基于 proposal、实现说明和已采集串口日志整理最终报告/展示材料。
