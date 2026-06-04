#ifndef MODBUS_FRAME_H
#define MODBUS_FRAME_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>

QByteArray buildReadHoldingRegistersFrame(quint8 slaveAddress, quint16 startAddress, quint16 registerCount);
QByteArray buildWriteSingleRegisterFrame(quint8 slaveAddress, quint16 registerAddress, quint16 value);
QString frameToHexText(const QByteArray &frame);

#endif
