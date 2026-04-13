PROJECT_GUIDE.md
1. 项目定义

本项目是一个双端嵌入式系统：

Sensor Node
Base Station

目标是实现一个室内监测系统，支持：

BME680：温湿压和气体相关数据
VEML7700：环境光
FSR：压力 / 触摸检测（ADC）
UART 通信
基于 FSR 的事件检测
一条独立 interrupt GPIO，从 Sensor Node 通知 Base Station
Base Station 侧的 Zephyr driver / demo app
2. 两端职责
Sensor Node

使用 Pico SDK 开发。负责：

读取 BME680
读取 VEML7700
读取 FSR ADC
周期采样
本地事件检测
通过 UART 响应命令
通过 GPIO 输出 interrupt 信号
Base Station

使用 Zephyr 开发。负责：

通过 UART 与 Sensor Node 通信
发送命令并接收响应
检测 interrupt
获取事件和传感器数据
封装 driver
提供 demo application
3. 开发原则
3.1 这是双端项目，不是单板项目

必须始终保持：

sensor-node 和 base-station 分离
两边代码不能混写
两边分别独立构建
3.2 先让系统通，再让系统真，最后再完善

开发顺序必须是：

先打通最小链路
再接真实硬件
最后再做 driver、config、tests
3.3 不要一开始就做完整功能

先做最小可运行版本，不要提前写复杂实现。

4. 推荐开发顺序
Phase 0: 工程准备

先确认两端开发环境独立可用：

Sensor Node：Pico SDK 能编译和烧录最小程序
Base Station：Zephyr 能编译和烧录最小程序
Phase 1: 最小通信闭环

先实现最小 UART 通信：

Base Station 发送 PING
Sensor Node 返回 PONG

此阶段不要接真实传感器。

Phase 2: Mock 数据链路

加入最小 payload：

READ_FORCE 返回固定假值
READ_ALL 返回固定假值

此阶段目的是验证协议和解析，不是验证传感器。

Phase 3: 真实 FSR

先接入 FSR ADC：

读取原始 ADC
返回真实 force 数据
Phase 4: 事件逻辑

基于 FSR 实现：

threshold
hysteresis
event latch
Phase 5: Interrupt

实现：

Sensor Node 检测到事件后拉起 GPIO
Base Station 检测到 interrupt
Base Station 再通过 UART 拉取详情
Phase 6: 接入其他传感器

再接入：

VEML7700
BME680
Phase 7: 封装与收尾

最后再做：

driver
config interface
tests
demo
5. 当前主线

项目主线不是“三个传感器都读出来”。

项目主线应该始终是：

FSR 触发事件 → interrupt 通知 → Base Station 读取事件详情

因为这条链路最能体现系统价值和高分点。

BME680 和 VEML7700 是补全系统，不是最先优先的部分。

6. 文件结构原则

只保留必要层级，避免过度设计。

推荐结构：

project-root/
├── PROJECT_GUIDE.md
├── README.md
├── docs/
│   ├── protocol.md
│   ├── hardware.md
│   └── test-plan.md
├── sensor-node/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── pico_sdk_import.cmake
│   └── src/
│       ├── main.c
│       ├── board/
│       ├── comm/
│       ├── protocol/
│       ├── sensors/
│       └── event/
├── base-station/
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── prj.conf
│   └── src/
│       ├── main.c
│       ├── transport/
│       ├── protocol/
│       ├── driver/
│       ├── interrupt/
│       └── config/
└── tests/

要求：

sensor-node 只放 Pico SDK 相关内容
base-station 只放 Zephyr 相关内容
文档和测试分开
不要让一个文件负责太多事
7. 模块命名原则

命名必须清晰直接，按职责命名。

推荐：

uart_comm
frame_codec
protocol_defs
fsr_adc
event_detector
uart_transport
protocol_parser
sensor_node_driver
interrupt_handler
config_interface

不要用模糊命名，例如：

utils
common
handler
misc
data_process
8. Copilot 使用规则

Copilot 只应该一次做一个小任务。

允许它做的事：

创建目录
创建占位文件
补头文件接口
实现一个小模块
实现一个小命令
补一小段测试

不允许它一口气做的事：

完成整个项目
一次生成全部传感器驱动
一次生成完整 driver + protocol + tests
在架构未稳定前自行扩展结构

优先级永远是：

结构清晰
模块边界清晰
能编译
能调试
再考虑完整功能
9. 当前最先做什么

现在不要急着做完整功能。

当前最先做的是：

对 Sensor Node

确认 Pico SDK 工程最小可编译、可烧录、可串口打印。

对 Base Station

确认 Zephyr 工程最小可编译、可烧录、可串口打印。

然后

开始最小协议：

PING
PONG

再之后才是：

READ_FORCE mock
真实 FSR
event
interrupt
10. 最小里程碑

按这个顺序推进：

M1

两端都能独立编译和烧录最小程序。

M2

PING / PONG 通信成功。

M3

READ_FORCE mock 成功。

M4

真实 FSR ADC 读取成功。

M5

事件检测成功。

M6

interrupt 通知成功。

M7

VEML7700 和 BME680 接入成功。

M8

driver / demo / tests 完成。

11. 协作与执行补充规则

11.1 PROJECT_GUIDE 位置

PROJECT_GUIDE.md 固定保留在 docs/ 目录下，不移动到项目根目录。

11.2 WORKLOG 记录要求

项目新增 docs/WORKLOG.md 作为统一变更记录。

每次完成一次代码或结构改动后，必须更新该文件。

WORKLOG 采用中英双语记录，便于后续整理与复盘。

11.3 代码注释语言

新增代码注释必须使用英文，且保持简洁直接。

不写无信息量注释，不写冗长解释。

11.4 Copilot 执行约束

每次开始执行任务前，必须先参考 docs/PROJECT_GUIDE.md 中的最新准则。

若用户新增流程约束，应先补充到 PROJECT_GUIDE，再执行后续改动。