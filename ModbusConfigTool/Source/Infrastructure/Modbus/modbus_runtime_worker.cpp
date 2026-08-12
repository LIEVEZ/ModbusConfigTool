#include "modbus_runtime_worker.h"

#include "Domain/Values/value_converter.h"
#include "Infrastructure/Modbus/multi_slave_modbus_server.h"
#include "Infrastructure/Strategy/strategy_engine.h"

#include <QHash>
#include <QStringList>

#include <algorithm>

namespace
{
struct PointRange
{
    quint8 slave;
    int start;
    int end;
};

// 把一个分组内指定表的点位按从站合并成连续区间，输出 S3[4096-4096] 形式。
// 仅用于日志展示，不改变实际映射块（由 ModbusRegisterStore 管理）。
QString mergeRangesText(const QList<RegisterPoint> &points, StorageType table)
{
    QVector<PointRange> ranges;
    for (const RegisterPoint &point : points)
    {
        if (point.storageType != table)
        {
            continue;
        }
        const int count = qMax(1, int(point.registerCount));
        ranges.append({point.slaveAddress, int(point.address), int(point.address) + count - 1});
    }
    if (ranges.isEmpty())
    {
        return QString();
    }
    std::sort(ranges.begin(), ranges.end(), [](const PointRange &left, const PointRange &right) {
        if (left.slave != right.slave)
        {
            return left.slave < right.slave;
        }
        return left.start < right.start;
    });

    QStringList parts;
    quint8 currentSlave = ranges.first().slave;
    int currentStart = ranges.first().start;
    int currentEnd = ranges.first().end;
    for (int index = 1; index < ranges.size(); ++index)
    {
        const PointRange &range = ranges.at(index);
        if (range.slave == currentSlave && range.start <= currentEnd + 1)
        {
            currentEnd = qMax(currentEnd, range.end);
            continue;
        }
        parts.append(QStringLiteral("S%1[%2-%3]")
                         .arg(currentSlave).arg(currentStart).arg(currentEnd));
        currentSlave = range.slave;
        currentStart = range.start;
        currentEnd = range.end;
    }
    parts.append(QStringLiteral("S%1[%2-%3]")
                     .arg(currentSlave).arg(currentStart).arg(currentEnd));
    return parts.join(QStringLiteral(", "));
}
} // namespace

ModbusRuntimeWorker::ModbusRuntimeWorker(QObject *parent)
    : QObject(parent)
{
}

void ModbusRuntimeWorker::start(const ServerProfile &profile,
                                const QList<RegisterPoint> &points,
                                const QHash<QString, QString> &groupNames)
{
    stop();
    m_profile = profile;
    m_groupNames = groupNames;

    m_server = new MultiSlaveModbusServer(&m_store, this);
    connect(m_server, &MultiSlaveModbusServer::frameCaptured,
            this, &ModbusRuntimeWorker::frameCaptured);
    connect(m_server, &MultiSlaveModbusServer::dataWritten,
            this, &ModbusRuntimeWorker::handleDataWritten);
    connect(m_server, &MultiSlaveModbusServer::errorOccurred, this, [this](const QString &message) {
        emit failed(QStringLiteral("Modbus 运行时发生错误"), message);
    });

    rebuildMap(points, false);
    if (m_points.isEmpty() || m_store.isEmpty())
    {
        emit failed(QStringLiteral("寄存器映射为空"),
                    QStringLiteral("当前端口没有可映射寄存器。请确认分组已绑定该端口、分组为启用状态，"
                                   "并且点位已导入后重新启动连接。"));
        stop();
        return;
    }

    if (!m_server->start(profile))
    {
        emit failed(QStringLiteral("无法启动 Modbus 运行时"), m_server->errorString());
        stop();
        return;
    }

    if (m_strategyEngine)
    {
        m_strategyEngine->stop();
        delete m_strategyEngine;
        m_strategyEngine = nullptr;
    }
    m_strategyEngine = new StrategyEngine(this);
    connect(m_strategyEngine, &StrategyEngine::valueReady,
            this, &ModbusRuntimeWorker::writePoint);
    m_strategyEngine->start(m_points.values());
    emit started();
}

