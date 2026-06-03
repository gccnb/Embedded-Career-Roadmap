# termios 串口编程

## 1. 本节要解决什么问题

在单片机里，我们配置 USART 寄存器或 HAL；在嵌入式 Linux 里，串口通常表现为 `/dev/ttyS*`、`/dev/ttyUSB*`、`/dev/ttyAMA*` 这类设备文件。`termios` 就是 Linux 下配置串口参数的一套接口。

本节要解决的问题是：Linux 程序如何打开串口、设置波特率和校验位、收发字节，并把它用于 RS485/Modbus 调试。

## 2. 基本概念

Linux 里“串口也是文件”。程序通过 `open` 得到文件描述符，然后用 `read` 和 `write` 收发数据。`termios` 用来设置串口行为。

| 概念 | 作用 | 调试时看什么 |
| --- | --- | --- |
| 设备文件 | 表示串口设备 | `/dev/ttyUSB0` 是否存在 |
| 文件描述符 | 程序访问串口的句柄 | `open` 返回值 |
| 波特率 | 串口速度 | 与设备手册一致 |
| 原始模式 | 不让系统改动字节 | 二进制协议必须注意 |
| VMIN/VTIME | 控制 read 返回条件 | 影响阻塞和超时 |

Modbus RTU 是二进制协议，所以要尽量让串口处于 raw 模式，避免 Linux 终端层把换行、控制字符等内容做额外处理。

## 3. 工程用途

termios 常用于以下场景：

- Linux 网关读取 RS485 传感器；
- 工控机通过 USB-RS485 调试 Modbus 设备；
- 边缘设备采集串口数据后通过 TCP 上传；
- Qt 上位机底层间接使用串口接口。

涉及真实 USB-RS485 模块、控制卡、传感器和驱动器时，设备节点、波特率、校验位、A/B 端子、供电和接地必须以数据手册、原理图和实测结果为准，不能凭经验编造。

## 4. 简单代码或伪代码

下面示例展示最小配置流程，省略了平台差异和完整错误处理。

```c
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int open_serial(const char *dev)
{
    struct termios opt;
    int fd = open(dev, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        return -1;
    }

    tcgetattr(fd, &opt);
    cfmakeraw(&opt);
    cfsetispeed(&opt, B115200);
    cfsetospeed(&opt, B115200);
    opt.c_cflag |= CLOCAL | CREAD;
    opt.c_cflag &= ~PARENB;
    opt.c_cflag &= ~CSTOPB;
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_cc[VMIN] = 0;
    opt.c_cc[VTIME] = 10;
    tcsetattr(fd, TCSANOW, &opt);

    return fd;
}
```

发送 Modbus 帧时，用 `write(fd, frame, len)`；接收时用 `read(fd, buf, size)`，然后再做长度、地址、功能码和 CRC 检查。

## 5. 常见错误

- 没有权限访问 `/dev/ttyUSB0`。
- 串口参数没有和设备一致。
- 忘记设置 raw 模式，导致二进制帧被系统处理。
- `read` 阻塞策略没设计好，程序一直卡住。
- 以为 Linux 串口问题一定是代码问题，忽略 USB-RS485 模块和接线。
- 没有保存原始十六进制日志。

初学阶段要先分清：设备节点问题、权限问题、参数问题、接线问题、协议问题不是同一类问题。

## 6. 调试方法

建议按下面步骤排查：

1. `ls /dev/ttyUSB*` 看设备是否出现。
2. `dmesg` 查看 USB-RS485 是否被系统识别。
3. 用串口工具先验证设备能通信，再写代码。
4. 程序里打印 `open/read/write/tcsetattr` 返回值。
5. 用十六进制方式保存 TX/RX 字节。
6. 如果 Modbus CRC 错，检查是否收到了半帧、粘帧或多余字节。

Linux 下调串口也要保留硬件意识。A/B 线、终端电阻、供电、屏蔽、共地要求仍然要回到设备手册、模块手册和实测结果。

## 7. 面试常问问题

1. 问：Linux 下串口为什么是设备文件？
   答：Linux 把很多硬件抽象成文件接口，应用程序通过 `open/read/write` 访问串口，不直接操作 MCU 寄存器。

2. 问：termios 主要配置什么？
   答：配置波特率、数据位、校验位、停止位、原始模式、阻塞读取策略等串口属性。

3. 问：为什么 Modbus RTU 需要 raw 模式？
   答：因为 Modbus RTU 是二进制协议，系统不应该自动处理换行、控制字符或回显，否则原始帧可能被改变。

4. 问：`VMIN` 和 `VTIME` 有什么用？
   答：它们影响 `read` 的返回条件，可用于控制最少读取字节数和超时时间。

5. 问：Linux 串口收不到数据怎么排查？
   答：先查设备节点和权限，再查串口参数和 USB-RS485，再查接线和方向控制，最后看协议帧和 CRC。

## 8. 简历表达方式

> 在嵌入式 Linux 环境下使用 termios 完成串口参数配置和 RS485/Modbus RTU 报文收发，保存 TX/RX 十六进制日志并定位超时、半帧和 CRC 错误。

这句话适合放在 Linux 网关、工控机调试工具或边缘设备通信项目中。

## 9. 小练习

1. 写一个函数打开 `/dev/ttyUSB0` 并配置 115200、8N1。
2. 用 `read` 接收数据，并打印为十六进制。
3. 修改 `VMIN/VTIME`，观察阻塞行为变化。
4. 发送一帧 Modbus 03 请求，记录完整 TX/RX 日志。
5. 总结 Linux 串口和单片机 USART 调试的相同点与不同点。

## 10. 小结

termios 是嵌入式 Linux 串口开发的基础。它和单片机 USART 的目标一样，都是稳定收发字节，但开发方式从“配置外设寄存器”变成了“操作设备文件和系统接口”。

能把 termios、RS485、Modbus RTU、十六进制日志和错误排查连起来讲，就是 Linux 串口方向很扎实的项目表达。
