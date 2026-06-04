# 嵌入式 Linux 示例代码说明

这个目录放置 `06_嵌入式Linux` 模块配套的最小 C 示例。代码目标是教学：展示 Linux 应用层如何使用串口、TCP、UDP 接口，不绑定具体开发板、控制卡、传感器或真实项目。

涉及真实串口设备、USB-RS485 模块、网口、控制卡、传感器和端子接线时，必须以数据手册、系统设备树、原理图、端子定义和实测结果为准。这里的代码只演示应用层 API 使用方式。

## 文件说明

| 文件 | 作用 | 对应文档 |
|---|---|---|
| `linux_serial_demo.c` | 使用 `termios` 打开串口、配置 9600 8N1、发送一帧示例数据并读取响应 | `07_termios串口编程.md` |
| `tcp_client_demo.c` | 使用 TCP client 连接服务器、发送文本并接收响应 | `09_TCP_UDP_Socket.md` |
| `udp_demo.c` | 使用 UDP 发送数据并等待一次响应 | `09_TCP_UDP_Socket.md` |

## 编译

Linux、WSL 或 GitHub Actions 环境中可运行：

```bash
make
```

也可以单独编译：

```bash
gcc -Wall -Wextra -std=c99 linux_serial_demo.c -o linux_serial_demo
gcc -Wall -Wextra -std=c99 tcp_client_demo.c -o tcp_client_demo
gcc -Wall -Wextra -std=c99 udp_demo.c -o udp_demo
```

## 运行示例

串口示例：

```bash
./linux_serial_demo /dev/ttyUSB0
```

TCP client 示例：

```bash
./tcp_client_demo 127.0.0.1 9000
```

UDP 示例：

```bash
./udp_demo 127.0.0.1 9001
```

如果没有真实串口设备或服务端程序，示例可能会提示打开失败、连接失败或超时。这是正常现象。学习时先关注代码结构和错误处理，再接入实际环境验证。

## 面试表达

可以这样描述：

> 编写嵌入式 Linux 应用层通信示例，使用 `termios` 完成串口参数配置和字节收发，使用 TCP/UDP Socket 完成网络通信基础流程，并通过返回值、超时和原始数据日志定位通信问题。