void ModbusRuntimeWorker::reloadPoints(const QList<RegisterPoint> &points,
                                       const QHash<QString, QString> &groupNames)
{
    if (!m_server)
    {
        return;
    }

    m_groupNames = groupNames;
    rebuildMap(points, true);
    const QList<quint8> slaves = m_store.slaveAddresses();
    QStringList slaveText;
    for (quint8 slave : slaves)
    {
        slaveText.append(QString::number(slave));
    }
    emit diagnostics(QStringLiteral("寄存器映射已热更新（连接保持），从站: %1")
                         .arg(slaveText.isEmpty() ? QStringLiteral("-") : slaveText.join(QLatin1Char(','))));
}

void ModbusRuntimeWorker::rebuildMap(const QList<RegisterPoint> &points, bool restartStrategy)
{
    if (m_strategyEngine)
    {
        m_strategyEngine->stop();
        delete m_strategyEngine;
        m_strategyEngine = nullptr;
    }

    m_store.clear();
    m_points.clear();

    QList<RegisterPoint> enabledPoints;
    enabledPoints.reserve(points.size());
    for (const RegisterPoint &point : points)
    {
        // 点位启用字段已废弃，全部参与映射。
        // 从站地址保留点位自身配置，不再由端口覆盖。
        RegisterPoint mapped = point;
        mapped.enabled = true;
        if (mapped.slaveAddress < 1 || mapped.slaveAddress > 247)
        {
            mapped.slaveAddress = 1;
        }
        m_points.insert(mapped.id, mapped);
        enabledPoints.append(mapped);
    }

    m_store.build(enabledPoints, StorageType::Holding);
    m_store.build(enabledPoints, StorageType::Input);

    const QList<quint8> slaves = m_store.slaveAddresses();
    QStringList slaveText;
    for (quint8 slave : slaves)
    {
        slaveText.append(QString::number(slave));
    }

    emit diagnostics(QStringLiteral(
        "收到点位 %1，映射 %2；从站[%3]；Holding %4 块 %5；Input %6 块 %7")
        .arg(points.size())
        .arg(m_points.size())
        .arg(slaveText.isEmpty() ? QStringLiteral("-") : slaveText.join(QLatin1Char(',')))
        .arg(m_store.blockCount(QModbusDataUnit::HoldingRegisters))
        .arg(m_store.summary(QModbusDataUnit::HoldingRegisters))
        .arg(m_store.blockCount(QModbusDataUnit::InputRegisters))
        .arg(m_store.summary(QModbusDataUnit::InputRegisters)));

    // 按分组输出地址映射，便于确认每个分组绑定了哪些寄存器区间
    QHash<QString, QList<RegisterPoint>> pointsByGroup;
    const QList<RegisterPoint> mappedPoints = m_points.values();
    for (const RegisterPoint &point : mappedPoints)
    {
        pointsByGroup[point.groupId].append(point);
    }
    QList<QPair<QString, QString>> groupLines; // (分组名, 日志行)，按名称排序输出
    for (auto it = pointsByGroup.constBegin(); it != pointsByGroup.constEnd(); ++it)
    {
        const QString label = m_groupNames.value(it.key(), QStringLiteral("未命名分组"));
        const QString holdingText = mergeRangesText(it.value(), StorageType::Holding);
        const QString inputText = mergeRangesText(it.value(), StorageType::Input);
        QStringList sections;
        if (!holdingText.isEmpty())
        {
            sections.append(QStringLiteral("Holding %1").arg(holdingText));
        }
        if (!inputText.isEmpty())
        {
            sections.append(QStringLiteral("Input %1").arg(inputText));
        }
        if (sections.isEmpty())
        {
            continue;
        }
        groupLines.append(qMakePair(label, QStringLiteral("分组映射「%1」: %2（%3 点位）")
                                        .arg(label, sections.join(QStringLiteral("; ")),
                                             QString::number(it.value().size()))));
    }
    std::sort(groupLines.begin(), groupLines.end(),
              [](const QPair<QString, QString> &left, const QPair<QString, QString> &right) {
                  return QString::localeAwareCompare(left.first, right.first) < 0;
              });
    for (const auto &line : groupLines)
    {
        emit diagnostics(line.second);
    }

    if (!restartStrategy || m_points.isEmpty())
    {
        return;
    }

    m_strategyEngine = new StrategyEngine(this);
    connect(m_strategyEngine, &StrategyEngine::valueReady,
            this, &ModbusRuntimeWorker::writePoint);
    m_strategyEngine->start(m_points.values());
}

