# Final Verification Evidence / 最终验证证据

Date / 日期: 2026-05-11

This file records evidence observed in this workspace and on the two connected boards.
本文件记录当前 workspace 与两块已连接开发板上实际观察到的验证证据。

## Build And Test Evidence / 构建与测试证据

- Sensor-node build / sensor-node 构建:
  - Command / 命令: `cmake --build sensor-node/build`
  - Result / 结果: passed; latest rerun reported `ninja: no work to do` and `sensor-node/build/sensor_node.uf2` remained current.
- Base-station build / base-station 构建:
  - Command / 命令: `env -u ZEPHYR_SDK_INSTALL_DIR west build -d base-station/build base-station -- -UZEPHYR_SDK_INSTALL_DIR -DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb -DGNUARMEMB_TOOLCHAIN_PATH=/opt/homebrew`
  - Result / 结果: passed; generated `base-station/build/zephyr/zephyr.uf2`, output size 40960 bytes.
- Protocol tests / 协议测试:
  - Command / 命令: `python3 tests/protocol_frame_test.py`
  - Result / 结果: passed, 10 tests.
  - Coverage / 覆盖: checksum definition, command/response frames, multiple-frame streams, checksum rejection, noise skipping, bad-frame recovery, incomplete-frame rejection, and the `BASE_BOOTSEL` maintenance frame.
- Script syntax / 脚本语法:
  - Command / 命令: `bash -n flash-base.sh flash-pico.sh`
  - Result / 结果: passed.

## Flash Evidence / 烧录证据

- Sensor-node / sensor-node:
  - Target chipid / 目标 chipid: `0x0dd161c1c0ba0642`
  - Command / 命令: `picotool load -v -x sensor-node/build/sensor_node.uf2 --ser 0DD161C1C0BA0642 -f`
  - Result / 结果: flash verify OK and rebooted into application mode.
- Base-station / base-station:
  - Target chipid / 目标 chipid: `0x3cff0b54acc69f66`
  - Command / 命令: `picotool load -v base-station/build/zephyr/zephyr.uf2 --ser 3CFF0B54ACC69F66`
  - Result / 结果: flash verify OK.
  - Reboot command / 重启命令: `picotool reboot --ser 3CFF0B54ACC69F66`

## Automatic Base Flashing / base 自动烧录

The current flashing path is `./flash-base.sh`.
当前 base 烧录路径是 `./flash-base.sh`。

Verified command / 已验证命令:

```bash
./flash-base.sh
```

Observed result / 已观察结果:

```text
[1/5] Checking picotool...
[2/5] Requesting base-station BOOTSEL...
[3/5] Waiting for base-station BOOTSEL: 3CFF0B54ACC69F66
Device Information
 chipid:                 0x3cff0b54acc69f66
 boot type:              bootsel
[4/5] Loading firmware: base-station/build/zephyr/zephyr.uf2
Verifying Flash:      100%
  OK
[5/5] Rebooting base-station...
The device was rebooted into application mode.
Done.
```

Verified flow / 已验证流程:

1. Script writes `BASE_BOOTSEL\n` to the sensor-node USB serial port.
2. Sensor-node forwards a framed UART maintenance command: `0x7E` with payload `BOOTSEL`.
3. Base-station calls `reset_usb_boot(0, 0)` and enters RP2350 BOOTSEL.
4. Script verifies the base chipid `0x3cff0b54acc69f66`, loads `base-station/build/zephyr/zephyr.uf2`, then reboots base-station.

This has been verified after installing the maintenance-capable firmware once.
该流程已在首次安装维护通道固件后完成验证。

### Post-Flash Runtime Check / 自动烧录后运行检查

After `./flash-base.sh` completed, the sensor-node console still showed normal base-station polling:
`./flash-base.sh` 完成后，sensor-node console 仍显示 base-station 正常轮询：

```text
[link-rx] type=0x10 cmd=READ_FORCE
[sample] force=26 event=0 thresh=1000 env=ERR light=379
[link-rx] type=0x11 cmd=READ_ALL
```

The final plan rerun repeated the same post-flash runtime check after automatically flashing both boards:
最终计划复跑在自动烧录两块板后再次执行了运行检查：

```text
[link-rx] type=0x10 cmd=READ_FORCE
[link-rx] type=0x11 cmd=READ_ALL
[sample] force=27 event=0 thresh=1000 env=ERR light=342
[link-rx] type=0x10 cmd=READ_FORCE
[link-rx] type=0x11 cmd=READ_ALL
[sample] force=26 event=0 thresh=1000 env=ERR light=343
```

## Hardware Runtime Evidence / 实板运行证据

Serial capture source / 串口采集来源: sensor-node USB console on `/dev/cu.usbmodem1101`.

The final base-station image has USB CDC disabled for stable macOS flashing/runtime behavior, so board-to-board behavior was verified from the sensor-node console by observing the framed commands sent by base-station over UART0.
final base-station 镜像关闭 USB CDC，以避免 macOS 运行时阻塞；因此本次通过 sensor-node USB console 观察 base-station 经 UART0 发出的 framed commands 来验证板间行为。

### Runtime Configuration / 运行时配置

Observed after rebooting the final base-station firmware:
重启 final base-station 固件后观察到：

```text
[link-rx] type=0x21 cmd=SET_FORCE_THRESHOLD=1000
[config] force_threshold=1000
[link-rx] type=0x20 cmd=SET_SAMPLING_RATE=2000
[config] sampling_ms=2000
[link-rx] type=0x22 cmd=ENABLE_INTERRUPT=1
[config] interrupt=1
[link-rx] type=0x16 cmd=READ_CONFIG
```

### Periodic Data Polling / 周期数据轮询

Observed continuous polling from base-station:
观察到 base-station 持续轮询：

```text
[link-rx] type=0x11 cmd=READ_ALL
[sample] force=24 event=0 thresh=1000 env=ERR light=703
[link-rx] type=0x10 cmd=READ_FORCE
[link-rx] type=0x11 cmd=READ_ALL
```

Notes / 备注:

- FSR ADC values and VEML7700 light values were observed on the current hardware run.
- 当前硬件运行中已观察到 FSR ADC 与 VEML7700 光照值。
- BME680 protocol and driver support is implemented, including `TEMP`, `HUM`, `PRESS`, and `GAS` fields, but the current final hardware capture returned `env=ERR`.
- 已实现 BME680 协议与驱动支持，包括 `TEMP`、`HUM`、`PRESS` 和 `GAS` 字段；但当前 final 实板采集返回 `env=ERR`。

### FSR Event Latch / FSR 事件锁存

Observed by pressing the FSR above the configured threshold:
按压 FSR 使其超过配置阈值后观察到：

```text
[event] latched force=2648 threshold=1000
[link-rx] type=0x14 cmd=READ_EVENT
[link-rx] type=0x15 cmd=CLEAR_EVENT
[event] cleared
[link-rx] type=0x11 cmd=READ_ALL
[sample] force=25 event=0 thresh=1000 env=ERR light=697
```

This verifies that the sensor-node latches the event, base-station reacts by sending `READ_EVENT`, then clears the latch with `CLEAR_EVENT`, after which sensor-node reports `event=0`.
这验证了 sensor-node 会锁存事件，base-station 会发送 `READ_EVENT` 响应该事件，再通过 `CLEAR_EVENT` 清除 latch，随后 sensor-node 回到 `event=0`。
