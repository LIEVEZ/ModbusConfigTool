#ifndef DOMAIN_ENUMS_H
#define DOMAIN_ENUMS_H

#include <QString>

enum class ConnectionType
{
    Tcp,
    Rtu
};

enum class DataType
{
    Int16,
    UInt16,
    Int32,
    UInt32,
    Float32,
    Int64,
    UInt64,
    Float64
};

enum class Endian
{
    Big,
    Little,
    LittleSwap
};

enum class StorageType
{
    Holding,
    Input
};

enum class StrategyType
{
    None,
    Linear,
    Random,
    SineWave
};

enum class RuntimeState
{
    Idle,
    Starting,
    Running,
    Stopping,
    Fault
};

QString dataTypeToString(DataType type);
bool dataTypeFromString(const QString &text, DataType *type);
QString endianToString(Endian endian);
bool endianFromString(const QString &text, Endian *endian);
QString storageTypeToString(StorageType type);
bool storageTypeFromString(const QString &text, StorageType *type);
QString strategyTypeToString(StrategyType type);
bool strategyTypeFromString(const QString &text, StrategyType *type);
QString runtimeStateToString(RuntimeState state);

#endif
