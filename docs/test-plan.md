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

## Independence rule / 独立性规则
- sensor-node and base-station must be verified separately.
- sensor-node 与 base-station 必须分开验证。
- Do not add shared runtime code between the two endpoints.
- 不要在两端之间加入共享运行时代码。
- Keep build instructions and outputs endpoint-specific.
- 构建说明与产物必须保持端侧独立。
