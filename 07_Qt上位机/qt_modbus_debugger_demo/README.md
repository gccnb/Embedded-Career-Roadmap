# Qt Modbus 调试上位机最小 Demo

这个目录给出一个教学用 Qt 上位机最小工程。它的目标不是替代成熟调试工具，而是帮助初学者看清楚一个上位机项目最小应该包含哪些模块：

- 串口配置和打开关闭；
- Modbus RTU 03 读保持寄存器请求帧生成；
- Modbus RTU 06 写单个寄存器请求帧生成；
- CRC16 追加和接收帧校验；
- TX/RX 原始十六进制日志显示；
- CSV 日志导出；
- 简单的响应帧提示。

本 Demo 不绑定任何真实设备、控制卡、驱动器或传感器。涉及实际 USB-RS485 模块、端子、A/B 线、供电和接地时，必须以对应芯片手册、模块手册、原理图、端子定义和实测结果为准。

## 推荐学习目标

| 目标 | 学完能说明什么 |
| --- | --- |
| 界面和串口分离 | UI 只负责输入输出，不直接写协议细节 |
| 组帧和 CRC 分离 | Modbus 帧生成可以单独测试 |
| 原始日志保留 | 面试和调试时能用 TX/RX 证据复盘问题 |
| 只做最小闭环 | 先保证 03/06 请求和响应能看懂，再扩展图表和配置文件 |

## 目录结构

```text
qt_modbus_debugger_demo/
├── README.md
├── CMakeLists.txt
├── docs/
│   └── test_cases.md
└── src/
    ├── main.cpp
    ├── mainwindow.h
    ├── mainwindow.cpp
    ├── modbus_crc.h
    ├── modbus_crc.cpp
    ├── modbus_frame.h
    └── modbus_frame.cpp
```

## 编译方式

需要本机已安装 Qt 5 或 Qt 6，并包含 `Widgets` 和 `SerialPort` 模块。

```bash
cmake -S . -B build
cmake --build build
```

如果 CMake 找不到 Qt，需要先确认 Qt 安装路径和环境变量。不同系统、不同 Qt 安装方式的路径不一样，不建议在开源示例里写死本机路径。

## 使用方式

1. 打开程序后点击“刷新串口”。
2. 选择串口号和波特率。
3. 点击“打开串口”。
4. 选择功能码 `03 Read Holding Registers` 或 `06 Write Single Register`。
5. 设置从机地址、寄存器地址和数量/数值。
6. 点击“发送请求”，观察 TX/RX 日志。
7. 点击“导出日志”，把当前日志保存为 CSV 文件。

如果没有真实下位机，可以先阅读 `docs/test_cases.md`，用串口环回、虚拟串口或自己写的模拟从机配合测试。

导出的 CSV 字段为：

| 字段 | 说明 |
| --- | --- |
| `time` | 记录时间，精确到毫秒 |
| `direction` | `TX`、`RX`、`PARSE`、`ERROR`、`INFO` 等日志方向或类型 |
| `message` | 原始帧、解析结果或错误说明 |

## 面试表达

> 基于 Qt 编写 Modbus RTU 串口调试上位机最小 Demo，拆分串口收发、Modbus 03/06 组帧、CRC16 校验、TX/RX 日志显示和 CSV 日志导出模块，可配合下位机模拟寄存器完成通信链路验证和问题复盘。

这句话只能用于你确实阅读、运行并改造过该 Demo 的情况。不要把学习 Demo 包装成真实公司项目或量产工具。