void ModbusRuntimeWorker::stop()
{
    if (m_strategyEngine)
    {
        m_strategyEngine->stop();
        delete m_strategyEngine;
        m_strategyEngine = nullptr;
    }
    if (m_server)
    {
        m_server->stop();
        delete m_server;
        m_server = nullptr;
    }
    m_points.clear();
    m_store.clear();
    emit stopped();
}

void ModbusRuntimeWorker::handleDataWritten(quint8 slaveAddress,
                                            QModbusDataUnit::RegisterType table,
                                            int address,
                                            int size)
{
    for (const RegisterPoint &point : m_points)
    {
        if (point.slaveAddress != slaveAddress)
        {
            continue;
        }
        const QModbusDataUnit::RegisterType pointTable = point.storageType == StorageType::Holding
            ? QModbusDataUnit::HoldingRegisters : QModbusDataUnit::InputRegisters;
        const int pointEnd = int(point.address) + point.registerCount - 1;
        const int writeEnd = address + size - 1;
        if (pointTable != table || point.address > writeEnd || address > pointEnd)
        {
            continue;
        }

        QVector<quint16> registers;
        for (int offset = 0; offset < point.registerCount; ++offset)
        {
            quint16 registerValue = 0;
            if (!m_store.readOne(point.slaveAddress, pointTable, int(point.address) + offset, &registerValue))
            {
                registers.clear();
                break;
            }
            registers.append(registerValue);
        }
        if (registers.size() != point.registerCount)
        {
            continue;
        }

        const ValueResult decoded = ValueConverter::fromRegisters(point.dataType, point.endian, registers);
        if (decoded.result.success)
        {
            RegisterPoint updated = point;
            updated.currentValue = decoded.value;
            m_points.insert(updated.id, updated);
            emit valueChanged(updated.id, decoded.value);
        }
    }
}

void ModbusRuntimeWorker::writePoint(const QString &pointId,
                                     const RegisterValue &value)
{
    if (!m_server || !m_points.contains(pointId))
    {
        return;
    }
    RegisterPoint point = m_points.value(pointId);
    const ConversionResult converted = ValueConverter::toRegisters(value, point.endian);
    if (!converted.result.success)
    {
        return;
    }
    const QModbusDataUnit::RegisterType table = point.storageType == StorageType::Holding
        ? QModbusDataUnit::HoldingRegisters : QModbusDataUnit::InputRegisters;
    for (int index = 0; index < converted.registers.size(); ++index)
    {
        if (!m_store.writeOne(point.slaveAddress, table, int(point.address) + index, converted.registers.at(index)))
        {
            emit failed(QStringLiteral("写入寄存器失败"),
                        QStringLiteral("从站 %1 地址 %2 未映射")
                            .arg(point.slaveAddress)
                            .arg(point.address + index));
            return;
        }
    }
    point.currentValue = value;
    m_points.insert(pointId, point);
    emit valueChanged(pointId, value);
}
