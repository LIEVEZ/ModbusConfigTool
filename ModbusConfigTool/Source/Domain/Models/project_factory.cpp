#include "project_factory.h"

#include <QDateTime>
#include <QUuid>

#include <limits>

ProjectDocument ProjectFactory::createEmpty()
{
    ProjectDocument document;
    const QDateTime now = QDateTime::currentDateTime();
    document.project.createdAt = now;
    document.project.updatedAt = now;

    RegisterGroup defaultGroup;
    defaultGroup.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    defaultGroup.name = QStringLiteral("默认分组");
    defaultGroup.description = QStringLiteral("未分类寄存器");
    defaultGroup.isDefault = true;
    document.groups.append(defaultGroup);
    document.uiState.selectedGroupId = defaultGroup.id;
    return document;
}

RegisterPoint ProjectFactory::createRegister(const QString &groupId, quint16 address)
{
    RegisterPoint point;
    point.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    point.groupId = groupId;
    point.address = address;
    point.name = QStringLiteral("新寄存器");
    point.protocolKey = QStringLiteral("register_%1").arg(address);
    point.minimumValue = minimumFor(point.dataType);
    point.maximumValue = maximumFor(point.dataType);
    point.currentValue = RegisterValue::fromUnsigned64(0, point.dataType);
    return point;
}

quint16 ProjectFactory::registerCountFor(DataType type)
{
    switch (type)
    {
    case DataType::Int16:
    case DataType::UInt16: return 1;
    case DataType::Int32:
    case DataType::UInt32:
    case DataType::Float32: return 2;
    case DataType::Int64:
    case DataType::UInt64:
    case DataType::Float64: return 4;
    default: return 1;
    }
}

RegisterValue ProjectFactory::minimumFor(DataType type)
{
    switch (type)
    {
    case DataType::Int16: return RegisterValue::fromSigned64(-32768, type);
    case DataType::UInt16: return RegisterValue::fromUnsigned64(0, type);
    case DataType::Int32: return RegisterValue::fromSigned64(-2147483648LL, type);
    case DataType::UInt32: return RegisterValue::fromUnsigned64(0, type);
    case DataType::Float32: return RegisterValue::fromFloating(-3.4028235e38, type);
    case DataType::Int64: return RegisterValue::fromSigned64(std::numeric_limits<qint64>::min(), type);
    case DataType::UInt64: return RegisterValue::fromUnsigned64(0, type);
    case DataType::Float64: return RegisterValue::fromFloating(-std::numeric_limits<double>::max(), type);
    default: return RegisterValue();
    }
}

RegisterValue ProjectFactory::maximumFor(DataType type)
{
    switch (type)
    {
    case DataType::Int16: return RegisterValue::fromSigned64(32767, type);
    case DataType::UInt16: return RegisterValue::fromUnsigned64(65535, type);
    case DataType::Int32: return RegisterValue::fromSigned64(2147483647LL, type);
    case DataType::UInt32: return RegisterValue::fromUnsigned64(4294967295ULL, type);
    case DataType::Float32: return RegisterValue::fromFloating(3.4028235e38, type);
    case DataType::Int64: return RegisterValue::fromSigned64(std::numeric_limits<qint64>::max(), type);
    case DataType::UInt64: return RegisterValue::fromUnsigned64(std::numeric_limits<quint64>::max(), type);
    case DataType::Float64: return RegisterValue::fromFloating(std::numeric_limits<double>::max(), type);
    default: return RegisterValue();
    }
}
