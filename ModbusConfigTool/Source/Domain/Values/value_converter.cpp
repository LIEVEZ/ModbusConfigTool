#include "value_converter.h"

#include "Domain/Models/project_factory.h"

#include <QDataStream>

#include <algorithm>
#include <limits>

namespace
{
bool isBcdEndian(Endian endian)
{
    return endian == Endian::BigBcd || endian == Endian::LittleBcd;
}

bool isFloatingType(DataType type)
{
    return type == DataType::Float32 || type == DataType::Float64;
}

bool isSignedType(DataType type)
{
    return type == DataType::Int16
        || type == DataType::Int32
        || type == DataType::Int64;
}

int byteCountFor(DataType type)
{
    return int(ProjectFactory::registerCountFor(type)) * 2;
}

quint64 maxBcdValue(int byteCount)
{
    quint64 maxValue = 0;
    for (int i = 0; i < byteCount * 2; ++i)
    {
        maxValue = maxValue * 10ULL + 9ULL;
    }
    return maxValue;
}

QByteArray encodeBcd(quint64 value, int byteCount, OperationResult *error)
{
    if (value > maxBcdValue(byteCount))
    {
        if (error)
        {
            *error = OperationResult::fail(
                QStringLiteral("bcd_overflow"),
                QStringLiteral("value"),
                QStringLiteral("BCD 数值超出可表示位数"));
        }
        return {};
    }

    QByteArray bytes(byteCount, char(0));
    for (int index = byteCount - 1; index >= 0; --index)
    {
        const int lowDigit = int(value % 10ULL);
        value /= 10ULL;
        const int highDigit = int(value % 10ULL);
        value /= 10ULL;
        bytes[index] = char((highDigit << 4) | lowDigit);
    }
    return bytes;
}

bool decodeBcd(const QByteArray &bytes, quint64 *outValue, OperationResult *error)
{
    if (!outValue)
    {
        return false;
    }

    quint64 value = 0;
    for (char raw : bytes)
    {
        const auto byte = quint8(raw);
        const int highDigit = (byte >> 4) & 0x0F;
        const int lowDigit = byte & 0x0F;
        if (highDigit > 9 || lowDigit > 9)
        {
            if (error)
            {
                *error = OperationResult::fail(
                    QStringLiteral("invalid_bcd"),
                    QStringLiteral("value"),
                    QStringLiteral("BCD 编码包含非法半字节"));
            }
            return false;
        }
        value = value * 100ULL + quint64(highDigit * 10 + lowDigit);
    }
    *outValue = value;
    return true;
}

QByteArray valueBytesBinary(const RegisterValue &value)
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

bool valueBytesBcd(const RegisterValue &value, QByteArray *bytes, OperationResult *error)
{
    if (!bytes)
    {
        return false;
    }

    if (isFloatingType(value.dataType()))
    {
        if (error)
        {
            *error = OperationResult::fail(
                QStringLiteral("bcd_float_unsupported"),
                QStringLiteral("dataType"),
                QStringLiteral("浮点类型不支持 BCD 编码"));
        }
        return false;
    }

    if (isSignedType(value.dataType()) && value.toSigned64() < 0)
    {
        if (error)
        {
            *error = OperationResult::fail(
                QStringLiteral("bcd_negative_unsupported"),
                QStringLiteral("value"),
                QStringLiteral("BCD 不支持负数"));
        }
        return false;
    }

    const quint64 numeric = isSignedType(value.dataType())
        ? quint64(value.toSigned64())
        : value.toUnsigned64();
    *bytes = encodeBcd(numeric, byteCountFor(value.dataType()), error);
    return !bytes->isEmpty();
}

void applyEndian(QByteArray *bytes, Endian endian)
{
    if (!bytes || bytes->isEmpty())
    {
        return;
    }

    switch (endian)
    {
    case Endian::Big:
    case Endian::BigBcd:
        break;
    case Endian::Little:
    case Endian::LittleBcd:
        std::reverse(bytes->begin(), bytes->end());
        break;
    case Endian::LittleSwap:
        if (bytes->size() > 2)
        {
            QByteArray swapped;
            swapped.reserve(bytes->size());
            for (int index = bytes->size() - 2; index >= 0; index -= 2)
            {
                swapped.append(bytes->mid(index, 2));
            }
            *bytes = swapped;
        }
        break;
    case Endian::BigSwap:
        // ABCD -> BADC：每个寄存器内部字节对调
        for (int index = 0; index + 1 < bytes->size(); index += 2)
        {
            const char left = bytes->at(index);
            const char right = bytes->at(index + 1);
            (*bytes)[index] = right;
            (*bytes)[index + 1] = left;
        }
        break;
    default:
        break;
    }
}

QVector<quint16> bytesToRegisters(const QByteArray &bytes)
{
    QVector<quint16> registers;
    registers.reserve((bytes.size() + 1) / 2);
    for (int index = 0; index + 1 < bytes.size(); index += 2)
    {
        const quint16 high = quint8(bytes.at(index));
        const quint16 low = quint8(bytes.at(index + 1));
        registers.append(quint16((high << 8U) | low));
    }
    return registers;
}

QByteArray registersToBytes(const QVector<quint16> &registers)
{
    QByteArray bytes;
    bytes.reserve(registers.size() * 2);
    for (quint16 value : registers)
    {
        bytes.append(char((value >> 8U) & 0xFFU));
        bytes.append(char(value & 0xFFU));
    }
    return bytes;
}

ValueResult valueFromBinaryBytes(DataType type, const QByteArray &bytes)
{
    ValueResult output;
    QDataStream stream(bytes);
    stream.setByteOrder(QDataStream::BigEndian);
    switch (type)
    {
    case DataType::Int16:
    {
        qint16 v = 0;
        stream >> v;
        output.value = RegisterValue::fromSigned64(v, type);
        break;
    }
    case DataType::UInt16:
    {
        quint16 v = 0;
        stream >> v;
        output.value = RegisterValue::fromUnsigned64(v, type);
        break;
    }
    case DataType::Int32:
    {
        qint32 v = 0;
        stream >> v;
        output.value = RegisterValue::fromSigned64(v, type);
        break;
    }
    case DataType::UInt32:
    {
        quint32 v = 0;
        stream >> v;
        output.value = RegisterValue::fromUnsigned64(v, type);
        break;
    }
    case DataType::Float32:
    {
        float v = 0.0f;
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream >> v;
        output.value = RegisterValue::fromFloating(v, type);
        break;
    }
    case DataType::Int64:
    {
        qint64 v = 0;
        stream >> v;
        output.value = RegisterValue::fromSigned64(v, type);
        break;
    }
    case DataType::UInt64:
    {
        quint64 v = 0;
        stream >> v;
        output.value = RegisterValue::fromUnsigned64(v, type);
        break;
    }
    case DataType::Float64:
    {
        double v = 0.0;
        stream >> v;
        output.value = RegisterValue::fromFloating(v, type);
        break;
    }
    default:
        break;
    }
    output.result = OperationResult::ok();
    return output;
}

ValueResult valueFromBcdBytes(DataType type, const QByteArray &bytes)
{
    ValueResult output;
    if (isFloatingType(type))
    {
        output.result = OperationResult::fail(
            QStringLiteral("bcd_float_unsupported"),
            QStringLiteral("dataType"),
            QStringLiteral("浮点类型不支持 BCD 解码"));
        return output;
    }

    quint64 decoded = 0;
    if (!decodeBcd(bytes, &decoded, &output.result))
    {
        return output;
    }

    switch (type)
    {
    case DataType::Int16:
        if (decoded > quint64(std::numeric_limits<qint16>::max()))
        {
            output.result = OperationResult::fail(
                QStringLiteral("bcd_range"),
                QStringLiteral("value"),
                QStringLiteral("BCD 解码结果超出 INT16 范围"));
            return output;
        }
        output.value = RegisterValue::fromSigned64(qint64(decoded), type);
        break;
    case DataType::Int32:
        if (decoded > quint64(std::numeric_limits<qint32>::max()))
        {
            output.result = OperationResult::fail(
                QStringLiteral("bcd_range"),
                QStringLiteral("value"),
                QStringLiteral("BCD 解码结果超出 INT32 范围"));
            return output;
        }
        output.value = RegisterValue::fromSigned64(qint64(decoded), type);
        break;
    case DataType::Int64:
        if (decoded > quint64(std::numeric_limits<qint64>::max()))
        {
            output.result = OperationResult::fail(
                QStringLiteral("bcd_range"),
                QStringLiteral("value"),
                QStringLiteral("BCD 解码结果超出 INT64 范围"));
            return output;
        }
        output.value = RegisterValue::fromSigned64(qint64(decoded), type);
        break;
    case DataType::UInt16:
        if (decoded > 65535ULL)
        {
            output.result = OperationResult::fail(
                QStringLiteral("bcd_range"),
                QStringLiteral("value"),
                QStringLiteral("BCD 解码结果超出 UINT16 范围"));
            return output;
        }
        output.value = RegisterValue::fromUnsigned64(decoded, type);
        break;
    case DataType::UInt32:
        if (decoded > 4294967295ULL)
        {
            output.result = OperationResult::fail(
                QStringLiteral("bcd_range"),
                QStringLiteral("value"),
                QStringLiteral("BCD 解码结果超出 UINT32 范围"));
            return output;
        }
        output.value = RegisterValue::fromUnsigned64(decoded, type);
        break;
    case DataType::UInt64:
        output.value = RegisterValue::fromUnsigned64(decoded, type);
        break;
    default:
        output.result = OperationResult::fail(
            QStringLiteral("bcd_type"),
            QStringLiteral("dataType"),
            QStringLiteral("不支持的 BCD 数据类型"));
        return output;
    }

    output.result = OperationResult::ok();
    return output;
}
} // namespace

ConversionResult ValueConverter::toRegisters(const RegisterValue &value, Endian endian)
{
    ConversionResult output;
    QByteArray bytes;

    if (isBcdEndian(endian))
    {
        if (!valueBytesBcd(value, &bytes, &output.result))
        {
            return output;
        }
    }
    else
    {
        bytes = valueBytesBinary(value);
        if (bytes.isEmpty())
        {
            output.result = OperationResult::fail(
                QStringLiteral("encode_failed"),
                QStringLiteral("value"),
                QStringLiteral("数值编码失败"));
            return output;
        }
    }

    applyEndian(&bytes, endian);
    output.registers = bytesToRegisters(bytes);
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
        output.result = OperationResult::fail(
            QStringLiteral("invalid_register_count"),
            QStringLiteral("registerCount"),
            QStringLiteral("寄存器数量与数据类型不匹配"));
        return output;
    }

    QByteArray bytes = registersToBytes(registers);
    applyEndian(&bytes, endian);

    if (isBcdEndian(endian))
    {
        return valueFromBcdBytes(type, bytes);
    }
    return valueFromBinaryBytes(type, bytes);
}
