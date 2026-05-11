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

### [2026-05-11] Task: Rerun final build, flashing, and runtime plan
- Scope (EN): End-to-end final verification rerun only.
- 范围（中文）：仅最终端到端验证复跑。
- Changes (EN): Updated final verification evidence with the latest full-plan rerun results.
- 改动（中文）：将最新完整计划复跑结果写入最终验证证据。
- Validation (EN): `cmake --build sensor-node/build` passed with no rebuild needed; base-station `west build` passed and regenerated `zephyr.uf2`; `python3 tests/protocol_frame_test.py` passed 10 tests; `bash -n flash-base.sh flash-pico.sh` passed. Sensor-node auto flash completed with verify OK using `picotool load -v -x ... --ser 0DD161C1C0BA0642 -f`. Base-station auto flash completed through `./flash-base.sh`: requested BOOTSEL through sensor-node, verified base chipid `0x3cff0b54acc69f66`, loaded and verified the Zephyr UF2, then rebooted. Post-flash serial capture showed continuing `READ_ALL`/`READ_FORCE` polling and live FSR/light samples.
- 验证（中文）：`cmake --build sensor-node/build` 通过且无需重建；base-station `west build` 通过并重新生成 `zephyr.uf2`；`python3 tests/protocol_frame_test.py` 通过 10 个测试；`bash -n flash-base.sh flash-pico.sh` 通过。sensor-node 通过 `picotool load -v -x ... --ser 0DD161C1C0BA0642 -f` 自动烧录并 verify OK。base-station 通过 `./flash-base.sh` 自动烧录：经 sensor-node 请求 BOOTSEL、验证 base chipid `0x3cff0b54acc69f66`、烧录并校验 Zephyr UF2、随后重启。烧录后串口采集显示 `READ_ALL`/`READ_FORCE` 持续轮询，FSR 与光照采样正常。
- Notes (EN): BME680 still reports `env=ERR` on the connected hardware during this rerun.
- 备注（中文）：本次复跑中，连接的硬件上 BME680 仍返回 `env=ERR`。

### [2026-05-11] Task: Verify automatic base-station flashing
- Scope (EN): Base-station automatic flashing path, firmware maintenance command, and final verification evidence.
- 范围（中文）：base-station 自动烧录路径、固件维护命令与最终验证证据。
- Changes (EN): Added a sensor-node USB command `BASE_BOOTSEL` that forwards a framed UART maintenance command to base-station. Added a base-station maintenance frame handler for command `0x7E` payload `BOOTSEL`, which calls `reset_usb_boot(0, 0)`. Updated `flash-base.sh` to request BOOTSEL through sensor-node, verify the base chipid, load the Zephyr UF2, and reboot. Updated protocol, progress, README, and evidence docs.
- 改动（中文）：新增 sensor-node USB 命令 `BASE_BOOTSEL`，用于向 base-station 转发 framed UART 维护命令；新增 base-station 维护帧处理，命令 `0x7E`、payload `BOOTSEL` 时调用 `reset_usb_boot(0, 0)`；更新 `flash-base.sh`，使其通过 sensor-node 请求 BOOTSEL、验证 base chipid、烧录 Zephyr UF2 并重启；同步更新 protocol、progress、README 和 evidence 文档。
- Validation (EN): Sensor-node build passed; base-station build passed; `python3 tests/protocol_frame_test.py` passed 10 tests; `bash -n flash-base.sh flash-pico.sh` passed. After one manual install of the maintenance-capable base firmware, `./flash-base.sh` completed without manual intervention: base entered BOOTSEL, `picotool` verified chipid `0x3cff0b54acc69f66`, loaded and verified `base-station/build/zephyr/zephyr.uf2`, then rebooted base-station. Post-flash sensor-node logs show normal `READ_ALL`/`READ_FORCE` polling.
- 验证（中文）：sensor-node 构建通过；base-station 构建通过；`python3 tests/protocol_frame_test.py` 通过 10 个测试；`bash -n flash-base.sh flash-pico.sh` 通过。在手动安装一次带维护通道的 base 固件后，`./flash-base.sh` 已无需手动介入完整跑通：base 进入 BOOTSEL，`picotool` 校验 chipid `0x3cff0b54acc69f66`，烧录并校验 `base-station/build/zephyr/zephyr.uf2`，随后重启 base-station。自动烧录后 sensor-node 日志显示 `READ_ALL`/`READ_FORCE` 正常轮询。
- Notes (EN): Future base-station updates can use `./flash-base.sh` as long as sensor-node remains connected on the configured USB serial port and the board serial numbers stay unchanged.
- 备注（中文）：后续 base-station 更新可直接使用 `./flash-base.sh`，前提是 sensor-node 仍连接在配置的 USB 串口且板卡序列号不变。

