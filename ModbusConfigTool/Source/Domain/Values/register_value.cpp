#include "register_value.h"

#include <limits>

RegisterValue::RegisterValue() = default;

RegisterValue RegisterValue::fromSigned64(qint64 value, DataType type)
{
    RegisterValue result;
    result.m_type = type;
    result.m_value = QVariant::fromValue(value);
    return result;
}

RegisterValue RegisterValue::fromUnsigned64(quint64 value, DataType type)
{
    RegisterValue result;
    result.m_type = type;
    result.m_value = QVariant::fromValue(value);
    return result;
}

RegisterValue RegisterValue::fromFloating(double value, DataType type)
{
    RegisterValue result;
    result.m_type = type;
    result.m_value = value;
    return result;
}

ValueResult RegisterValue::fromText(DataType type, const QString &text)
{
    ValueResult output;
    bool valid = false;

    switch (type)
    {
    case DataType::Int16:
    case DataType::Int32:
    case DataType::Int64:
    {
        const qint64 value = text.toLongLong(&valid);
        qint64 minimum = std::numeric_limits<qint64>::min();
        qint64 maximum = std::numeric_limits<qint64>::max();
        if (type == DataType::Int16) { minimum = -32768; maximum = 32767; }
        if (type == DataType::Int32) { minimum = -2147483648LL; maximum = 2147483647LL; }
        valid = valid && value >= minimum && value <= maximum;
        output.value = fromSigned64(value, type);
        break;
    }
    case DataType::UInt16:
    case DataType::UInt32:
    case DataType::UInt64:
    {
        const quint64 value = text.toULongLong(&valid);
        quint64 maximum = std::numeric_limits<quint64>::max();
        if (type == DataType::UInt16) { maximum = 65535ULL; }
        if (type == DataType::UInt32) { maximum = 4294967295ULL; }
        valid = valid && value <= maximum && !text.trimmed().startsWith('-');
        output.value = fromUnsigned64(value, type);
        break;
    }
    case DataType::Float32:
    case DataType::Float64:
    {
        const double value = text.toDouble(&valid);
        output.value = fromFloating(value, type);
        break;
    }
    default:
        valid = false;
        break;
    }

    output.result = valid
        ? OperationResult::ok()
        : OperationResult::fail(QStringLiteral("invalid_value"),
                                QStringLiteral("value"),
                                QStringLiteral("数值超出数据类型允许范围"), text);
    return output;
}

DataType RegisterValue::dataType() const { return m_type; }
qint64 RegisterValue::toSigned64() const { return m_value.toLongLong(); }
quint64 RegisterValue::toUnsigned64() const { return m_value.toULongLong(); }
double RegisterValue::toDouble() const { return m_value.toDouble(); }

QString RegisterValue::toStorageString() const
{
    if (m_type == DataType::UInt64) { return QString::number(toUnsigned64()); }
    if (m_type == DataType::Int64) { return QString::number(toSigned64()); }
    if (m_type == DataType::Float32 || m_type == DataType::Float64)
    {
        return QString::number(toDouble(), 'g', 17);
    }
    return m_value.toString();
}

QString RegisterValue::toDisplayString(int precision) const
{
    if (m_type == DataType::Float32 || m_type == DataType::Float64)
    {
        // 浮点用 g 格式，避免超大/超小数被 f+低精度裁成难读长整数，并尽量完整展示有效数字。
        const int digits = precision <= 0 ? 8 : qBound(1, precision, 15);
        return QString::number(toDouble(), 'g', digits);
    }
    return toStorageString();
}
