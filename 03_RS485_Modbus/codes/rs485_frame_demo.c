/*
 * rs485_frame_demo.c
 *
 * Teaching purpose:
 * 1. Demonstrate a simple RS485 custom text-frame receiver.
 * 2. Use '#' as the frame terminator.
 * 3. Use rxBuffer, rxIndex, and rxFlag.
 * 4. Keep the ISR short and process complete frames in the main loop.
 * 5. Reply with a simulated frame after receiving "$MODWA#".
 *
 * Notes:
 * - RS485 is a physical-layer/electrical interface, not a protocol.
 * - This file is for learning only and does not describe a real device protocol.
 * - Real wiring, RS485_EN/DE/RE polarity, and A/B terminals must be checked
 *   against the chip manual, module manual, schematic, and actual measurement.
 */

#define RX_BUFFER_SIZE 64

static char rxBuffer[RX_BUFFER_SIZE];
static volatile unsigned char rxIndex = 0;
static volatile unsigned char rxFlag = 0;

/*
 * In a real MCU project, this function would be called by the UART RX ISR.
 * Here, ch is one simulated received byte.
 */
void uart_rx_isr_simulate(char ch)
{
    if (rxFlag != 0) {
        return;
    }

    if (ch == '#') {
        rxBuffer[rxIndex] = '\0';
        rxFlag = 1;
        return;
    }

    if (rxIndex < RX_BUFFER_SIZE - 1) {
        rxBuffer[rxIndex] = ch;
        rxIndex++;
    } else {
        /* Buffer full: drop the current frame and wait for a new one. */
        rxIndex = 0;
        rxFlag = 0;
    }
}

/*
 * Simulate sending a string through RS485.
 * A real project would switch RS485_EN/DE to TX mode, send bytes, wait for
 * the UART TC flag, and switch back to RX mode.
 */
static void rs485_send_string_simulate(const char *text)
{
    (void)text;
    /*
     * No printf is used here, so the example stays close to embedded C.
     * Set a breakpoint here or replace this function with your UART sender.
     */
}

static int string_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

/*
 * Main-loop handler for a complete frame.
 * The ISR sets rxFlag after receiving '#'. The main loop parses the buffer.
 */
void main_loop_process_frame(void)
{
    if (rxFlag == 0) {
        return;
    }

    if (string_equal(rxBuffer, "$MODWA")) {
        rs485_send_string_simulate("$MODWA,OK#");
    } else {
        rs485_send_string_simulate("$ERR#");
    }

    rxIndex = 0;
    rxFlag = 0;
}

/*
 * Teaching demo: feed "$MODWA#" byte by byte.
 */
void demo_receive_modwa_frame(void)
{
    const char frame[] = "$MODWA#";
    unsigned char i;

    for (i = 0; frame[i] != '\0'; i++) {
        uart_rx_isr_simulate(frame[i]);
    }

    main_loop_process_frame();
}
