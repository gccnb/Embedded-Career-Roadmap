# static、extern、const、volatile

## 1. 本节要解决什么问题

`static`、`extern`、`const`、`volatile` 是嵌入式 C 面试和项目中经常出现的关键字。很多同学能背定义，但不知道它们在串口中断、驱动模块、寄存器配置和协议表里怎么用。

本节要解决的问题是：这些关键字如何服务工程代码。

## 2. 基本概念

- `static`：限制作用域或延长变量生命周期。
- `extern`：声明一个变量或函数在别的文件中定义。
- `const`：表示只读，常用于常量表、配置表、字符串和函数参数保护。
- `volatile`：告诉编译器变量可能被当前代码之外的因素修改，常见于中断、硬件寄存器、DMA 状态。

它们不是为了让代码显得高级，而是为了让模块边界、内存访问和编译器优化更符合硬件行为。

## 3. 为什么嵌入式项目需要它

在 RS485/Modbus 项目中：

- 串口接收标志位可能在中断中被修改，需要 `volatile`；
- 协议解析模块内部缓冲区不希望被其他文件直接访问，可以用 `static`；
- 多个源文件共享设备寄存器表时，需要 `extern` 做声明；
- Modbus 功能码表、CRC 初值、默认配置可以用 `const`。

这些关键字用不好，轻则代码混乱，重则出现优化导致的死循环或状态读取错误。

## 4. 工程使用场景

典型场景：

- `static` 修饰模块内部变量：防止其他文件误改。
- `static` 修饰局部变量：在函数多次调用之间保留状态。
- `extern` 放在头文件中声明共享对象。
- `const` 修饰查表数据，避免运行时误修改。
- `volatile` 修饰中断标志位、硬件状态寄存器映射变量。

如果变量来自真实硬件寄存器，地址和位定义必须以芯片数据手册、参考手册、原理图或实测结果为准。

## 5. 代码示例

下面是串口接收标志位的简化例子：

```c
static unsigned char g_rx_buffer[128];
static volatile unsigned char g_rx_done = 0;
static volatile unsigned short g_rx_length = 0;

const unsigned short MODBUS_CRC_INIT = 0xFFFF;

void uart_idle_isr(unsigned short length)
{
    g_rx_length = length;
    g_rx_done = 1;
}

int protocol_take_frame(unsigned char *out, unsigned short *length)
{
    unsigned short i;

    if (g_rx_done == 0) {
        return 0;
    }

    for (i = 0; i < g_rx_length; i++) {
        out[i] = g_rx_buffer[i];
    }

    *length = g_rx_length;
    g_rx_done = 0;
    return 1;
}
```

如果 `g_rx_done` 不写 `volatile`，编译器可能认为它在循环中不会变化，从而让主程序看不到中断修改后的值。

## 6. 常见错误

- 认为所有全局变量都应该加 `extern`，导致模块边界混乱。
- 把 `const` 当成绝对安全，忽略指针强转可能破坏只读约束。
- 忘记给中断共享变量加 `volatile`。
- 以为 `volatile` 能解决多任务互斥问题。它不能替代临界区、互斥量或关中断保护。
- 在头文件中直接定义全局变量，导致重复定义。

## 7. 调试方法

- 如果主循环一直等不到中断标志，先检查变量是否需要 `volatile`。
- 如果链接时报重复定义，检查头文件里是否写了变量定义。
- 如果某个模块变量被莫名修改，检查是否应该改成 `static` 限制作用域。
- 如果多任务同时访问共享变量，检查是否需要临界区或互斥量。

## 8. 面试常问问题

1. `static` 修饰局部变量时，变量存储在哪里？
2. `static` 修饰全局变量有什么作用？
3. `extern` 声明和定义有什么区别？
4. `const` 修饰指针时有哪些情况？
5. `volatile` 为什么常用于中断标志位？
6. `volatile` 能不能保证线程安全？

## 9. 简历表达方式

可以写：

> 在串口通信模块中使用 `static` 封装接收缓冲区，使用 `volatile` 处理中断与主循环共享状态，并通过模块化接口完成 Modbus RTU 帧接收与解析。

这说明你不仅知道关键字，还知道它们在工程中的用途。

## 10. 小练习

1. 写一个 `.c` 文件内部可见的 `static` 缓冲区。
2. 用 `extern` 在两个文件之间共享一个设备状态结构体。
3. 写一个 `const` 功能码表。
4. 写一个中断标志位，并说明为什么需要 `volatile`。
5. 思考 `volatile` 和互斥锁的区别。

## 11. 小结

这四个关键字是嵌入式 C 的工程基础。它们背后的核心不是语法，而是模块边界、内存生命周期、只读约束和硬件异步访问。

面试时不要只背定义，要结合串口中断、RS485 接收、Modbus 协议表和 FreeRTOS 任务共享变量来解释。
