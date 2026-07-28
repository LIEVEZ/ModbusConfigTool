#include "domain_enums.h"

#include <QList>

QString dataTypeToString(DataType type)
{
    switch (type)
    {
    case DataType::Int16: return QStringLiteral("INT16");
    case DataType::UInt16: return QStringLiteral("UINT16");
    case DataType::Int32: return QStringLiteral("INT32");
    case DataType::UInt32: return QStringLiteral("UINT32");
    case DataType::Float32: return QStringLiteral("FLOAT32");
    case DataType::Int64: return QStringLiteral("INT64");
    case DataType::UInt64: return QStringLiteral("UINT64");
    case DataType::Float64: return QStringLiteral("FLOAT64");
    default: return QStringLiteral("UINT16");
    }
}

bool dataTypeFromString(const QString &text, DataType *type)
{
    if (!type)
    {
        return false;
    }

    const QString value = text.trimmed().toUpper();
    // 兼容旧协议/CSV 别名
    if (value == QStringLiteral("FLOAT") || value == QStringLiteral("FLOAT32"))
    {
        *type = DataType::Float32;
        return true;
    }
    if (value == QStringLiteral("DFLOAT")
        || value == QStringLiteral("FLOAT64")
        || value == QStringLiteral("DOUBLE"))
    {
        *type = DataType::Float64;
        return true;
    }

    const QList<DataType> types = {DataType::Int16, DataType::UInt16, DataType::Int32,
                                   DataType::UInt32, DataType::Float32, DataType::Int64,
                                   DataType::UInt64, DataType::Float64};
    for (DataType candidate : types)
    {
        if (dataTypeToString(candidate) == value)
        {
            *type = candidate;
            return true;
        }
    }
    return false;
}

QString endianToString(Endian endian)
{
    switch (endian)
    {
    case Endian::Big: return QStringLiteral("BIG");
    case Endian::Little: return QStringLiteral("LITTLE");
    case Endian::LittleSwap: return QStringLiteral("LITSWAP");
    case Endian::BigSwap: return QStringLiteral("BIGSWAP");
    case Endian::BigBcd: return QStringLiteral("BIGBCD");
    case Endian::LittleBcd: return QStringLiteral("LITBCD");
    default: return QStringLiteral("BIG");
    }
}

bool endianFromString(const QString &text, Endian *endian)
{
    if (!endian)
    {
        return false;
    }

    const QString value = text.trimmed().toUpper();
    if (value == QStringLiteral("BIG")) { *endian = Endian::Big; return true; }
    if (value == QStringLiteral("LITTLE")) { *endian = Endian::Little; return true; }
    if (value == QStringLiteral("LITSWAP")) { *endian = Endian::LittleSwap; return true; }
    if (value == QStringLiteral("BIGSWAP")) { *endian = Endian::BigSwap; return true; }
    if (value == QStringLiteral("BIGBCD")) { *endian = Endian::BigBcd; return true; }
    if (value == QStringLiteral("LITBCD")) { *endian = Endian::LittleBcd; return true; }
    return false;
}

QString storageTypeToString(StorageType type)
{
    return type == StorageType::Holding ? QStringLiteral("holding")
                                        : QStringLiteral("input");
}

bool storageTypeFromString(const QString &text, StorageType *type)
{
    const QString value = text.trimmed().toLower();
    if (value == QStringLiteral("holding")) { *type = StorageType::Holding; return true; }
    if (value == QStringLiteral("input")) { *type = StorageType::Input; return true; }
    return false;
}

QString strategyTypeToString(StrategyType type)
{
    switch (type)
    {
    case StrategyType::None: return QStringLiteral("none");
    case StrategyType::Linear: return QStringLiteral("linear");
    case StrategyType::Random: return QStringLiteral("random");
    case StrategyType::SineWave: return QStringLiteral("sine_wave");
    default: return QStringLiteral("none");
    }
}

bool strategyTypeFromString(const QString &text, StrategyType *type)
{
    const QString value = text.trimmed().toLower();
    const QList<StrategyType> types = {StrategyType::None, StrategyType::Linear,
                                       StrategyType::Random, StrategyType::SineWave};
    for (StrategyType candidate : types)
    {
        if (strategyTypeToString(candidate) == value)
        {
            *type = candidate;
            return true;
        }
    }
    return false;
}

QString runtimeStateToString(RuntimeState state)
{
    switch (state)
    {
    case RuntimeState::Idle: return QStringLiteral("空闲");
    case RuntimeState::Starting: return QStringLiteral("启动中");
    case RuntimeState::Running: return QStringLiteral("运行中");
    case RuntimeState::Stopping: return QStringLiteral("停止中");
    case RuntimeState::Fault: return QStringLiteral("故障");
    default: return QStringLiteral("空闲");
    }
}