### [2026-05-11] Task: Flash and verify final dual-board integration
- Scope (EN): Final source cleanup, dual-board flashing, hardware-in-loop verification, and evidence documentation.
- 范围（中文）：最终源码清理、双板烧录、实板闭环验证与证据文档。
- Changes (EN): Removed temporary byte-level and diagnostic UART frames used during bring-up. Kept concise sensor-node `[link-rx]`, `[config]`, `[sample]`, and `[event]` logs for demo evidence. Updated the base flashing path to require explicit base-station BOOTSEL serial selection. Replaced stale verification notes with final build, flash, polling, runtime configuration, and FSR event-latch evidence.
- 改动（中文）：移除 bring-up 阶段使用的逐字节调试输出和临时 UART 诊断帧；保留 sensor-node 简洁的 `[link-rx]`、`[config]`、`[sample]` 与 `[event]` 日志作为演示证据；将 base 烧录路径改为显式选择 base-station BOOTSEL 序列号；用最终构建、烧录、轮询、运行时配置和 FSR event latch 证据替换旧验证说明。
- Validation (EN): `cmake --build sensor-node/build` passed; `env -u ZEPHYR_SDK_INSTALL_DIR west build -d base-station/build base-station -- -UZEPHYR_SDK_INSTALL_DIR -DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb -DGNUARMEMB_TOOLCHAIN_PATH=/opt/homebrew` passed; `python3 tests/protocol_frame_test.py` passed 9 tests. Sensor-node flashed with `picotool load -v -x ... --ser 0DD161C1C0BA0642 -f`; base-station flashed with `picotool load -v ... --ser 3CFF0B54ACC69F66`; both verified OK. Final hardware log shows base-station sends runtime config, continuous `READ_ALL`/`READ_FORCE`, `READ_EVENT`, and `CLEAR_EVENT`.
- 验证（中文）：`cmake --build sensor-node/build` 通过；`env -u ZEPHYR_SDK_INSTALL_DIR west build -d base-station/build base-station -- -UZEPHYR_SDK_INSTALL_DIR -DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb -DGNUARMEMB_TOOLCHAIN_PATH=/opt/homebrew` 通过；`python3 tests/protocol_frame_test.py` 通过 9 个测试。sensor-node 已通过 `picotool load -v -x ... --ser 0DD161C1C0BA0642 -f` 烧录；base-station 已通过 `picotool load -v ... --ser 3CFF0B54ACC69F66` 烧录；两者均 verify OK。最终实板日志显示 base-station 会发送运行时配置、持续 `READ_ALL`/`READ_FORCE`、`READ_EVENT` 和 `CLEAR_EVENT`。
- Notes (EN): The current hardware run verifies FSR, VEML7700 light, UART framing, runtime config, and event handling. BME680 support remains in firmware, but this final hardware capture returned `env=ERR`, so BME680 wiring/module readiness should be checked if environmental readings are required for the live demo.
- 备注（中文）：当前硬件运行已验证 FSR、VEML7700 光照、UART framing、运行时配置与事件处理。BME680 支持仍在固件中，但本次 final 实板采集返回 `env=ERR`；如果现场演示必须显示环境读数，需要检查 BME680 接线或模块状态。

