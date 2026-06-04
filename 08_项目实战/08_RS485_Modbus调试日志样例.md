# RS485/Modbus 调试日志样例

这篇文档给出一组公开、抽象、可复现的 RS485/Modbus 调试日志样例。它适合配合 `03_RS485_Modbus` 文档、`03_RS485_Modbus/codes` 示例、`07_Qt上位机/qt_modbus_debugger_demo` 和 `08_项目实战/07_项目测试报告模板.md` 使用。

注意：下面所有帧和寄存器地址都是教学样例，不对应任何真实设备。真实设备的地址、寄存器表、端子接线、A/B 线定义、终端电阻、供电和控制引脚，必须以设备手册、模块手册、原理图、端子定义和实测结果为准。

## 1. 本节要解决什么问题

调试日志要解决的问题是：当通信不正常时，你能不能根据证据判断问题在哪里。

初学者常见问题是只记一句“通信失败”。这句话没有价值。更好的日志应该包含：

| 日志字段 | 作用 |
| --- | --- |
| 时间 | 判断超时、响应间隔和任务调度延迟 |
| 方向 | 区分 TX 请求和 RX 响应 |
| 原始帧 | 保留字节级证据 |
| 解析结果 | 说明功能码、地址、数据和 CRC |
| 判断结论 | 说明下一步排查方向 |

## 2. 正常 03 功能码日志

场景：上位机读取从机地址 `01` 的 2 个保持寄存器。

```text
[10:00:01.120] [TX] 01 03 00 00 00 02 C4 0B
[10:00:01.120] [PARSE] slave=1, function=0x03, start=0x0000, quantity=2, crc=OK
[10:00:01.145] [RX] 01 03 04 00 19 00 64 2A 1F
[10:00:01.145] [PARSE] slave=1, function=0x03, byteCount=4, reg0=25, reg1=100, crc=OK
[10:00:01.145] [RESULT] 03 read holding registers success, responseTime=25ms
```

怎么看：

- 请求帧里 `01` 是从机地址，`03` 是读保持寄存器。
- `00 00` 是起始地址，`00 02` 是寄存器数量。
- 响应帧里 `04` 表示数据区 4 字节，对应 2 个寄存器。
- CRC 是否正确要由代码计算确认，不能凭眼睛猜。

## 3. 正常 06 功能码日志

场景：上位机向示例寄存器 `0x0001` 写入 `0x0005`。

```text
[10:02:10.300] [TX] 01 06 00 01 00 05 18 09
[10:02:10.300] [PARSE] slave=1, function=0x06, address=0x0001, value=0x0005, crc=OK
[10:02:10.326] [RX] 01 06 00 01 00 05 18 09
[10:02:10.326] [PARSE] slave=1, function=0x06, echo address=0x0001, value=0x0005, crc=OK
[10:02:10.326] [RESULT] 06 write single register success, responseTime=26ms
```

怎么看：

- 06 功能码正常响应通常回显写入地址和值。
- 如果返回的地址或值和请求不一致，要先确认是不是解析代码、字节序或设备协议理解错误。
- 示例地址只用于学习，不代表真实设备寄存器。

## 4. CRC 错误日志

场景：接收到完整帧，但 CRC 校验失败。

```text
[10:05:33.010] [RX] 01 03 04 00 19 00 64 00 00
[10:05:33.010] [CRC] calculated=0x3B2A, received=0x0000, result=FAIL
[10:05:33.011] [ACTION] drop frame, do not update register table
[10:05:33.011] [CHECK] verify CRC low/high byte order, frame length, lost byte, ASCII/HEX input mode
```

排查顺序：

1. 确认 CRC 是否低字节在前。
2. 确认计算 CRC 时是否漏算地址码或功能码。
3. 确认没有把 ASCII 字符串 `"01 03"` 当成真实字节 `0x01 0x03`。
4. 确认串口工具发送的是十六进制模式还是文本模式。

面试回答可以这样讲：CRC 错误时我不会更新业务数据，而是先保留原始帧和计算结果，再检查字节顺序、帧长度和输入模式。

## 5. 超时无响应日志

场景：上位机发送请求后没有收到从机响应。

```text
[10:08:00.500] [TX] 01 03 00 00 00 02 C4 0B
[10:08:00.700] [TIMEOUT] no response in 200ms
[10:08:00.701] [CHECK] serial config: baud=9600, data=8, parity=None, stop=1
[10:08:00.701] [CHECK] slave address=1, function=0x03
[10:08:00.702] [CHECK] RS485 direction: TX before send, wait TC, RX after send
[10:08:00.702] [CHECK] wiring must follow module manual, schematic, terminal definition, and measurement
```

排查顺序：

| 排查项 | 说明 |
| --- | --- |
| 串口参数 | 波特率、校验位、停止位不一致会直接无响应 |
| 从机地址 | 地址不匹配时从机通常不会回应 |
| CRC | CRC 错误时从机可能丢弃请求 |
| RS485 方向 | 发送后未切回接收会收不到响应 |
| 接线依据 | A/B、GND、终端电阻和供电必须查资料和实测 |

