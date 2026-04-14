# WORKLOG / 变更日志

This file records every scoped change made in this project.
本文件用于记录本项目每一次范围明确的改动。

## Rules / 记录规则

- Update this file after each completed change.
- 每次完成一次改动后都要更新本文件。
- Keep entries concise, factual, and scoped.
- 条目保持简洁、客观、范围明确。
- Use bilingual notes (CN + EN).
- 使用中英双语记录。

## Entry Template / 条目模板

### [YYYY-MM-DD] Task: <short title>
- Scope (EN):
- 范围（中文）：
- Changes (EN):
- 改动（中文）：
- Validation (EN):
- 验证（中文）：
- Notes (EN):
- 备注（中文）：

## Entries / 历史记录

### [2026-04-14] Task: Add guide constraints and worklog baseline
- Scope (EN): Project process documentation only; no business logic implementation.
- 范围（中文）：仅调整项目流程文档；不实现业务功能。
- Changes (EN): Added Section 11 in docs/PROJECT_GUIDE.md for collaboration and execution rules; created docs/WORKLOG.md as bilingual change log.
- 改动（中文）：在 docs/PROJECT_GUIDE.md 新增第 11 节协作与执行规则；创建 docs/WORKLOG.md 作为中英双语变更日志。
- Validation (EN): Confirmed both files exist and content is updated.
- 验证（中文）：已确认两个文件存在且内容已更新。
- Notes (EN): Keep PROJECT_GUIDE.md under docs/ as the authoritative rule source.
- 备注（中文）：继续将 docs/PROJECT_GUIDE.md 作为权威规则来源。

### [2026-04-14] Task: Create minimal dual-end project skeletons
- Scope (EN): Structural setup only for sensor-node (Pico SDK) and base-station (Zephyr); no business feature implementation.
- 范围（中文）：仅搭建 sensor-node（Pico SDK）与 base-station（Zephyr）的结构骨架；不实现业务功能。
- Changes (EN): Created sensor-node and base-station directories with required src subfolders; added minimal CMake and main entry files; added TODO markers for unresolved hardware parameters.
- 改动（中文）：创建 sensor-node 与 base-station 目录及要求的 src 子目录；补充最小 CMake 与 main 入口文件；对未确定硬件参数使用 TODO 标记。
- Validation (EN): Verified required files and folder layout are present and separated by endpoint.
- 验证（中文）：已核对所需文件与目录结构存在，且双端内容保持分离。
- Notes (EN): Keep implementing one small task at a time and update this worklog after each change.
- 备注（中文）：后续继续按“一次一个小任务”推进，并在每次改动后更新本日志。

### [2026-04-14] Task: Install VS Code extensions for Pico and Zephyr workflow
- Scope (EN): Development environment setup only; no project source code changes.
- 范围（中文）：仅进行开发环境安装；不改动项目业务源码。
- Changes (EN): Installed missing extensions via VS Code CLI path: nordic-semiconductor.nrf-devicetree, nordic-semiconductor.nrf-kconfig, mylonics.zephyr-ide. Confirmed ms-vscode.cmake-tools, ms-vscode.cpptools, and ms-python.python were already installed.
- 改动（中文）：通过 VS Code CLI 绝对路径安装缺失扩展：nordic-semiconductor.nrf-devicetree、nordic-semiconductor.nrf-kconfig、mylonics.zephyr-ide。并确认 ms-vscode.cmake-tools、ms-vscode.cpptools、ms-python.python 已预装。
- Validation (EN): CLI output confirmed successful installation and existing extension status.
- 验证（中文）：命令输出已确认安装成功与已安装状态。
- Notes (EN): The plain `code` command was not found in PATH; used the absolute app binary path on macOS.
- 备注（中文）：终端未配置 `code` 命令，已使用 macOS 下 VS Code 绝对路径完成安装。