### [2026-05-11] Task: Add base-station BOOTSEL maintenance command
- Scope (EN): Base-station update ergonomics and flashing scripts.
- 范围（中文）：base-station 更新便利性与烧录脚本。
- Changes (EN): Added a USB CDC maintenance command parser to the Zephyr base-station app. Sending `BOOTSEL` or `REBOOT_BOOTSEL` over the base USB serial console calls `reset_usb_boot(0, 0)` and enters the RP2350 ROM bootloader. Added `flash-base.sh` to automate future base-station updates by sending `BOOTSEL`, waiting for BOOTSEL, loading `base-station/build/zephyr/zephyr.uf2`, and rebooting. Documented the one-time manual BOOTSEL requirement for installing this new maintenance-capable image.
- 改动（中文）：在 Zephyr base-station app 中加入 USB CDC 维护命令解析；通过 base USB 串口发送 `BOOTSEL` 或 `REBOOT_BOOTSEL` 会调用 `reset_usb_boot(0, 0)` 并进入 RP2350 ROM bootloader。新增 `flash-base.sh`，用于后续自动发送 `BOOTSEL`、等待 BOOTSEL、烧录 `base-station/build/zephyr/zephyr.uf2` 并重启。文档中记录首次安装该维护能力镜像时仍需手动进入 BOOTSEL。
- Validation (EN): Base-station build passed and regenerated `base-station/build/zephyr/zephyr.uf2`; protocol frame tests still pass 9 tests.
- 验证（中文）：base-station 构建通过并重新生成 `base-station/build/zephyr/zephyr.uf2`；协议帧测试仍通过 9 个测试。
- Notes (EN): Runtime verification of the `BOOTSEL` command requires flashing this image once through manual BOOTSEL first.
- 备注（中文）：`BOOTSEL` 命令的运行时验证需要先通过手动 BOOTSEL 将该镜像刷入一次。

### [2026-05-11] Task: Add BME680 gas output and reactivate Zephyr driver demo
- Scope (EN): Sensor-node gas measurement, base-station Zephyr driver build path, documentation, and verification evidence.
- 范围（中文）：sensor-node 气体测量、base-station Zephyr driver 构建路径、文档与验证证据。
- Changes (EN): Added BME680 gas-resistance compensation and `bme680_read_all_min()`. Updated sensor-node `READ_ENVIRONMENT`, `READ_ALL`, and sample logs to include `GAS`. Reconnected `src/driver/sensor_node_driver.c/.h` into the base-station build and changed `main.c` into a small driver demo that configures runtime settings, polls data, handles GPIO15 events, and clears event latches. Updated README/protocol/progress/test/evidence documents to match the active driver design and current flashing status.
- 改动（中文）：新增 BME680 气体电阻补偿与 `bme680_read_all_min()`；更新 sensor-node 的 `READ_ENVIRONMENT`、`READ_ALL` 与采样日志，加入 `GAS`；将 `src/driver/sensor_node_driver.c/.h` 接回 base-station 构建，并把 `main.c` 改为小型 driver demo，用于配置运行时参数、轮询数据、处理 GPIO15 事件并清除 latch；同步更新 README、protocol、progress、test 与 evidence 文档，反映当前 driver 设计与烧录状态。
- Validation (EN): `python3 tests/protocol_frame_test.py` passed 9 tests; sensor-node build passed; base-station build passed with `zephyr.uf2` regenerated. Sensor-node was flashed with `picotool` and live serial logs show `gas=...ohm`; the currently flashed base-station direct baseline receives `GAS=...` over UART.
- 验证（中文）：`python3 tests/protocol_frame_test.py` 通过 9 个测试；sensor-node 构建通过；base-station 构建通过并重新生成 `zephyr.uf2`；sensor-node 已通过 `picotool` 烧录，实时串口日志显示 `gas=...ohm`；当前已刷入的 base-station direct 基线可通过 UART 收到 `GAS=...`。
- Notes (EN): Flashing the new base-station driver image is blocked on this host because no SWD debug probe is visible, `probe-rs`/J-Link tools are unavailable, and the installed OpenOCD setup lacks `target/rp2350.cfg`.
- 备注（中文）：当前主机无法继续刷入新版 base-station driver 镜像，因为看不到 SWD 调试探针，`probe-rs`/J-Link 工具不可用，且已安装的 OpenOCD 缺少 `target/rp2350.cfg`。

