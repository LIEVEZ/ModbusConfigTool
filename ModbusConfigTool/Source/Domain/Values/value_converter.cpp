#include "value_converter.h"

#include "Domain/Models/project_factory.h"

#include <QDataStream>

#include <algorithm>

namespace
{
QByteArray valueBytes(const RegisterValue &value)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    switch (value.dataType())
    {
    case DataType::Int16: stream << qint16(value.toSigned64()); break;
    case DataType::UInt16: stream << quint16(value.toUnsigned64()); break;
    case DataType::Int32: stream << qint32(value.toSigned64()); break;
    case DataType::UInt32: stream << quint32(value.toUnsigned64()); break;
    case DataType::Float32:
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream << float(value.toDouble());
        break;
    case DataType::Int64: stream << qint64(value.toSigned64()); break;
    case DataType::UInt64: stream << quint64(value.toUnsigned64()); break;
    case DataType::Float64: stream << double(value.toDouble()); break;
    default: break;
    }
    return bytes;
}

void applyEndian(QByteArray *bytes, Endian endian)
{
    if (endian == Endian::Little)
    {
        std::reverse(bytes->begin(), bytes->end());
    }
    else if (endian == Endian::LittleSwap && bytes->size() > 2)
    {
        QByteArray swapped;
        for (int index = bytes->size() - 2; index >= 0; index -= 2)
        {
            swapped.append(bytes->mid(index, 2));
        }
        *bytes = swapped;
    }
}
}

ConversionResult ValueConverter::toRegisters(const RegisterValue &value, Endian endian)
{
    ConversionResult output;
    QByteArray bytes = valueBytes(value);
    applyEndian(&bytes, endian);

    for (int index = 0; index < bytes.size(); index += 2)
    {
        const quint16 high = quint8(bytes.at(index));
        const quint16 low = quint8(bytes.at(index + 1));
        output.registers.append(quint16((high << 8U) | low));
    }
    output.result = OperationResult::ok();
    return output;
}

ValueResult ValueConverter::fromRegisters(DataType type,
                                          Endian endian,
                                          const QVector<quint16> &registers)
{
    ValueResult output;
    if (registers.size() != ProjectFactory::registerCountFor(type))
    {
        output.result = OperationResult::fail(QStringLiteral("invalid_register_count"),
                                              QStringLiteral("registerCount"),
                                              QStringLiteral("寄存器数量与数据类型不匹配"));
        return output;
    }

    QByteArray bytes;
    for (quint16 value : registers)
    {
        bytes.append(char((value >> 8U) & 0xFFU));
        bytes.append(char(value & 0xFFU));
    }
    applyEndian(&bytes, endian);

    QDataStream stream(bytes);
    stream.setByteOrder(QDataStream::BigEndian);
    switch (type)
    {
    case DataType::Int16: { qint16 v; stream >> v; output.value = RegisterValue::fromSigned64(v, type); break; }
    case DataType::UInt16: { quint16 v; stream >> v; output.value = RegisterValue::fromUnsigned64(v, type); break; }
    case DataType::Int32: { qint32 v; stream >> v; output.value = RegisterValue::fromSigned64(v, type); break; }
    case DataType::UInt32: { quint32 v; stream >> v; output.value = RegisterValue::fromUnsigned64(v, type); break; }
    case DataType::Float32:
    {
        float v;
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream >> v;
        output.value = RegisterValue::fromFloating(v, type);
        break;
    }
    case DataType::Int64: { qint64 v; stream >> v; output.value = RegisterValue::fromSigned64(v, type); break; }
    case DataType::UInt64: { quint64 v; stream >> v; output.value = RegisterValue::fromUnsigned64(v, type); break; }
    case DataType::Float64: { double v; stream >> v; output.value = RegisterValue::fromFloating(v, type); break; }
    default: break;
    }
    output.result = OperationResult::ok();
    return output;
}
