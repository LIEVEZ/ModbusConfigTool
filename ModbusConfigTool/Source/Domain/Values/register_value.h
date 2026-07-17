#ifndef REGISTER_VALUE_H
#define REGISTER_VALUE_H

#include "Domain/Models/domain_enums.h"
#include "Domain/Values/operation_result.h"

#include <QVariant>

class RegisterValue;
struct ValueResult;

class RegisterValue
{
public:
    RegisterValue();

    static RegisterValue fromSigned64(qint64 value, DataType type = DataType::Int64);
    static RegisterValue fromUnsigned64(quint64 value, DataType type = DataType::UInt64);
    static RegisterValue fromFloating(double value, DataType type = DataType::Float64);
    static ValueResult fromText(DataType type, const QString &text);

    DataType dataType() const;
    qint64 toSigned64() const;
    quint64 toUnsigned64() const;
    double toDouble() const;
    QString toStorageString() const;
    QString toDisplayString(int precision = 6) const;

private:
    DataType m_type = DataType::UInt16;
    QVariant m_value = QVariant::fromValue<quint64>(0);
};

struct ValueResult
{
    OperationResult result;
    RegisterValue value;
};

Q_DECLARE_METATYPE(RegisterValue)

#endif
