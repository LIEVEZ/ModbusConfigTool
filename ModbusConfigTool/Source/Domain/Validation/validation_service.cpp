#include "validation_service.h"

#include "Domain/Models/project_factory.h"

#include <QSet>

OperationResult ValidationService::validateServerProfile(const ServerProfile &profile)
{
    if (profile.pollIntervalMs < 100 || profile.pollIntervalMs > 60000)
    {
        return OperationResult::fail(QStringLiteral("invalid_poll_interval"),
                                     QStringLiteral("pollIntervalMs"),
                                     QStringLiteral("轮询周期必须位于 100～60000 ms"));
    }
    if (profile.connectionType == ConnectionType::Tcp && profile.tcpHost.trimmed().isEmpty())
    {
        return OperationResult::fail(QStringLiteral("empty_host"),
                                     QStringLiteral("tcpHost"),
                                     QStringLiteral("TCP 主机不能为空"));
    }
    if (profile.connectionType == ConnectionType::Rtu && profile.serialPort.trimmed().isEmpty())
    {
        return OperationResult::fail(QStringLiteral("empty_serial_port"),
                                     QStringLiteral("serialPort"),
                                     QStringLiteral("RTU 串口不能为空"));
    }
    return OperationResult::ok();
}

OperationResult ValidationService::validateRegister(const RegisterPoint &point)
{
    if (point.name.trimmed().isEmpty())
    {
        return OperationResult::fail(QStringLiteral("empty_name"),
                                     QStringLiteral("name"),
                                     QStringLiteral("寄存器名称不能为空"));
    }
    if (point.protocolKey.trimmed().isEmpty())
    {
        return OperationResult::fail(QStringLiteral("empty_protocol_key"),
                                     QStringLiteral("protocolKey"),
                                     QStringLiteral("协议键不能为空"));
    }
    if (point.slaveAddress < 1 || point.slaveAddress > 247)
    {
        return OperationResult::fail(QStringLiteral("invalid_slave"),
                                     QStringLiteral("slaveAddress"),
                                     QStringLiteral("从站地址必须位于 1～247"));
    }
    if (point.registerCount != ProjectFactory::registerCountFor(point.dataType))
    {
        return OperationResult::fail(QStringLiteral("invalid_register_count"),
                                     QStringLiteral("registerCount"),
                                     QStringLiteral("寄存器数量与数据类型不匹配"));
    }
    return OperationResult::ok();
}

OperationResult ValidationService::validateProject(const ProjectDocument &document)
{
    OperationResult result = validateServerProfile(document.serverProfile);
    if (!result.success) { return result; }

    QSet<QString> groupIds;
    QSet<QString> groupNames;
    for (const RegisterGroup &group : document.groups)
    {
        if (group.id.isEmpty() || group.name.trimmed().isEmpty())
        {
            return OperationResult::fail(QStringLiteral("invalid_group"),
                                         QStringLiteral("groups"),
                                         QStringLiteral("分组 ID 和名称不能为空"));
        }
        if (groupIds.contains(group.id) || groupNames.contains(group.name.trimmed()))
        {
            return OperationResult::fail(QStringLiteral("duplicate_group"),
                                         QStringLiteral("groups"),
                                         QStringLiteral("分组 ID 或名称重复"));
        }
        groupIds.insert(group.id);
        groupNames.insert(group.name.trimmed());
    }

    QSet<QString> protocolKeys;
    for (int index = 0; index < document.registers.size(); ++index)
    {
        const RegisterPoint &point = document.registers.at(index);
        result = validateRegister(point);
        if (!result.success) { return result; }
        if (!groupIds.contains(point.groupId))
        {
            return OperationResult::fail(QStringLiteral("missing_group"),
                                         QStringLiteral("groupId"),
                                         QStringLiteral("寄存器引用的分组不存在"));
        }
        if (protocolKeys.contains(point.protocolKey))
        {
            return OperationResult::fail(QStringLiteral("duplicate_protocol_key"),
                                         QStringLiteral("protocolKey"),
                                         QStringLiteral("协议键不允许重复"));
        }
        protocolKeys.insert(point.protocolKey);

        const quint32 pointEnd = quint32(point.address) + point.registerCount - 1U;
        if (pointEnd > 65535U)
        {
            return OperationResult::fail(QStringLiteral("address_overflow"),
                                         QStringLiteral("address"),
                                         QStringLiteral("寄存器地址范围超出 65535"));
        }
        for (int otherIndex = 0; otherIndex < index; ++otherIndex)
        {
            const RegisterPoint &other = document.registers.at(otherIndex);
            if (other.slaveAddress != point.slaveAddress
                || other.storageType != point.storageType)
            {
                continue;
            }
            const quint32 otherEnd = quint32(other.address) + other.registerCount - 1U;
            if (point.address <= otherEnd && other.address <= pointEnd)
            {
                return OperationResult::fail(QStringLiteral("address_overlap"),
                                             QStringLiteral("address"),
                                             QStringLiteral("寄存器地址范围发生重叠"));
            }
        }
    }
    return OperationResult::ok();
}
