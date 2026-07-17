#ifndef VALUE_CONVERTER_H
#define VALUE_CONVERTER_H

#include "Domain/Models/domain_enums.h"
#include "Domain/Values/register_value.h"

#include <QVector>

struct ConversionResult
{
    OperationResult result;
    QVector<quint16> registers;
};

class ValueConverter
{
public:
    static ConversionResult toRegisters(const RegisterValue &value, Endian endian);
    static ValueResult fromRegisters(DataType type,
                                     Endian endian,
                                     const QVector<quint16> &registers);
};

#endif