### [2026-04-14] Task: Complete local Pico + Zephyr toolchain installation
- Scope (EN): Local host environment setup and SDK initialization only; no application feature changes.
- 范围（中文）：仅完成本机工具链与 SDK 初始化；不涉及应用功能改动。
- Changes (EN): Installed Homebrew dependencies, ARM GNU embedded toolchain, west and Python dependencies; cloned Pico SDK to ~/dev/pico/pico-sdk; initialized Zephyr workspace at ~/dev/zephyrproject; extracted and set up Zephyr SDK 0.17.0; updated ~/.zshrc with PATH, PICO_SDK_PATH, ZEPHYR_BASE, and ZEPHYR_SDK_INSTALL_DIR.
- 改动（中文）：安装 Homebrew 依赖、ARM GNU 嵌入式工具链、west 与 Python 依赖；将 Pico SDK 拉取到 ~/dev/pico/pico-sdk；初始化 Zephyr 工作区到 ~/dev/zephyrproject；解压并完成 Zephyr SDK 0.17.0 初始化；在 ~/.zshrc 写入 PATH、PICO_SDK_PATH、ZEPHYR_BASE、ZEPHYR_SDK_INSTALL_DIR。
- Validation (EN): Verified command availability for code, west, arm-none-eabi-gcc, and Zephyr SDK compiler arm-zephyr-eabi-gcc.
- 验证（中文）：已验证 code、west、arm-none-eabi-gcc 与 Zephyr SDK 编译器 arm-zephyr-eabi-gcc 可用。
- Notes (EN): One interrupted install attempt was recovered by fixing SDK archive naming and completing setup in separate steps.
- 备注（中文）：中途出现一次安装中断，已通过修正 SDK 包名并分步执行完成恢复。

### [2026-04-14] Task: Relocate SDK and workspace paths to ~/softwares
- Scope (EN): Directory relocation and environment variable retargeting only; no project code changes.
- 范围（中文）：仅进行目录迁移与环境变量重定向；不改动项目代码。
- Changes (EN): Moved pico-sdk, zephyrproject workspace, and zephyr-sdk-0.17.0 into ~/softwares; updated ~/.zshrc exports for PICO_SDK_PATH, ZEPHYR_BASE, and ZEPHYR_SDK_INSTALL_DIR.
- 改动（中文）：将 pico-sdk、zephyrproject 工作区和 zephyr-sdk-0.17.0 全部迁移到 ~/softwares；更新 ~/.zshrc 中的 PICO_SDK_PATH、ZEPHYR_BASE、ZEPHYR_SDK_INSTALL_DIR。
- Validation (EN): Confirmed new directories exist and toolchain commands still work (west, arm-none-eabi-gcc, arm-zephyr-eabi-gcc).
- 验证（中文）：已确认新目录存在且工具链命令仍可用（west、arm-none-eabi-gcc、arm-zephyr-eabi-gcc）。
- Notes (EN): Homebrew-managed binaries remain in /opt/homebrew and were not moved.
- 备注（中文）：Homebrew 管理的可执行文件仍保留在 /opt/homebrew，未进行迁移。

### [2026-04-14] Task: Split project into two minimal verification tasks
- Scope (EN): Documentation only; no firmware feature changes.
- 范围（中文）：仅更新文档；不改动固件功能。
- Changes (EN): Added docs/test-plan.md with separate sensor-node and base-station verification tasks; added minimal endpoint READMEs in sensor-node/ and base-station/.
- 改动（中文）：新增 docs/test-plan.md，将 sensor-node 与 base-station 的验证任务拆分；在 sensor-node/ 与 base-station/ 下补充最小说明文档。
- Validation (EN): Confirmed the new documentation files exist in their respective endpoint or docs directories.
- 验证（中文）：已确认新增文档文件存在于各自端目录或 docs 目录中。
- Notes (EN): The two tasks remain independent and exclude business logic, communication, sensors, and drivers.
- 备注（中文）：两个任务保持独立，且不包含业务逻辑、通信、传感器和驱动。

### [2026-04-14] Task: Lock sensor-node RP2350A verification baseline
- Scope (EN): sensor-node minimal verification baseline only; no protocol/sensor/event implementation.
- 范围（中文）：仅固化 sensor-node 最小验证基线；不实现协议、传感器与事件逻辑。
- Changes (EN): Rebuilt sensor-node for RP2350A using Pico board pico2 and platform rp2350-arm-s; added verified hardware model and build command to docs/test-plan.md.
- 改动（中文）：以 RP2350A 为目标（Pico 板型 pico2，平台 rp2350-arm-s）重新完成 sensor-node 构建；并将已验证型号与编译命令写入 docs/test-plan.md。
- Validation (EN): Build completed successfully and generated flashable outputs (sensor_node.uf2/elf/hex/bin).
- 验证（中文）：构建成功并生成可烧录产物（sensor_node.uf2/elf/hex/bin）。
- Notes (EN): Future hardware-specific tasks must ask for exact model/board details before implementation.
- 备注（中文）：后续涉及硬件细节的任务，需先确认具体型号与板型后再实施。