不要在日志里写“怀疑设备坏了”作为第一结论。先把可验证项查完。

## 6. RS485 方向切换过早

场景：发送任务刚写入最后一个字节就立刻切回接收，没有等待 TC 标志位。

```text
[10:12:21.000] [TX] 01 03 00 00 00 02 C4 0B
[10:12:21.001] [WARN] RS485_EN switched to RX immediately after write buffer empty
[10:12:21.030] [RX] no response
[10:12:21.031] [ANALYSIS] last byte may not be fully shifted out on bus
[10:12:21.031] [FIX] wait UART TC before RS485_EN=RX
```

判断逻辑：

- TXE 只能说明发送数据寄存器空，不代表最后一个停止位已经发完。
- TC 才更接近“整帧发送完成”。
- 切换过早可能导致最后一个字节或停止位不完整，从机校验失败后不响应。

## 7. RS485 方向切换过晚

场景：发送完成后长时间保持发送模式，没有及时切回接收。

```text
[10:14:45.100] [TX] 01 06 00 01 00 05 18 09
[10:14:45.126] [INFO] request sent, TC=1
[10:14:45.220] [TIMEOUT] no response
[10:14:45.221] [ANALYSIS] RS485 stayed in TX mode too long, slave response may be missed
[10:14:45.221] [FIX] switch to RX immediately after TC and required guard time
```

判断逻辑：

- 切换太早会破坏自己的发送帧。
- 切换太晚会错过从机响应。
- 实际保护时间要结合芯片、收发器、波特率和实测结果，不能凭经验写死。

## 8. FreeRTOS 队列异常日志

场景：通信接收正常，但协议任务处理不及时，接收队列溢出。

```text
[10:20:10.000] [ISR] frame received, give rxSemaphore
[10:20:10.001] [RxTask] push frame to rxFrameQueue, used=4/4
[10:20:10.003] [RxTask] queue full, drop frame, queueDrops=1
[10:20:10.004] [CHECK] ProtocolTask priority, parse time, queue length, log blocking
[10:20:10.005] [ACTION] reduce ISR work, move parsing to task, check LogTask output cost
```

排查重点：

- 通信任务优先级是否太低。
- 协议解析是否耗时过长。
- 日志打印是否阻塞。
- 队列长度是否合理。
- 是否有任务长期占用 CPU。

这类日志可以和 `04_FreeRTOS/codes/freertos_rs485_arch_demo.c` 对照理解。

## 9. Qt 上位机日志导出样例

CSV 可以这样设计：

```csv
time,direction,raw_hex,parse_result,status,note
10:00:01.120,TX,"01 03 00 00 00 02 C4 0B","read holding registers","OK","quantity=2"
10:00:01.145,RX,"01 03 04 00 19 00 64 2A 1F","register values","OK","reg0=25;reg1=100"
10:05:33.010,RX,"01 03 04 00 19 00 64 00 00","crc mismatch","FAIL","drop frame"
```

字段不要太花哨。能说明时间、方向、原始帧、解析结果、状态和备注就够了。

## 10. 面试常问问题

1. 问：日志里最重要的字段是什么？
   答：时间、方向、原始帧、解析结果和错误结论。原始帧尤其重要，因为它能定位 CRC、长度和字节序问题。

2. 问：RS485 无响应你怎么查？
   答：先查串口参数、从机地址、CRC，再查方向控制和接线依据。接线必须依据手册、原理图和实测结果。

3. 问：方向切换过早和过晚有什么区别？
   答：过早可能导致最后字节没发完整，从机丢帧；过晚可能错过从机响应。

4. 问：为什么异常帧不能更新业务状态？
   答：CRC 错、长度错或功能码异常时，数据可信度不足，更新业务状态会把通信错误变成控制错误。

5. 问：FreeRTOS 下通信偶发丢帧怎么查？
   答：看任务优先级、队列长度、队列溢出计数、协议解析耗时、日志阻塞和任务栈剩余。

## 11. 简历表达方式

> 在个人 RS485/Modbus RTU 学习项目中，设计 TX/RX 调试日志格式，记录正常 03/06 功能码通信、CRC 错误、超时、RS485 方向切换异常和 FreeRTOS 队列溢出等场景，能够基于原始帧和错误统计复盘通信问题。

这条表达的前提是你确实整理过类似日志，并能解释每一条日志的含义。

## 12. 小练习

1. 手写一条 03 功能码请求日志，并标出地址、功能码、起始地址、数量和 CRC。
2. 把一条 CRC 错误日志改写成面试回答。
3. 设计一个方向切换过晚的复现实验步骤。
4. 给 Qt 上位机日志增加一个 `response_time_ms` 字段，并说明用途。

## 13. 小结

调试日志不是流水账，而是证据链。对嵌入式求职来说，能拿出正常帧、异常帧、超时记录和排查结论，比只说“我调过串口”更有说服力。
