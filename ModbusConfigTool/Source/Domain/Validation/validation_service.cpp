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
    if (profile.slaveAddress < 1 || profile.slaveAddress > 247)
    {
        return OperationResult::fail(QStringLiteral("invalid_server_address"),
                                     QStringLiteral("slaveAddress"),
                                     QStringLiteral("服务器从站地址必须位于 1～247"));
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
    // 协议键允许为空（CSV 未填时保持空白，不做默认生成）。
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
    if (point.strategy.type == StrategyType::None && point.strategy.enabled)
    {
        return OperationResult::fail(QStringLiteral("invalid_strategy_state"),
                                     QStringLiteral("strategy.enabled"),
                                     QStringLiteral("无策略时不能启用策略"));
    }
    if (point.strategy.enabled)
    {
        const QVariantMap &params = point.strategy.parameters;
        const int interval = params.value(QStringLiteral("intervalMs")).toInt();
        if (interval < 10 || interval > 60000)
        {
            return OperationResult::fail(QStringLiteral("invalid_strategy_interval"),
                                         QStringLiteral("strategy.intervalMs"),
                                         QStringLiteral("策略周期必须位于 10～60000 ms"));
        }
        if (point.strategy.type == StrategyType::Linear
            && (params.value(QStringLiteral("step")).toDouble() <= 0.0
                || params.value(QStringLiteral("startValue")).toDouble()
                   > params.value(QStringLiteral("endValue")).toDouble()))
        {
            return OperationResult::fail(QStringLiteral("invalid_linear_strategy"),
                                         QStringLiteral("strategy"),
                                         QStringLiteral("线性策略起止值或步长无效"));
        }
        if (point.strategy.type == StrategyType::Random
            && params.value(QStringLiteral("minValue")).toDouble()
               > params.value(QStringLiteral("maxValue")).toDouble())
        {
            return OperationResult::fail(QStringLiteral("invalid_random_strategy"),
                                         QStringLiteral("strategy"),
                                         QStringLiteral("随机策略最小值不能大于最大值"));
        }
        if (point.strategy.type == StrategyType::SineWave
            && (params.value(QStringLiteral("amplitude")).toDouble() < 0.0
                || params.value(QStringLiteral("frequencyHz")).toDouble() <= 0.0))
        {
            return OperationResult::fail(QStringLiteral("invalid_sine_strategy"),
                                         QStringLiteral("strategy"),
                                         QStringLiteral("正弦策略振幅或频率无效"));
        }
    }
    return OperationResult::ok();
}

OperationResult ValidationService::validateProject(const ProjectDocument &document)
{
    OperationResult result;
    QSet<QString> portIds;
    for (const ConnectionPort &port : document.ports)
    {
        if (port.id.isEmpty() || port.name.trimmed().isEmpty())
        {
            return OperationResult::fail(QStringLiteral("invalid_port"),
                                         QStringLiteral("ports"),
                                         QStringLiteral("端口 ID 和名称不能为空"));
        }
        if (portIds.contains(port.id))
        {
            return OperationResult::fail(QStringLiteral("duplicate_port"),
                                         QStringLiteral("ports"),
                                         QStringLiteral("端口 ID 重复"));
        }
        portIds.insert(port.id);
        result = validateServerProfile(port.profile);
        if (!result.success) { return result; }
    }

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
        if (!group.portId.isEmpty() && !portIds.contains(group.portId))
        {
            return OperationResult::fail(QStringLiteral("missing_port"),
                                         QStringLiteral("portId"),
                                         QStringLiteral("分组绑定的端口不存在"));
        }
        if (group.canvasX < 0 || group.canvasY < 0)
        {
            return OperationResult::fail(QStringLiteral("invalid_canvas_coord"),
                                         QStringLiteral("canvas"),
                                         QStringLiteral("分组画布坐标不能为负"));
        }
    }

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
