#include "validation_service.h"

#include "Domain/Models/project_factory.h"

#include <QHash>
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
    // 寄存器名称允许为空（CSV 未填时保持空白）。
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

    QHash<QString, const RegisterGroup *> groupById;
    for (const RegisterGroup &group : document.groups)
    {
        groupById.insert(group.id, &group);
    }

    auto rangesOverlap = [](quint16 leftAddress, quint16 leftCount,
                            quint16 rightAddress, quint16 rightCount) -> bool {
        const quint32 leftEnd = quint32(leftAddress) + leftCount - 1U;
        const quint32 rightEnd = quint32(rightAddress) + rightCount - 1U;
        return leftAddress <= rightEnd && rightAddress <= leftEnd;
    };

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

        const RegisterGroup *pointGroup = groupById.value(point.groupId, nullptr);
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
            if (other.storageType != point.storageType)
            {
                continue;
            }
            if (!rangesOverlap(point.address, point.registerCount,
                               other.address, other.registerCount))
            {
                continue;
            }

            const RegisterGroup *otherGroup = groupById.value(other.groupId, nullptr);
            const bool sameGroup = (point.groupId == other.groupId);

            if (sameGroup)
            {
                // 同组内仍按从站+地址范围隔离检查
                if (other.slaveAddress != point.slaveAddress)
                {
                    continue;
                }
                return OperationResult::fail(
                    QStringLiteral("address_overlap"),
                    QStringLiteral("address"),
                    QStringLiteral("同组内寄存器地址范围重叠"),
                    QStringLiteral("分组「%1」：从站 %2，地址 %3 与 %4 重叠")
                        .arg(pointGroup ? pointGroup->name : point.groupId)
                        .arg(point.slaveAddress)
                        .arg(point.address)
                        .arg(other.address));
            }

            // 跨组：仅当两边都启用，且绑定同一非空端口时才冲突
            // 未绑定端口（portId 为空）不参与跨组冲突检查
            if (!pointGroup || !otherGroup)
            {
                continue;
            }
            if (!pointGroup->enabled || !otherGroup->enabled)
            {
                continue;
            }
            if (pointGroup->portId.isEmpty() || otherGroup->portId.isEmpty())
            {
                continue;
            }
            if (pointGroup->portId != otherGroup->portId)
            {
                continue;
            }

            QString portName = pointGroup->portId;
            for (const ConnectionPort &port : document.ports)
            {
                if (port.id == pointGroup->portId)
                {
                    portName = port.name;
                    break;
                }
            }

            return OperationResult::fail(
                QStringLiteral("address_overlap"),
                QStringLiteral("address"),
                QStringLiteral("同端口启用分组存在地址重叠"),
                QStringLiteral("端口「%1」上，分组「%2」地址 %3 与分组「%4」地址 %5 重叠（均已启用）")
                    .arg(portName,
                         pointGroup->name,
                         QString::number(point.address),
                         otherGroup->name,
                         QString::number(other.address)));
        }
    }
    return OperationResult::ok();
}