### [2026-05-11] Task: Add final verification evidence scaffolding
- Scope (EN): Tests and documentation evidence only; no sensor-node or base-station source changes.
- 范围（中文）：仅测试与文档证据；不修改 sensor-node 或 base-station 源码。
- Changes (EN): Expanded protocol frame tests from 4 to 9 cases covering checksum definition, response frames, multiple frames, bad checksum rejection, noise skipping, bad-frame recovery, and incomplete-frame rejection. Added `docs/final-verification-evidence.md` to separate observed local test evidence from hardware logs that still need capture. Updated progress and test plan references to point at the evidence checklist.
- 改动（中文）：将协议帧测试从 4 个扩展到 9 个，覆盖 checksum 定义、响应帧、多帧、错误 checksum 丢弃、噪声跳过、坏帧后恢复，以及不完整帧丢弃。新增 `docs/final-verification-evidence.md`，区分已观察到的本地测试证据和尚未采集的硬件日志。更新 progress 与 test plan 中的证据清单引用。
- Validation (EN): `python3 tests/protocol_frame_test.py` passed 9 tests.
- 验证（中文）：`python3 tests/protocol_frame_test.py` 通过 9 个测试。
- Notes (EN): No hardware serial logs were observed or claimed in this entry.
- 备注（中文）：本条目未观察也未声称任何硬件串口日志。

### [2026-04-27] Task: Restore direct base-station baseline
- Scope (EN): Stabilize base-station bring-up by removing the custom driver path from the active build.
- 范围（中文）：通过从当前构建中移除 custom driver 路径，恢复 base-station 稳定 bring-up 基线。
- Changes (EN): Changed `base-station/CMakeLists.txt` to compile only `src/main.c`. Replaced the driver-based demo with a direct single-file Zephyr app that handles framed UART, GPIO15 event polling, runtime configuration, periodic reads, and event clear flow. Removed DTR waiting, heartbeat debug, LED debug, and driver-ready checks from the active path. Kept the experimental `src/driver/` files parked but inactive.
- 改动（中文）：修改 `base-station/CMakeLists.txt`，当前只编译 `src/main.c`；将 driver-based demo 替换为 direct 单文件 Zephyr app，由 main 直接处理 framed UART、GPIO15 事件轮询、运行时配置、周期读取和事件清除；从当前路径移除 DTR 等待、heartbeat debug、LED debug 和 driver-ready 检查；实验性的 `src/driver/` 文件保留但不参与构建。
- Validation (EN): Reconfigured and rebuilt base-station successfully; generated `base-station/build/zephyr/zephyr.uf2` at 85 KB. Protocol frame tests still pass 4 tests.
- 验证（中文）：base-station 重新配置并构建成功；生成 85 KB 的 `base-station/build/zephyr/zephyr.uf2`；协议帧测试仍通过 4 个测试。
- Notes (EN): The expected startup line is now `[app] base-station direct demo start`.
- 备注（中文）：现在期望启动行是 `[app] base-station direct demo start`。

### [2026-04-27] Task: Enable base-station UART/GPIO devicetree nodes
- Scope (EN): Zephyr devicetree configuration for the base-station sensor-node driver.
- 范围（中文）：base-station sensor-node driver 所需的 Zephyr devicetree 配置。
- Changes (EN): Updated `base-station/app.overlay` to enable `uart0` for the board-to-board link and `gpio0` for the GPIO15 event interrupt input. Added `CONFIG_GPIO=y` and `CONFIG_PRINTK_SYNC=y` to `prj.conf`. Moved UART/GPIO readiness checks from Zephyr device-registration time into `sensor_node_driver_start()` to avoid early init-order failures, removed the demo app's premature `device_is_ready()` check on the custom driver device, removed the USB CDC DTR wait entirely, and added startup and heartbeat markers plus LED heartbeat.
- 改动（中文）：更新 `base-station/app.overlay`，启用用于板间通信的 `uart0`，以及用于 GPIO15 事件中断输入的 `gpio0`；在 `prj.conf` 中加入 `CONFIG_GPIO=y` 和 `CONFIG_PRINTK_SYNC=y`；将 UART/GPIO ready 检查从 Zephyr device 注册阶段挪到 `sensor_node_driver_start()`，避免早期初始化顺序导致失败；移除 demo app 对 custom driver device 的过早 `device_is_ready()` 检查；完全移除 USB CDC DTR 等待，并新增启动标记、heartbeat 串口输出和 LED heartbeat。
- Validation (EN): Reconfigured base-station and confirmed generated devicetree marks `uart0` and `gpio0` as `okay`; base-station build passed and regenerated `zephyr.uf2`; protocol frame tests still pass 4 tests.
- 验证（中文）：重新配置 base-station，并确认生成的 devicetree 中 `uart0` 和 `gpio0` 均为 `okay`；base-station 构建通过并重新生成 `zephyr.uf2`；协议帧测试仍通过 4 个测试。
- Notes (EN): The new firmware prints `[app] base-station demo start v20260427-no-dtr` and `[app] heartbeat`; seeing only the Zephyr boot banner means either the new UF2 is not running or the app is not reaching main.
- 备注（中文）：新固件会打印 `[app] base-station demo start v20260427-no-dtr` 和 `[app] heartbeat`；如果只看到 Zephyr boot banner，说明新版 UF2 没有运行，或 app 没有进入 main。

