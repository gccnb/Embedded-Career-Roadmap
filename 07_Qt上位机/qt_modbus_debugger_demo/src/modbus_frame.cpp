#include "modbus_frame.h"

#include "modbus_crc.h"

static void appendU16(QByteArray &payload, quint16 value)
{
    payload.append(static_cast<char>((value >> 8) & 0x00FF));
    payload.append(static_cast<char>(value & 0x00FF));
}

QByteArray buildReadHoldingRegistersFrame(quint8 slaveAddress, quint16 startAddress, quint16 registerCount)
{
    QByteArray payload;

    payload.append(static_cast<char>(slaveAddress));
    payload.append(static_cast<char>(0x03));
    appendU16(payload, startAddress);
    appendU16(payload, registerCount);

    return appendModbusCrc(payload);
}

QByteArray buildWriteSingleRegisterFrame(quint8 slaveAddress, quint16 registerAddress, quint16 value)
{
    QByteArray payload;

    payload.append(static_cast<char>(slaveAddress));
    payload.append(static_cast<char>(0x06));
    appendU16(payload, registerAddress);
    appendU16(payload, value);

    return appendModbusCrc(payload);
}

QString frameToHexText(const QByteArray &frame)
{
    return QString::fromLatin1(frame.toHex(' ').toUpper());
}
