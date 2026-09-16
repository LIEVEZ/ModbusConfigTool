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
    // 按大端顺序把寄存器序列组合成无符号整数（registers[0] 为最高位）
    static quint64 registersToUnsigned64(const QVector<quint16> &registers);
};

#endif
