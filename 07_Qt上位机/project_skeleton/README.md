# Qt 上位机项目骨架

这个目录给出一个“工业串口/Modbus 调试上位机”的推荐项目骨架。它不是完整 Qt 工程源码，而是用于指导后续实现：界面怎么分区、模块怎么拆、日志怎么保存、面试时怎么讲。

建议把它和 `07_Qt上位机/09_Modbus调试上位机项目.md`、`08_项目实战/02_Modbus_RTU设备调试项目.md` 一起阅读。

## 推荐目录结构

```text
qt_modbus_debugger/
├── README.md
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── mainwindow.h
│   ├── mainwindow.cpp
│   ├── serial_port_manager.h
│   ├── serial_port_manager.cpp
│   ├── modbus_frame_builder.h
│   ├── modbus_frame_builder.cpp
│   ├── modbus_parser.h
│   ├── modbus_parser.cpp
│   ├── log_manager.h
│   └── log_manager.cpp
├── ui/
│   └── mainwindow.ui
├── docs/
│   ├── ui_layout.md
│   ├── test_cases.md
│   └── debug_log_samples.md
└── logs/
```

## 模块职责

| 模块 | 主要职责 | 不应该做什么 |
|---|---|---|
| `MainWindow` | 组织界面、按钮、表格、状态栏 | 不直接拼 Modbus 字节 |
| `SerialPortManager` | 打开串口、关闭串口、收发字节 | 不解析业务寄存器 |
| `ModbusFrameBuilder` | 生成 03/06 请求帧、追加 CRC | 不操作 UI |
| `ModbusParser` | 校验响应帧、解析功能码和数据区 | 不保存文件 |
| `LogManager` | 记录 TX/RX、错误、解析结果并导出 | 不控制串口 |

这种拆分的目的不是让文件变多，而是让面试时能讲清楚“串口、协议、界面、日志”的边界。

## 最小功能闭环

第一版建议只做 5 个功能：

1. 串口配置：端口号、波特率、数据位、停止位、校验位。
2. 打开/关闭串口。
3. 发送 03 读保持寄存器请求。
4. 发送 06 写单个寄存器请求。
5. 显示并导出 TX/RX 原始十六进制日志。

不要一开始就做复杂皮肤、图表和多页面。通信链路稳定、日志清楚、错误能复盘，比界面复杂更重要。

## 与下位机项目的关系

```text
Qt 上位机
-> 串口/USB-RS485
-> 下位机通信控制器
-> 模拟寄存器表/状态字
-> 响应帧
-> Qt 日志和表格显示
```

涉及真实 USB-RS485 模块、控制卡、驱动器、传感器或端子时，必须以数据手册、原理图、端子定义和实测结果为准。这里的骨架只说明软件结构，不提供真实接线结论。

## 简历表达

> 设计 Qt 工业串口调试上位机项目骨架，拆分串口管理、Modbus 帧生成、响应解析、日志导出和界面显示模块，可配合下位机通信控制器完成 03/06 功能码联调和 TX/RX 日志复盘。
