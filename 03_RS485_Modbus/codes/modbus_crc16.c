/*
 * modbus_crc16.c
 *
 * Teaching purpose:
 * 1. Implement Modbus RTU CRC16.
 * 2. Build one example request frame for function code 03.
 * 3. Provide a CRC check function for received frames.
 *
 * Notes:
 * - This file is for learning only and does not depend on any chip library.
 * - The example frame is not a real device protocol.
 * - Real register addresses and data meanings must come from the device
 *   protocol manual.
 */

#define MODBUS_CRC_OK 1
#define MODBUS_CRC_ERROR 0

unsigned short modbus_crc16(const unsigned char *data, unsigned short length)
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

/*
 * Append CRC to a Modbus RTU frame.
 * Modbus RTU stores CRC low byte first, then high byte.
 */
unsigned short modbus_append_crc(unsigned char *frame,
                                 unsigned short length_without_crc)
{
    unsigned short crc;

    crc = modbus_crc16(frame, length_without_crc);
    frame[length_without_crc] = (unsigned char)(crc & 0xFF);
    frame[length_without_crc + 1] = (unsigned char)((crc >> 8) & 0xFF);

    return (unsigned short)(length_without_crc + 2);
}

/*
 * Check CRC of a received Modbus RTU frame.
 */
unsigned char modbus_check_crc(const unsigned char *frame,
                               unsigned short length_with_crc)
{
    unsigned short crc;
    unsigned char crc_low;
    unsigned char crc_high;

    if (length_with_crc < 4) {
        return MODBUS_CRC_ERROR;
    }

    crc = modbus_crc16(frame, (unsigned short)(length_with_crc - 2));
    crc_low = (unsigned char)(crc & 0xFF);
    crc_high = (unsigned char)((crc >> 8) & 0xFF);

    if (frame[length_with_crc - 2] != crc_low) {
        return MODBUS_CRC_ERROR;
    }

    if (frame[length_with_crc - 1] != crc_high) {
        return MODBUS_CRC_ERROR;
    }

    return MODBUS_CRC_OK;
}

/*
 * Function 03 request example:
 * Slave address: 0x01
 * Function code: 0x03, read holding registers
 * Start address: 0x0000
 * Quantity: 0x000A
 *
 * Without CRC: 01 03 00 00 00 0A
 * With CRC:    01 03 00 00 00 0A C5 CD
 */
void demo_build_03_request(unsigned char *out_frame,
                           unsigned short *out_length)
{
    out_frame[0] = 0x01;
    out_frame[1] = 0x03;
    out_frame[2] = 0x00;
    out_frame[3] = 0x00;
    out_frame[4] = 0x00;
    out_frame[5] = 0x0A;

    *out_length = modbus_append_crc(out_frame, 6);
}

/*
 * Teaching check: build the demo frame and verify its CRC.
 */
unsigned char demo_check_03_request(void)
{
    unsigned char frame[8];
    unsigned short length;

    demo_build_03_request(frame, &length);
    return modbus_check_crc(frame, length);
}