### [2026-04-27] Task: Expose runtime configuration API and no-reply diagnostics
- Scope (EN): Base-station Zephyr driver API completeness and hardware-link debugging.
- 范围（中文）：base-station Zephyr driver API 完整性与硬件链路调试。
- Changes (EN): Exposed force threshold, sampling interval, interrupt enable, and config readback as public `sensor_node_driver` APIs. Updated the demo app to call those APIs at startup while servicing UART replies between commands. Added concise `[tx]` logs and a repeated `[warn] no sensor reply yet...` diagnostic when the base-station is transmitting but receives no sensor-node response. Updated README, progress, and test-plan notes.
- 改动（中文）：将 FSR 阈值、采样间隔、中断开关和配置读回公开为 `sensor_node_driver` API；demo app 启动时调用这些 API，并在命令间继续处理 UART 回复；新增简洁的 `[tx]` 发送日志，以及 base-station 正在发送但收不到 sensor-node 回复时的 `[warn] no sensor reply yet...` 诊断；更新 README、进度和测试计划说明。
- Validation (EN): sensor-node build passed; base-station build passed and regenerated `zephyr.uf2`; protocol frame tests passed 4 tests.
- 验证（中文）：sensor-node 构建通过；base-station 构建通过并重新生成 `zephyr.uf2`；协议帧测试通过 4 个测试。
- Notes (EN): If the base-station console remains completely blank after flashing, verify that the latest base-station `zephyr.uf2` is flashed and the USB CDC console has DTR enabled.
- 备注（中文）：如果烧录后 base-station 控制台仍完全空白，请确认已刷入最新 base-station `zephyr.uf2`，并且 USB CDC 控制台启用了 DTR。

### [2026-04-27] Task: Reduce runtime logs and register Zephyr sensor-node driver
- Scope (EN): Runtime log readability and base-station driver abstraction.
- 范围（中文）：运行日志可读性与 base-station driver 抽象。
- Changes (EN): Reduced sensor-node output to concise config, sample, and event lines. Reduced base-station output to concise app, driver, config, data, force, event, and error lines. Split base-station protocol/event logic into `src/driver/sensor_node_driver.c/.h`, registered it as a Zephyr device with `DEVICE_DEFINE`, and changed `main.c` into a small demo app that uses the driver API. Updated progress and README documentation.
- 改动（中文）：将 sensor-node 输出精简为配置、采样和事件短行；将 base-station 输出精简为 app、driver、config、data、force、event 和 error 短行；把 base-station 的协议/事件逻辑拆分到 `src/driver/sensor_node_driver.c/.h`，通过 `DEVICE_DEFINE` 注册为 Zephyr 设备，并将 `main.c` 改为调用 driver API 的小型 demo app；更新进度与 README 文档。
- Validation (EN): sensor-node build passed; base-station build passed with the Zephyr driver device; protocol frame tests passed 4 tests.
- 验证（中文）：sensor-node 构建通过；base-station 携带 Zephyr driver device 构建通过；协议帧测试通过 4 个测试。
- Notes (EN): Remaining proposal work is mainly final hardware-in-loop evidence and final report/presentation preparation.
- 备注（中文）：proposal 剩余工作主要是补实板端到端证据，以及整理最终报告/展示材料。

