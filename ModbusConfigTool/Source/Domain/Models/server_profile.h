#ifndef SERVER_PROFILE_H
#define SERVER_PROFILE_H

#include "Domain/Models/domain_enums.h"

#include <QString>

struct ServerProfile
{
    ConnectionType connectionType = ConnectionType::Tcp;
    QString tcpHost = QStringLiteral("127.0.0.1");
    quint16 tcpPort = 5020;
    QString serialPort;
    int baudRate = 9600;
    QChar parity = QLatin1Char('N');
    int dataBits = 8;
    int stopBits = 1;
    int pollIntervalMs = 1000;
    quint8 slaveAddress = 1;
};

#endif
