#include "modbus_runtime_worker.h"

#include "Domain/Values/value_converter.h"
#include "Infrastructure/Modbus/multi_slave_modbus_server.h"
#include "Infrastructure/Strategy/strategy_engine.h"

#include <QHash>
#include <QStringList>

#include <algorithm>

ModbusRuntimeWorker::ModbusRuntimeWorker(QObject *parent)
    : QObject(parent)
{
}

void ModbusRuntimeWorker::start(const ServerProfile &profile,
                                const QList<RegisterPoint> &points)
{
    stop();
    m_profile = profile;

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

void ModbusRuntimeWorker::reloadPoints(const QList<RegisterPoint> &points)
{
    if (!m_server)
    {
        return;
    }

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
