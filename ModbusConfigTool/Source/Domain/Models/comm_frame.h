#ifndef COMM_FRAME_H
#define COMM_FRAME_H

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QModbusPdu>
#include <QString>

enum class CommDirection
{
    Rx,
    Tx
};

struct CommFrame
{
    CommDirection direction = CommDirection::Rx;
    QString functionCodeText;
    int address = -1;
    int quantity = -1;
    bool success = true;
    int exceptionCode = -1;
    bool isRequest = true;
    QByteArray pduBytes;
    QDateTime timestamp = QDateTime::currentDateTime();
};

namespace CommFrameFactory
{
QString formatHex(const QByteArray &bytes);
QByteArray pduToBytes(const QModbusPdu &pdu);
CommFrame fromRequest(const QModbusPdu &request);
CommFrame fromResponse(const QModbusPdu &response, const CommFrame &requestContext);
}

Q_DECLARE_METATYPE(CommFrame)

#endif