### [2026-04-14] Task: Verify base-station minimal Zephyr build on RP2350A
- Scope (EN): base-station environment verification only; no protocol/driver/feature implementation.
- 范围（中文）：仅进行 base-station 环境验证；不实现协议、驱动和功能逻辑。
- Changes (EN): Built base-station with board target rpi_pico2/rp2350a/m33; recorded verified command, artifact list, and runner options in docs/test-plan.md.
- 改动（中文）：以 rpi_pico2/rp2350a/m33 目标完成 base-station 构建；并将验证命令、产物清单与 runner 信息写入 docs/test-plan.md。
- Validation (EN): Build succeeded and generated flashable artifacts (zephyr.uf2/elf/hex/bin).
- 验证（中文）：构建成功并生成可烧录产物（zephyr.uf2/elf/hex/bin）。
- Notes (EN): For this workspace state, unsetting ZEPHYR_SDK_INSTALL_DIR avoids a Zephyr SDK version compatibility check and allows gnuarmemb toolchain usage.
- 备注（中文）：在当前环境中，先 unset ZEPHYR_SDK_INSTALL_DIR 可绕过 Zephyr SDK 版本兼容检查，并使用 gnuarmemb 工具链完成构建。

### [2026-04-14] Task: Fix base-station no-output issue with USB CDC console
- Scope (EN): base-station runtime visibility fix only; no protocol or business feature changes.
- 范围（中文）：仅修复 base-station 运行可见性问题；不涉及协议或业务功能。
- Changes (EN): Switched console setup to Zephyr 4.4 USB Device Stack Next + CDC ACM; added app.overlay to bind zephyr,console to cdc_acm_uart0; updated main to wait for DTR and print periodic heartbeat.
- 改动（中文）：将控制台切换为 Zephyr 4.4 的 USB Device Stack Next + CDC ACM；新增 app.overlay 将 zephyr,console 绑定到 cdc_acm_uart0；main 改为等待 DTR 后输出并周期打印心跳。
- Validation (EN): Rebuild succeeded for rpi_pico2/rp2350a/m33 and generated zephyr.uf2/zephyr.elf; Kconfig check confirms USB CDC console-related symbols are enabled.
- 验证（中文）：在 rpi_pico2/rp2350a/m33 目标下重建成功并生成 zephyr.uf2/zephyr.elf；Kconfig 检查确认 USB CDC 控制台相关配置已生效。
- Notes (EN): Previous no-output symptom was caused by using UART console defaults without external UART wiring.
- 备注（中文）：此前“无输出”主要因为使用了 UART 控制台默认路径且未接外部 UART。

### [2026-04-14] Task: Implement minimal one-way UART link validation
- Scope (EN): Minimal UART communication test only; no frame/payload/checksum protocol and no sensor/event logic.
- 范围（中文）：仅实现最小 UART 通信验证；不实现 frame/payload/checksum 协议，不涉及传感器和事件逻辑。
- Changes (EN): sensor-node now initializes UART0 (GPIO0/1, 115200) and sends fixed "PING\n" periodically; base-station polls UART0 and prints received lines on USB console.
- 改动（中文）：sensor-node 新增 UART0（GPIO0/1，115200）初始化并周期发送固定字符串 "PING\n"；base-station 轮询接收 UART0 字节并在 USB 控制台打印整行。
- Validation (EN): Both endpoints rebuilt successfully and generated flashable artifacts (sensor_node.uf2, zephyr.uf2).
- 验证（中文）：两端重建成功并生成可烧录产物（sensor_node.uf2、zephyr.uf2）。
- Notes (EN): This step validates one-way link first, keeping implementation intentionally minimal.
- 备注（中文）：本步骤先验证单向链路，保持最小实现。
