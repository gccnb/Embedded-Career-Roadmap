#include "modbus_crc.h"

quint16 modbusCrc16(const QByteArray &payload)
{
    quint16 crc = 0xFFFF;

    for (char rawByte : payload) {
        crc ^= static_cast<quint8>(rawByte);

        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x0001) != 0) {
                crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
            } else {
                crc = static_cast<quint16>(crc >> 1);
            }
        }
    }

    return crc;
}

QByteArray appendModbusCrc(const QByteArray &payload)
{
    QByteArray frame = payload;
    quint16 crc = modbusCrc16(payload);

    frame.append(static_cast<char>(crc & 0x00FF));
    frame.append(static_cast<char>((crc >> 8) & 0x00FF));

    return frame;
}

bool hasValidModbusCrc(const QByteArray &frame)
{
    if (frame.size() < 4) {
        return false;
    }

    QByteArray payload = frame.left(frame.size() - 2);
    quint16 calculated = modbusCrc16(payload);
    quint16 received = static_cast<quint8>(frame.at(frame.size() - 2))
        | static_cast<quint16>(static_cast<quint8>(frame.at(frame.size() - 1)) << 8);

    return calculated == received;
}
