#include "comm_frame.h"

namespace
{
QString fcText(QModbusPdu::FunctionCode code)
{
    return QStringLiteral("%1").arg(quint8(code) & 0x7F, 2, 16, QLatin1Char('0')).toUpper();
}

void fillAddressQuantity(const QModbusPdu &pdu, int *address, int *quantity)
{
    *address = -1;
    *quantity = -1;
    const auto functionCode = pdu.functionCode();
    if (functionCode == QModbusPdu::ReadHoldingRegisters
        || functionCode == QModbusPdu::ReadInputRegisters
        || functionCode == QModbusPdu::WriteMultipleRegisters)
    {
        if (pdu.dataSize() >= 4)
        {
            quint16 addr = 0;
            quint16 qty = 0;
            pdu.decodeData(&addr, &qty);
            *address = int(addr);
            *quantity = int(qty);
        }
    }
    else if (functionCode == QModbusPdu::WriteSingleRegister)
    {
        if (pdu.dataSize() >= 4)
        {
            quint16 addr = 0;
            quint16 value = 0;
            pdu.decodeData(&addr, &value);
            Q_UNUSED(value);
            *address = int(addr);
            *quantity = 1;
        }
    }
}
}

QString CommFrameFactory::formatHex(const QByteArray &bytes)
{
    if (bytes.isEmpty())
    {
        return QString();
    }

    QString text;
    text.reserve(bytes.size() * 3 - 1);
    static const char kHex[] = "0123456789ABCDEF";
    for (int i = 0; i < bytes.size(); ++i)
    {
        if (i > 0)
        {
            text.append(QLatin1Char(' '));
        }
        const auto value = static_cast<unsigned char>(bytes.at(i));
        text.append(QLatin1Char(kHex[value >> 4]));
        text.append(QLatin1Char(kHex[value & 0x0F]));
    }
    return text;
}

QByteArray CommFrameFactory::pduToBytes(const QModbusPdu &pdu)
{
    QByteArray bytes;
    const quint8 functionByte = pdu.isException()
        ? quint8(quint8(pdu.functionCode()) | 0x80)
        : quint8(pdu.functionCode());
    bytes.append(char(functionByte));
    bytes.append(pdu.data());
    return bytes;
}

CommFrame CommFrameFactory::fromRequest(const QModbusPdu &request)
{
    CommFrame frame;
    frame.direction = CommDirection::Rx;
    frame.isRequest = true;
    frame.timestamp = QDateTime::currentDateTime();
    frame.functionCodeText = fcText(request.functionCode());
    fillAddressQuantity(request, &frame.address, &frame.quantity);
    frame.success = true;
    frame.exceptionCode = -1;
    frame.pduBytes = pduToBytes(request);
    return frame;
}

CommFrame CommFrameFactory::fromResponse(const QModbusPdu &response, const CommFrame &requestContext)
{
    CommFrame frame;
    frame.direction = CommDirection::Tx;
    frame.isRequest = false;
    frame.timestamp = QDateTime::currentDateTime();
    frame.functionCodeText = fcText(response.functionCode());
    frame.pduBytes = pduToBytes(response);
    frame.address = requestContext.address;
    frame.quantity = requestContext.quantity;

    if (response.isException())
    {
        frame.success = false;
        frame.exceptionCode = int(response.exceptionCode());
    }
    else
    {
        frame.success = true;
        frame.exceptionCode = -1;
        if (response.functionCode() == QModbusPdu::WriteSingleRegister
            || response.functionCode() == QModbusPdu::WriteMultipleRegisters)
        {
            fillAddressQuantity(response, &frame.address, &frame.quantity);
        }
    }
    return frame;
}