### [2026-04-27] Task: Add framed UART protocol, clearer logs, and protocol tests
- Scope (EN): UART protocol robustness, serial log readability, and automated protocol frame tests.
- 范围（中文）：UART 协议鲁棒性、串口日志可读性，以及协议帧自动化测试。
- Changes (EN): Added framed UART support using `0xA5 0x5A CMD LEN PAYLOAD CHECKSUM`, where checksum is XOR over command, length, and payload. Base-station now sends framed commands by default and parses framed responses; sensor-node accepts framed commands while keeping newline text commands for manual debugging. Added visible separator headers for base-station transmit/receive logs, sensor-node frame/text receive logs, and sensor-node local samples. Added `tests/protocol_frame_test.py` for frame round-trip, checksum rejection, and noise resynchronization checks. Updated protocol, progress, and test-plan documentation.
- 改动（中文）：新增 framed UART 支持，格式为 `0xA5 0x5A CMD LEN PAYLOAD CHECKSUM`，其中 checksum 为命令、长度和 payload 的 XOR；base-station 默认发送 framed 命令并解析 framed 响应，sensor-node 支持 framed 命令，同时保留换行文本命令用于手动调试；为 base-station 收发日志、sensor-node frame/text 接收日志和 sensor-node 本地采样日志增加明显分隔；新增 `tests/protocol_frame_test.py`，验证帧往返、错误 checksum 丢弃和噪声后重新同步；更新协议、进度和测试计划文档。
- Validation (EN): `cmake --build build` passed for sensor-node; `env -u ZEPHYR_SDK_INSTALL_DIR CCACHE_DISABLE=1 cmake --build build` passed for base-station; `python3 tests/protocol_frame_test.py` passed 4 tests.
- 验证（中文）：sensor-node 已通过 `cmake --build build`；base-station 已通过 `env -u ZEPHYR_SDK_INSTALL_DIR CCACHE_DISABLE=1 cmake --build build`；`python3 tests/protocol_frame_test.py` 通过 4 个测试。
- Notes (EN): The framed protocol keeps ASCII payloads to preserve readable serial debugging while satisfying the proposal's header/length/checksum requirement.
- 备注（中文）：framed 协议保留 ASCII payload，既保持串口可读性，也满足 proposal 中帧头、长度和校验的要求。

### [2026-04-27] Task: Implement proposal event/config flow and update project docs
- Scope (EN): Proposal-aligned integration features for UART commands, FSR event latch, event interrupt GPIO, base-station demo flow, and documentation.
- 范围（中文）：实现与 proposal 对齐的集成功能，包括 UART 命令、FSR 事件 latch、事件中断 GPIO、base-station demo 流程和文档。
- Changes (EN): Added sensor-node runtime commands for environment, light, force, event read/clear, configuration read, sampling interval, force threshold, and interrupt enable. Added FSR threshold detection with hysteresis and a latched event output on GPIO15. Updated base-station to configure the node at startup, poll sensor values, monitor GPIO15, read latched events, and clear them. Added protocol, hardware, and progress documentation; updated endpoint READMEs and test plan with integrated verification steps.
- 改动（中文）：为 sensor-node 新增环境、光照、压力、事件读取/清除、配置读取、采样间隔、FSR 阈值和中断开关命令；新增带 hysteresis 的 FSR 阈值检测，并通过 GPIO15 输出 latch 后的事件中断；更新 base-station，使其启动时配置节点、轮询传感器数据、监听 GPIO15、读取 latch 事件并清除；新增协议、硬件和进度文档，并更新两端 README 与 test plan 的集成验证步骤。
- Validation (EN): sensor-node build succeeded with `cmake --build build`; base-station configure succeeded after clearing cached ZEPHYR_SDK_INSTALL_DIR and build succeeded with `env -u ZEPHYR_SDK_INSTALL_DIR CCACHE_DISABLE=1 cmake --build build`.
- 验证（中文）：sensor-node 已通过 `cmake --build build` 构建；base-station 在清除缓存的 ZEPHYR_SDK_INSTALL_DIR 后配置成功，并通过 `env -u ZEPHYR_SDK_INSTALL_DIR CCACHE_DISABLE=1 cmake --build build` 构建。
- Notes (EN): Current protocol remains human-readable newline UART; binary header plus CRC is documented as the remaining robustness step if required by final grading.
- 备注（中文）：当前协议仍是可读的换行 UART 文本协议；如最终评分要求必须展示二进制帧头和 CRC，已在文档中列为剩余鲁棒性步骤。

