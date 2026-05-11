# Test Plan / 验证计划

This document splits the project into two independent minimal verification tasks.
本文件将项目拆分为两个独立的最小验证任务。

## Task A: sensor-node minimal verification / 任务 A：sensor-node 最小验证

### Goal / 目标
Verify that the Pico SDK project can build, flash, and run with only a minimal main program.
验证 Pico SDK 工程可以在只有最小 main 程序的情况下完成编译、烧录和运行。

### Scope / 范围
- Pico SDK build only.
- 仅限 Pico SDK 构建。
- No business logic.
- 不实现业务功能。
- No communication protocol.
- 不实现通信协议。
- No sensor integration.
- 不接入传感器。
- Keep the current sensor-node structure intact.
- 保持现有 sensor-node 结构不变。

### Minimum visible behavior / 最小可见行为
- Print a startup message on serial if available, or stay in a minimal loop with clear TODO markers.
- 如串口可用，打印启动信息；否则保持最小循环并保留明确 TODO 标记。

### Success criteria / 成功标准
- The project configures and builds cleanly.
- 工程可以正常配置并编译。
- The firmware can be flashed to the target board.
- 固件可以烧录到目标板。
- The board runs the minimal firmware without entering a fault state.
- 板子可以运行最小固件且不进入故障状态。

### Verified hardware and build baseline / 已验证硬件与构建基线
- MCU: RP2350A
- MCU：RP2350A
- Zephyr board target: rpi_pico2/rp2350a/m33
- Zephyr 目标板：rpi_pico2/rp2350a/m33
- Verified configure/build command:

```bash
cd /Users/yiwenxu/Projects/coursework/PESP/base-station
rm -rf build
unset ZEPHYR_SDK_INSTALL_DIR
python3 -m west build -b rpi_pico2/rp2350a/m33 -s . -d build \
	-- -DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb -DGNUARMEMB_TOOLCHAIN_PATH=/opt/homebrew
```

- Output artifacts for flashing include: zephyr.uf2, zephyr.elf, zephyr.hex, zephyr.bin.
- 可烧录产物包括：zephyr.uf2、zephyr.elf、zephyr.hex、zephyr.bin。
- Available flash runners: openocd, probe-rs, jlink, uf2.
- 可用烧录 runner：openocd、probe-rs、jlink、uf2。

### Verified hardware and build baseline / 已验证硬件与构建基线
- MCU: RP2350A
- MCU：RP2350A
- Pico SDK target board: pico2
- Pico SDK 目标板：pico2
- Pico platform resolved by SDK: rp2350-arm-s
- Pico SDK 解析平台：rp2350-arm-s
- Verified configure/build command:

```bash
cd /Users/yiwenxu/Projects/coursework/PESP/sensor-node
rm -rf build
PICO_SDK_PATH=/Users/yiwenxu/softwares/pico-sdk cmake -S . -B build -G Ninja \
	-DCMAKE_C_COMPILER=/opt/homebrew/bin/arm-none-eabi-gcc \
	-DCMAKE_CXX_COMPILER=/opt/homebrew/bin/arm-none-eabi-g++ \
	-DPICO_PLATFORM=rp2350 \
	-DPICO_BOARD=pico2
cmake --build build
```

- Output artifacts for flashing include: sensor_node.uf2, sensor_node.elf, sensor_node.hex, sensor_node.bin.
- 可烧录产物包括：sensor_node.uf2、sensor_node.elf、sensor_node.hex、sensor_node.bin。

### Out of scope / 不在范围内
- UART protocol.
- UART 协议。
- Sensor drivers.
- 传感器驱动。
- Event detection.
- 事件检测。
- Interrupt signaling.
- 中断信号。

## Task B: base-station minimal verification / 任务 B：base-station 最小验证

### Goal / 目标
Verify that the Zephyr project can build, flash, and run with only a minimal main program.
验证 Zephyr 工程可以在只有最小 main 程序的情况下完成编译、烧录和运行。

### Scope / 范围
- Zephyr build only.
- 仅限 Zephyr 构建。
- No business logic.
- 不实现业务功能。
- No communication protocol.
- 不实现通信协议。
- No sensor integration.
- 不接入传感器。
- Keep the current base-station structure intact.
- 保持现有 base-station 结构不变。

### Minimum visible behavior / 最小可见行为
- Print a startup message on serial or provide another minimal visible behavior supported by the board.
- 通过串口打印启动信息，或使用目标板支持的其他最小可见行为。

### Success criteria / 成功标准
- The project configures and builds cleanly.
- 工程可以正常配置并编译。
- The firmware can be flashed to the target board.
- 固件可以烧录到目标板。
- The board runs the minimal firmware without entering a fault state.
- 板子可以运行最小固件且不进入故障状态。

### Out of scope / 不在范围内
- UART protocol.
- UART 协议。
- Driver wrappers.
- 驱动封装。
- Interrupt handling logic.
- 中断处理逻辑。
- Demo application features.
- Demo 应用功能。

