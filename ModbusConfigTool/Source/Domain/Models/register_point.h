#ifndef REGISTER_POINT_H
#define REGISTER_POINT_H

#include "Domain/Models/domain_enums.h"
#include "Domain/Models/strategy_spec.h"
#include "Domain/Values/register_value.h"

#include <QString>

struct RegisterPoint
{
    QString id;
    QString groupId;
    quint8 slaveAddress = 1;
    quint16 address = 0;
    quint16 registerCount = 1;
    QString name;
    DataType dataType = DataType::UInt16;
    Endian endian = Endian::Big;
    StorageType storageType = StorageType::Holding;
    quint8 readFunctionCode = 3;
    int writeFunctionCode = 6;
    QString protocolKey;
    QString unit;
    double offset = 0.0;
    int precision = 0;
    RegisterValue minimumValue;
    RegisterValue maximumValue = RegisterValue::fromUnsigned64(65535, DataType::UInt16);
    RegisterValue currentValue;
    bool enabled = true; // 已废弃：点位一律生效，保留字段仅兼容旧工程
    StrategySpec strategy;
    QString category;
    QString label;
};

#endif
