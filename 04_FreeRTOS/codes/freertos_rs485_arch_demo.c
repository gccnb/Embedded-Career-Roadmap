/*
 * Teaching purpose:
 * This file demonstrates a FreeRTOS-style RS485 communication architecture.
 * It does not depend on a real MCU HAL, a real FreeRTOS kernel, or any real
 * device protocol. It runs as a desktop C simulation so beginners can inspect
 * task boundaries, queues, semaphore handoff, RS485 direction control, and logs.
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY 0xFFFFFFFFu
#define pdMS_TO_TICKS(ms) (ms)

#define MAX_FRAME_SIZE 64
#define FRAME_QUEUE_LENGTH 4
#define COMMAND_QUEUE_LENGTH 4
#define TX_QUEUE_LENGTH 4
#define LOG_QUEUE_LENGTH 12
#define LOG_TEXT_SIZE 128
#define REGISTER_COUNT 32

typedef int BaseType_t;
typedef uint32_t TickType_t;

typedef struct {
    int available;
} BinarySemaphore;

typedef struct {
    uint8_t data[MAX_FRAME_SIZE];
    size_t length;
} FrameMessage;

typedef enum {
    COMMAND_READ_REGISTER = 1,
    COMMAND_WRITE_REGISTER = 2
} CommandType;

typedef struct {
    CommandType type;
    uint16_t address;
    uint16_t value;
} CommandMessage;

typedef struct {
    FrameMessage items[FRAME_QUEUE_LENGTH];
    unsigned head;
    unsigned tail;
    unsigned count;
} FrameQueue;

typedef struct {
    CommandMessage items[COMMAND_QUEUE_LENGTH];
    unsigned head;
    unsigned tail;
    unsigned count;
} CommandQueue;

typedef struct {
    FrameMessage items[TX_QUEUE_LENGTH];
    unsigned head;
    unsigned tail;
    unsigned count;
} TxQueue;

typedef struct {
    char text[LOG_TEXT_SIZE];
} LogMessage;

typedef struct {
    LogMessage items[LOG_QUEUE_LENGTH];
    unsigned head;
    unsigned tail;
    unsigned count;
} LogQueue;

typedef struct {
    unsigned rxFrames;
    unsigned txFrames;
    unsigned formatErrors;
    unsigned queueDrops;
    unsigned rxOverflows;
} Diagnostics;

static BinarySemaphore rxSemaphore;
static FrameQueue rxFrameQueue;
static CommandQueue commandQueue;
static TxQueue txQueue;
static LogQueue logQueue;
static Diagnostics diagnostics;

static uint16_t holdingRegisters[REGISTER_COUNT];
static uint8_t isrRxBuffer[MAX_FRAME_SIZE];
static size_t isrRxIndex;
static size_t isrReadyLength;
static int isrFrameReady;

static BaseType_t xSemaphoreGiveFromISR_demo(BinarySemaphore *sem)
{
    sem->available = 1;
    return pdTRUE;
}

static BaseType_t xSemaphoreTake_demo(BinarySemaphore *sem, TickType_t ticks)
{
    (void)ticks;

    if (sem->available == 0) {
        return pdFALSE;
    }

    sem->available = 0;
    return pdTRUE;
}

static void vTaskDelay_demo(TickType_t ticks)
{
    (void)ticks;
}

static BaseType_t frameQueueSend(FrameQueue *queue, const FrameMessage *item)
{
    if (queue->count >= FRAME_QUEUE_LENGTH) {
        diagnostics.queueDrops++;
        return pdFALSE;
    }

    queue->items[queue->tail] = *item;
    queue->tail = (queue->tail + 1u) % FRAME_QUEUE_LENGTH;
    queue->count++;
    return pdTRUE;
}

static BaseType_t frameQueueReceive(FrameQueue *queue, FrameMessage *item)
{
    if (queue->count == 0u) {
        return pdFALSE;
    }

    *item = queue->items[queue->head];
    queue->head = (queue->head + 1u) % FRAME_QUEUE_LENGTH;
    queue->count--;
    return pdTRUE;
}

static BaseType_t commandQueueSend(CommandQueue *queue, const CommandMessage *item)
{
    if (queue->count >= COMMAND_QUEUE_LENGTH) {
        diagnostics.queueDrops++;
        return pdFALSE;
    }

    queue->items[queue->tail] = *item;
    queue->tail = (queue->tail + 1u) % COMMAND_QUEUE_LENGTH;
    queue->count++;
    return pdTRUE;
}

static BaseType_t commandQueueReceive(CommandQueue *queue, CommandMessage *item)
{
    if (queue->count == 0u) {
        return pdFALSE;
    }

    *item = queue->items[queue->head];
    queue->head = (queue->head + 1u) % COMMAND_QUEUE_LENGTH;
    queue->count--;
    return pdTRUE;
}

static BaseType_t txQueueSend(TxQueue *queue, const FrameMessage *item)
{
    if (queue->count >= TX_QUEUE_LENGTH) {
        diagnostics.queueDrops++;
        return pdFALSE;
    }

    queue->items[queue->tail] = *item;
    queue->tail = (queue->tail + 1u) % TX_QUEUE_LENGTH;
    queue->count++;
    return pdTRUE;
}

static BaseType_t txQueueReceive(TxQueue *queue, FrameMessage *item)
{
    if (queue->count == 0u) {
        return pdFALSE;
    }

    *item = queue->items[queue->head];
    queue->head = (queue->head + 1u) % TX_QUEUE_LENGTH;
    queue->count--;
    return pdTRUE;
}

static void enqueueLog(const char *format, ...)
{
    va_list args;
    LogMessage *message;

    if (logQueue.count >= LOG_QUEUE_LENGTH) {
        diagnostics.queueDrops++;
        return;
    }

    message = &logQueue.items[logQueue.tail];
    va_start(args, format);
    vsnprintf(message->text, sizeof(message->text), format, args);
    va_end(args);

    logQueue.tail = (logQueue.tail + 1u) % LOG_QUEUE_LENGTH;
    logQueue.count++;
}

static BaseType_t logQueueReceive(LogQueue *queue, LogMessage *item)
{
    if (queue->count == 0u) {
        return pdFALSE;
    }

    *item = queue->items[queue->head];
    queue->head = (queue->head + 1u) % LOG_QUEUE_LENGTH;
    queue->count--;
    return pdTRUE;
}

static void frameToText(const FrameMessage *frame, char *text, size_t textSize)
{
    size_t copyLength = frame->length;

    if (copyLength >= textSize) {
        copyLength = textSize - 1u;
    }

    memcpy(text, frame->data, copyLength);
    text[copyLength] = '\0';
}

static void makeTextFrame(FrameMessage *frame, const char *text)
{
    size_t length = strlen(text);

    if (length > MAX_FRAME_SIZE) {
        length = MAX_FRAME_SIZE;
    }

    memcpy(frame->data, text, length);
    frame->length = length;
}

static void rs485SetTransmitMode(void)
{
    printf("[RS485] RS485_EN=TX, DE=1, RE=1\n");
}

static void rs485SetReceiveMode(void)
{
    printf("[RS485] wait UART TC, then RS485_EN=RX, DE=0, RE=0\n");
}

static void uartSendFrame(const FrameMessage *frame)
{
    char text[MAX_FRAME_SIZE + 1u];

    frameToText(frame, text, sizeof(text));
    rs485SetTransmitMode();
    printf("[UART TX] %s\n", text);
    rs485SetReceiveMode();
}

static void UartRxIsr(uint8_t byte)
{
    if (isrFrameReady != 0) {
        diagnostics.queueDrops++;
        return;
    }

    if (isrRxIndex >= MAX_FRAME_SIZE) {
        diagnostics.rxOverflows++;
        isrRxIndex = 0u;
        return;
    }

    isrRxBuffer[isrRxIndex++] = byte;

    if (byte == '#') {
        isrReadyLength = isrRxIndex;
        isrFrameReady = 1;
        (void)xSemaphoreGiveFromISR_demo(&rxSemaphore);
    }
}

static void RxTask_RunOnce(void)
{
    FrameMessage frame;

    if (xSemaphoreTake_demo(&rxSemaphore, portMAX_DELAY) != pdTRUE) {
        return;
    }

    if (isrFrameReady == 0 || isrReadyLength == 0u) {
        return;
    }

    memcpy(frame.data, isrRxBuffer, isrReadyLength);
    frame.length = isrReadyLength;

    isrRxIndex = 0u;
    isrReadyLength = 0u;
    isrFrameReady = 0;

    if (frameQueueSend(&rxFrameQueue, &frame) == pdTRUE) {
        diagnostics.rxFrames++;
        enqueueLog("[RxTask] moved one frame to rxFrameQueue, length=%u", (unsigned)frame.length);
    }
}

static BaseType_t parseFrameToCommand(const FrameMessage *frame, CommandMessage *command)
{
    char text[MAX_FRAME_SIZE + 1u];
    unsigned address = 0u;
    unsigned value = 0u;

    frameToText(frame, text, sizeof(text));

    if (strcmp(text, "$READ#") == 0) {
        command->type = COMMAND_READ_REGISTER;
        command->address = 10u;
        command->value = 0u;
        return pdTRUE;
    }

    if (sscanf(text, "$READ:%u#", &address) == 1) {
        command->type = COMMAND_READ_REGISTER;
        command->address = (uint16_t)address;
        command->value = 0u;
        return pdTRUE;
    }

    if (sscanf(text, "$WRITE:%u=%u#", &address, &value) == 2) {
        command->type = COMMAND_WRITE_REGISTER;
        command->address = (uint16_t)address;
        command->value = (uint16_t)value;
        return pdTRUE;
    }

    return pdFALSE;
}

static void ProtocolTask_RunOnce(void)
{
    FrameMessage frame;
    CommandMessage command;
    char text[MAX_FRAME_SIZE + 1u];

    if (frameQueueReceive(&rxFrameQueue, &frame) != pdTRUE) {
        return;
    }

    frameToText(&frame, text, sizeof(text));
    enqueueLog("[ProtocolTask] parse frame: %s", text);

    if (parseFrameToCommand(&frame, &command) != pdTRUE) {
        diagnostics.formatErrors++;
        enqueueLog("[ProtocolTask] format error, drop frame");
        return;
    }

    if (command.address >= REGISTER_COUNT) {
        diagnostics.formatErrors++;
        enqueueLog("[ProtocolTask] address out of range: %u", (unsigned)command.address);
        return;
    }

    if (commandQueueSend(&commandQueue, &command) != pdTRUE) {
        enqueueLog("[ProtocolTask] commandQueue full");
    }
}

static void AppTask_RunOnce(void)
{
    CommandMessage command;
    FrameMessage response;
    char text[MAX_FRAME_SIZE];

    if (commandQueueReceive(&commandQueue, &command) != pdTRUE) {
        return;
    }

    if (command.type == COMMAND_READ_REGISTER) {
        snprintf(text, sizeof(text), "$REG:%u=%u#", (unsigned)command.address, (unsigned)holdingRegisters[command.address]);
        makeTextFrame(&response, text);
        (void)txQueueSend(&txQueue, &response);
        enqueueLog("[AppTask] read register %u", (unsigned)command.address);
        return;
    }

    if (command.type == COMMAND_WRITE_REGISTER) {
        holdingRegisters[command.address] = command.value;
        snprintf(text, sizeof(text), "$OK:%u=%u#", (unsigned)command.address, (unsigned)command.value);
        makeTextFrame(&response, text);
        (void)txQueueSend(&txQueue, &response);
        enqueueLog("[AppTask] write register %u=%u", (unsigned)command.address, (unsigned)command.value);
    }
}

static void TxTask_RunOnce(void)
{
    FrameMessage frame;

    if (txQueueReceive(&txQueue, &frame) != pdTRUE) {
        return;
    }

    uartSendFrame(&frame);
    diagnostics.txFrames++;
    enqueueLog("[TxTask] response sent");
}

static void LogTask_RunOnce(void)
{
    LogMessage message;

    while (logQueueReceive(&logQueue, &message) == pdTRUE) {
        printf("[LOG] %s\n", message.text);
    }
}

static void runSchedulerTicks(unsigned ticks)
{
    unsigned i;

    for (i = 0u; i < ticks; ++i) {
        RxTask_RunOnce();
        ProtocolTask_RunOnce();
        AppTask_RunOnce();
        TxTask_RunOnce();
        LogTask_RunOnce();
        vTaskDelay_demo(pdMS_TO_TICKS(1u));
    }
}

static void simulateUartReceiveText(const char *text)
{
    size_t i;

    printf("\n[SIM RX] %s\n", text);
    for (i = 0u; text[i] != '\0'; ++i) {
        UartRxIsr((uint8_t)text[i]);
    }
}

static void printDiagnostics(void)
{
    printf("\n[DIAG] rxFrames=%u txFrames=%u formatErrors=%u queueDrops=%u rxOverflows=%u\n",
           diagnostics.rxFrames,
           diagnostics.txFrames,
           diagnostics.formatErrors,
           diagnostics.queueDrops,
           diagnostics.rxOverflows);
}

int main(void)
{
    holdingRegisters[10] = 25u;

    simulateUartReceiveText("$READ#");
    runSchedulerTicks(3u);

    simulateUartReceiveText("$WRITE:10=123#");
    runSchedulerTicks(3u);

    simulateUartReceiveText("$READ:10#");
    runSchedulerTicks(3u);

    simulateUartReceiveText("$BAD#");
    runSchedulerTicks(3u);

    printDiagnostics();
    return 0;
}