## Task C: integrated proposal verification / 任务 C：proposal 集成验证

### Goal / 目标
Verify the proposal-level behavior across both boards.
验证 proposal 中要求的双板集成行为。

### Evidence record / 证据记录
- Use `docs/final-verification-evidence.md` as the final verification evidence checklist and result log.
- 使用 `docs/final-verification-evidence.md` 作为最终验证证据清单与结果记录。
- Record only observed command output, serial logs, photos, or measurements; leave hardware evidence as `not captured` until it is collected from the boards.
- 只记录实际观察到的命令输出、串口日志、照片或测量结果；在实板采集前，硬件证据保持 `not captured`。

### Wiring / 接线
- Cross UART0 TX/RX between the boards and share GND.
- 两块板之间交叉连接 UART0 TX/RX，并共地。
- Connect sensor-node GPIO15 to base-station GPIO15 for the event interrupt line.
- 将 sensor-node GPIO15 连接到 base-station GPIO15，作为事件中断线。
- Keep BME680 and VEML7700 on sensor-node I2C1 GPIO6/GPIO7.
- BME680 与 VEML7700 保持连接在 sensor-node I2C1 的 GPIO6/GPIO7。
- Connect the FSR voltage divider output to sensor-node GPIO26 / ADC0.
- 将 FSR 分压输出连接到 sensor-node GPIO26 / ADC0。

### Runtime checks / 运行检查
- Final base-station firmware is verified through the sensor-node USB console, because the current stable Zephyr image keeps USB CDC disabled.
- final base-station 固件通过 sensor-node USB console 验证，因为当前稳定 Zephyr 镜像关闭了 USB CDC。
- Sensor-node logs base-station runtime configuration commands: `SET_FORCE_THRESHOLD`, `SET_SAMPLING_RATE`, `ENABLE_INTERRUPT`, and `READ_CONFIG`.
- sensor-node 日志显示 base-station 发送运行时配置命令：`SET_FORCE_THRESHOLD`、`SET_SAMPLING_RATE`、`ENABLE_INTERRUPT` 和 `READ_CONFIG`。
- Sensor-node logs continuous base-station polling commands: `READ_ALL` and `READ_FORCE`.
- sensor-node 日志显示 base-station 持续发送轮询命令：`READ_ALL` 和 `READ_FORCE`。
- `READ_ALL` response payloads include `TEMP`, `HUM`, `PRESS`, `GAS`, `LIGHT`, `FORCE`, and `EVENT`; unavailable BME680 hardware readings are represented as `ERR`.
- `READ_ALL` 响应 payload 包含 `TEMP`、`HUM`、`PRESS`、`GAS`、`LIGHT`、`FORCE` 和 `EVENT`；不可用的 BME680 硬件读数以 `ERR` 表示。
- Pressing the FSR above the threshold drives sensor-node GPIO15 high.
- FSR 高于阈值时，sensor-node GPIO15 拉高。
- Base-station detects the GPIO15 event, sends `READ_EVENT`, logs the event, then sends `CLEAR_EVENT`.
- base-station 检测到 GPIO15 事件后发送 `READ_EVENT`，记录事件，再发送 `CLEAR_EVENT`。
- After `CLEAR_EVENT`, sensor-node GPIO15 returns low.
- `CLEAR_EVENT` 后，sensor-node GPIO15 回到低电平。
- If base-station prints `[warn] no sensor reply yet...`, the base firmware is transmitting but not receiving sensor-node replies; check sensor TX to base RX and common GND.
- 如果 base-station 打印 `[warn] no sensor reply yet...`，说明 base 固件正在发送但没有收到 sensor-node 回复；检查 sensor TX 到 base RX 以及共地。

### Success criteria / 成功标准
- Both firmware images build successfully.
- 两端固件均可成功构建。
- Protocol frame tests pass with `python3 tests/protocol_frame_test.py`; current coverage includes checksum definition, command/response frames, multiple-frame streams, checksum rejection, noise skipping, bad-frame recovery, and incomplete-frame rejection.
- 协议帧测试通过：`python3 tests/protocol_frame_test.py`；当前覆盖 checksum 定义、命令/响应帧、多帧流、错误 checksum 丢弃、噪声跳过、坏帧后恢复，以及不完整帧丢弃。
- Live serial output shows successful sensor data transfer.
- 串口输出显示传感器数据可以成功传输。
- FSR interaction triggers exactly one latched event until cleared.
- FSR 触发后事件会 latch，且清除前只保持同一个事件。
- Base-station clears the event latch without requiring a reboot.
- base-station 无需重启即可清除事件 latch。

## Independence rule / 独立性规则
- sensor-node and base-station must be verified separately.
- sensor-node 与 base-station 必须分开验证。
- Do not add shared runtime code between the two endpoints.
- 不要在两端之间加入共享运行时代码。
- Keep build instructions and outputs endpoint-specific.
- 构建说明与产物必须保持端侧独立。