### [2026-04-21] Task: Fix UART framing loss and replace invalid BME680 read path
- Scope (EN): Communication stability and sensor-node environmental sensor path; no interrupt/event architecture changes.
- 范围（中文）：修复通信稳定性与 sensor-node 环境传感器读取链路；不改动 interrupt/event 架构。
- Changes (EN): Updated base-station and sensor-node UART receive loops to drain FIFO per cycle, ignore CR cleanly, and reduce idle delay from 10 ms to 1 ms to prevent line corruption under 115200 baud. Normalized READ_ALL payload formatting so LIGHT always represents sensor data or ERR. Replaced the previous BME280-like placeholder BME680 implementation with a BME680-compatible minimal I2C flow using correct calibration register mapping, heater setup, forced-mode trigger, new-data polling, and Bosch-style temperature/pressure/humidity compensation.
- 改动（中文）：更新 base-station 与 sensor-node 的 UART 接收循环，使其每轮读空 FIFO、正确忽略 CR，并将空闲延时从 10 ms 降到 1 ms，避免在 115200 波特率下出现整行串口内容被拼坏。统一 READ_ALL 返回格式，使 LIGHT 字段始终表示光照值或 ERR。将原先仿 BME280 的占位版 BME680 实现替换为兼容 BME680 的最小 I2C 流程，包含正确的校准寄存器映射、heater 配置、forced mode 触发、new-data 轮询，以及 Bosch 风格的温度/气压/湿度补偿。
- Validation (EN): Rebuilt sensor-node successfully after the driver replacement and rebuilt base-station successfully with CCACHE_DISABLE=1; both firmware outputs remain available as UF2/ELF artifacts.
- 验证（中文）：替换驱动后已成功重建 sensor-node，并以 CCACHE_DISABLE=1 成功重建 base-station；两端固件产物 UF2/ELF 均已生成。
- Notes (EN): Hardware-side runtime verification is still required on the actual boards to confirm BME680 address, wiring, and live measurements.
- 备注（中文）：仍需在真实板卡上进行运行验证，以确认 BME680 地址、接线和实时测量结果。

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

### [2026-05-11] Task: Add host-side real-time GUI
- Scope (EN): Host computer GUI only; firmware behavior is unchanged by this step.
- 范围（中文）：仅新增上位机 GUI；本步骤不改动固件运行逻辑。
- Changes (EN): Added `host-gui/`, a Python HTTP/SSE dashboard that reads the sensor-node USB serial console and displays current environment values, a single interrupt lamp indicator, runtime configuration controls, and raw serial logs.
- 改动（中文）：新增 `host-gui/`，用 Python HTTP/SSE 从 sensor-node USB 串口读取日志，并显示当前环境数据、一个中断状态灯、运行时配置控件和原始串口数据。
- Validation (EN): Verified `host-gui/app.py` compiles, protocol tests pass 10/10, GUI API reports live serial data on `/dev/cu.usbmodem1101`, the simplified browser page renders connected live values at `http://127.0.0.1:8765`, and the GUI can apply FSR threshold, sampling interval, and interrupt enable changes at runtime.
- 验证（中文）：已验证 `host-gui/app.py` 可编译、协议测试 10/10 通过、GUI API 能从 `/dev/cu.usbmodem1101` 读取实时数据，简洁版浏览器页面在 `http://127.0.0.1:8765` 正常渲染实时值，并可通过 GUI 运行时修改 FSR 阈值、采样周期和中断开关。
- Notes (EN): BME680 temperature, humidity, and pressure are readable; gas remains reported as `ERR` by the current firmware/sensor state and is displayed explicitly by the GUI.
- 备注（中文）：BME680 温度、湿度、气压可读；gas 仍由当前固件/传感器状态报告为 `ERR`，GUI 会如实显示。
