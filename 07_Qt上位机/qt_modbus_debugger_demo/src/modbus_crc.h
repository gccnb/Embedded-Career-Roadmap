#ifndef MODBUS_CRC_H
#define MODBUS_CRC_H

#include <QByteArray>
#include <QtGlobal>

quint16 modbusCrc16(const QByteArray &payload);
QByteArray appendModbusCrc(const QByteArray &payload);
bool hasValidModbusCrc(const QByteArray &frame);

#endif
