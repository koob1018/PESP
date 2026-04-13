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
