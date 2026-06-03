/*
 * modbus_rtu_demo.c
 *
 * Teaching purpose:
 * 1. Demonstrate a minimal Modbus RTU slave parser.
 * 2. Support function code 03: read holding registers.
 * 3. Support function code 06: write single holding register.
 * 4. Use a simulated register array and no real hardware.
 *
 * Notes:
 * - This file is for learning only and does not describe a real device.
 * - Register addresses are array indexes in this demo, not real device
 *   addresses.
 * - Real register addresses, access rights, and data meanings must come from
 *   the device protocol manual.
 */

#define MODBUS_SLAVE_ADDR 0x01
#define MODBUS_REG_COUNT 16
#define MODBUS_FUNC_READ_HOLDING 0x03
#define MODBUS_FUNC_WRITE_SINGLE 0x06

#define MODBUS_OK 1
#define MODBUS_ERROR 0

#define MODBUS_EX_ILLEGAL_FUNCTION 0x01
#define MODBUS_EX_ILLEGAL_ADDRESS 0x02
#define MODBUS_EX_ILLEGAL_VALUE 0x03

static unsigned short holdingRegs[MODBUS_REG_COUNT] = {
    100, 200, 300, 400,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0
};

static unsigned short modbus_crc16(const unsigned char *data,
                                   unsigned short length)
{
    unsigned short crc = 0xFFFF;
    unsigned short i;
    unsigned char bit;

    for (i = 0; i < length; i++) {
        crc ^= data[i];

        for (bit = 0; bit < 8; bit++) {
            if ((crc & 0x0001) != 0) {
                crc = (unsigned short)((crc >> 1) ^ 0xA001);
            } else {
                crc = (unsigned short)(crc >> 1);
            }
        }
    }

    return crc;
}

static void modbus_append_crc(unsigned char *frame, unsigned short length)
{
    unsigned short crc;

    crc = modbus_crc16(frame, length);
    frame[length] = (unsigned char)(crc & 0xFF);
    frame[length + 1] = (unsigned char)((crc >> 8) & 0xFF);
}

static unsigned char modbus_check_crc(const unsigned char *frame,
                                      unsigned short length)
{
    unsigned short crc;

    if (length < 4) {
        return MODBUS_ERROR;
    }

    crc = modbus_crc16(frame, (unsigned short)(length - 2));

    if (frame[length - 2] != (unsigned char)(crc & 0xFF)) {
        return MODBUS_ERROR;
    }

    if (frame[length - 1] != (unsigned char)((crc >> 8) & 0xFF)) {
        return MODBUS_ERROR;
    }

    return MODBUS_OK;
}

static unsigned short read_u16_be(const unsigned char *data)
{
    return (unsigned short)(((unsigned short)data[0] << 8) | data[1]);
}

static void write_u16_be(unsigned char *data, unsigned short value)
{
    data[0] = (unsigned char)((value >> 8) & 0xFF);
    data[1] = (unsigned char)(value & 0xFF);
}

static unsigned short build_exception_response(unsigned char *response,
                                               unsigned char function,
                                               unsigned char exception_code)
{
    response[0] = MODBUS_SLAVE_ADDR;
    response[1] = (unsigned char)(function | 0x80);
    response[2] = exception_code;
    modbus_append_crc(response, 3);

    return 5;
}

static unsigned short handle_read_holding(const unsigned char *request,
                                          unsigned char *response)
{
    unsigned short start_addr;
    unsigned short quantity;
    unsigned short i;
    unsigned short response_len;

    start_addr = read_u16_be(&request[2]);
    quantity = read_u16_be(&request[4]);

    if (quantity == 0 || quantity > 8) {
        return build_exception_response(response,
                                        MODBUS_FUNC_READ_HOLDING,
                                        MODBUS_EX_ILLEGAL_VALUE);
    }

    if ((start_addr + quantity) > MODBUS_REG_COUNT) {
        return build_exception_response(response,
                                        MODBUS_FUNC_READ_HOLDING,
                                        MODBUS_EX_ILLEGAL_ADDRESS);
    }

    response[0] = MODBUS_SLAVE_ADDR;
    response[1] = MODBUS_FUNC_READ_HOLDING;
    response[2] = (unsigned char)(quantity * 2);

    for (i = 0; i < quantity; i++) {
        write_u16_be(&response[3 + i * 2], holdingRegs[start_addr + i]);
    }

    response_len = (unsigned short)(3 + quantity * 2);
    modbus_append_crc(response, response_len);

    return (unsigned short)(response_len + 2);
}

static unsigned short handle_write_single(const unsigned char *request,
                                          unsigned char *response)
{
    unsigned short reg_addr;
    unsigned short value;
    unsigned char i;

    reg_addr = read_u16_be(&request[2]);
    value = read_u16_be(&request[4]);

    if (reg_addr >= MODBUS_REG_COUNT) {
        return build_exception_response(response,
                                        MODBUS_FUNC_WRITE_SINGLE,
                                        MODBUS_EX_ILLEGAL_ADDRESS);
    }

    holdingRegs[reg_addr] = value;

    /*
     * Function 06 normal response usually echoes the first 6 request bytes
     * and appends a new CRC.
     */
    for (i = 0; i < 6; i++) {
        response[i] = request[i];
    }

    modbus_append_crc(response, 6);
    return 8;
}

/*
 * Modbus RTU slave entry point.
 *
 * request:  request frame with CRC.
 * response: response frame output buffer.
 * return:   response frame length; 0 means no response.
 */
unsigned short modbus_slave_process(const unsigned char *request,
                                    unsigned short request_len,
                                    unsigned char *response)
{
    unsigned char function;

    if (request_len < 8) {
        return 0;
    }

    if (request[0] != MODBUS_SLAVE_ADDR) {
        return 0;
    }

    if (modbus_check_crc(request, request_len) != MODBUS_OK) {
        return 0;
    }

    function = request[1];

    if (function == MODBUS_FUNC_READ_HOLDING) {
        return handle_read_holding(request, response);
    }

    if (function == MODBUS_FUNC_WRITE_SINGLE) {
        return handle_write_single(request, response);
    }

    return build_exception_response(response,
                                    function,
                                    MODBUS_EX_ILLEGAL_FUNCTION);
}

/*
 * Teaching demo: build a function 03 request and read holdingRegs[0..1].
 */
unsigned short demo_modbus_read03(unsigned char *response)
{
    unsigned char request[8];

    request[0] = MODBUS_SLAVE_ADDR;
    request[1] = MODBUS_FUNC_READ_HOLDING;
    request[2] = 0x00;
    request[3] = 0x00;
    request[4] = 0x00;
    request[5] = 0x02;
    modbus_append_crc(request, 6);

    return modbus_slave_process(request, 8, response);
}

/*
 * Teaching demo: build a function 06 request and write 1234 to holdingRegs[1].
 */
unsigned short demo_modbus_write06(unsigned char *response)
{
    unsigned char request[8];

    request[0] = MODBUS_SLAVE_ADDR;
    request[1] = MODBUS_FUNC_WRITE_SINGLE;
    request[2] = 0x00;
    request[3] = 0x01;
    request[4] = 0x04;
    request[5] = 0xD2;
    modbus_append_crc(request, 6);

    return modbus_slave_process(request, 8, response);
}
