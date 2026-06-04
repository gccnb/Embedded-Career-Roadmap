# FreeRTOS RS485 通信架构示例

这个目录提供一个教学用 FreeRTOS 风格 RS485 通信架构示例。它不依赖真实 FreeRTOS 内核，也不依赖具体芯片 HAL/BSP，而是在普通桌面 C 环境下模拟任务、队列、信号量和 RS485 发送方向切换。

这样设计的目的很明确：初学者先看懂“任务如何拆、消息如何传、日志如何留”，再迁移到真实芯片和真实 FreeRTOS 工程。

## 文件说明

| 文件 | 作用 |
| --- | --- |
| `freertos_rs485_arch_demo.c` | 模拟 RS485 多任务通信结构，包含接收、解析、业务、发送、日志任务 |
| `Makefile` | 使用 gcc 做最小编译检查 |

## 示例架构

```text
UART/RS485 接收中断
    -> rxSemaphore
RxTask
    -> rxFrameQueue
ProtocolTask
    -> commandQueue
AppTask
    -> txQueue
TxTask
    -> RS485_EN = TX -> 发送 -> 等待 TC -> RS485_EN = RX
LogTask
    -> 输出调试日志和错误统计
```

示例协议只用于教学：

| 请求帧 | 含义 |
| --- | --- |
| `$READ#` | 读取示例寄存器 10 |
| `$READ:10#` | 读取示例寄存器 10 |
| `$WRITE:10=123#` | 写入示例寄存器 10，值为 123 |

这些寄存器地址只是模拟数组下标，不对应任何真实设备。真实设备的地址、功能码、接线、端子定义和通信参数，必须以设备手册、原理图、端子定义和实测结果为准。

## 编译运行

在 Linux、macOS、MSYS2 或带 gcc 的环境中运行：

```bash
make
./freertos_rs485_arch_demo
```

清理产物：

```bash
make clean
```

## 和真实 FreeRTOS 工程的对应关系

| 本示例 | 真实 FreeRTOS 工程 |
| --- | --- |
| `BinarySemaphore` | `SemaphoreHandle_t` |
| `xSemaphoreGiveFromISR_demo` | `xSemaphoreGiveFromISR` |
| `FrameQueue`、`CommandQueue`、`LogQueue` | `xQueueCreate` 创建的队列 |
| `RxTask_RunOnce` | `RxTask(void *argument)` |
| `ProtocolTask_RunOnce` | `ProtocolTask(void *argument)` |
| `TxTask_RunOnce` | 串口发送任务或通信任务中的发送分支 |

迁移到真实项目时，不要把这个文件整段搬进工程。正确做法是保留职责划分：中断只做轻量通知，复杂解析放到任务；任务之间通过队列传消息；共享串口、日志文件或寄存器表时再考虑互斥锁或单任务所有权。

## 面试表达

> 设计 FreeRTOS 风格 RS485 通信任务架构，将串口中断接收、帧缓存、协议解析、业务处理、发送响应和日志记录拆分为独立职责，使用信号量完成中断到任务同步，使用队列传递帧和命令，并记录队列溢出、格式错误和发送统计。

这句话只能在你理解并运行过示例后使用。不要把教学示例描述成真实公司项目或量产设备。
