#include "modbus_runtime_worker.h"

#include "Domain/Values/value_converter.h"
#include "Infrastructure/Strategy/strategy_engine.h"

#include <QModbusDataUnitMap>
#include <QModbusRtuSerialSlave>
#include <QModbusTcpServer>
#include <QSerialPort>

#include <functional>

namespace
{
using FrameHandler = std::function<void(const CommFrame &)>;

class BlockModbusTcpServer : public QModbusTcpServer
{
public:
    explicit BlockModbusTcpServer(ModbusRegisterStore *store,
                                  FrameHandler frameHandler,
                                  QObject *parent = nullptr)
        : QModbusTcpServer(parent), m_store(store), m_frameHandler(std::move(frameHandler))
    {
    }

protected:
    QModbusResponse processRequest(const QModbusPdu &request) override
    {
        const CommFrame rx = CommFrameFactory::fromRequest(request);
        if (m_frameHandler)
        {
            m_frameHandler(rx);
        }

        const QModbusPdu::FunctionCode functionCode = request.functionCode();
        const QModbusResponse response = m_store->processRequest(request);

        if (m_frameHandler)
        {
            m_frameHandler(CommFrameFactory::fromResponse(response, rx));
        }

        if (!response.isException())
        {
            if (functionCode == QModbusPdu::WriteSingleRegister)
            {
                quint16 address = 0;
                quint16 value = 0;
                request.decodeData(&address, &value);
                Q_UNUSED(value);
                emit dataWritten(QModbusDataUnit::HoldingRegisters, int(address), 1);
            }
            else if (functionCode == QModbusPdu::WriteMultipleRegisters)
            {
                quint16 address = 0;
                quint16 count = 0;
                quint8 byteCount = 0;
                request.decodeData(&address, &count, &byteCount);
                Q_UNUSED(byteCount);
                emit dataWritten(QModbusDataUnit::HoldingRegisters, int(address), int(count));
            }
        }
        return response;
    }

private:
    ModbusRegisterStore *m_store = nullptr;
    FrameHandler m_frameHandler;
};

class BlockModbusRtuSlave : public QModbusRtuSerialSlave
{
public:
    explicit BlockModbusRtuSlave(ModbusRegisterStore *store,
                                 FrameHandler frameHandler,
                                 QObject *parent = nullptr)
        : QModbusRtuSerialSlave(parent), m_store(store), m_frameHandler(std::move(frameHandler))
    {
    }

protected:
    QModbusResponse processRequest(const QModbusPdu &request) override
    {
        const CommFrame rx = CommFrameFactory::fromRequest(request);
        if (m_frameHandler)
        {
            m_frameHandler(rx);
        }

        const QModbusPdu::FunctionCode functionCode = request.functionCode();
        const QModbusResponse response = m_store->processRequest(request);

        if (m_frameHandler)
        {
            m_frameHandler(CommFrameFactory::fromResponse(response, rx));
        }

        if (!response.isException())
        {
            if (functionCode == QModbusPdu::WriteSingleRegister)
            {
                quint16 address = 0;
                quint16 value = 0;
                request.decodeData(&address, &value);
                Q_UNUSED(value);
                emit dataWritten(QModbusDataUnit::HoldingRegisters, int(address), 1);
            }
            else if (functionCode == QModbusPdu::WriteMultipleRegisters)
            {
                quint16 address = 0;
                quint16 count = 0;
                quint8 byteCount = 0;
                request.decodeData(&address, &count, &byteCount);
                Q_UNUSED(byteCount);
                emit dataWritten(QModbusDataUnit::HoldingRegisters, int(address), int(count));
            }
        }
        return response;
    }

private:
    ModbusRegisterStore *m_store = nullptr;
    FrameHandler m_frameHandler;
};
}

ModbusRuntimeWorker::ModbusRuntimeWorker(QObject *parent) : QObject(parent) {}

void ModbusRuntimeWorker::start(const ServerProfile &profile,
                                const QList<RegisterPoint> &points)
{
    stop();
    m_profile = profile;

    FrameHandler frameHandler = [this](const CommFrame &frame) {
        emit frameCaptured(frame);
    };

    m_server = profile.connectionType == ConnectionType::Tcp
        ? static_cast<QModbusServer *>(new BlockModbusTcpServer(&m_store, frameHandler, this))
        : static_cast<QModbusServer *>(new BlockModbusRtuSlave(&m_store, frameHandler, this));
    m_server->setServerAddress(profile.slaveAddress);

    rebuildMap(points, false);
    if (m_points.isEmpty() || m_store.isEmpty())
    {
        emit failed(QStringLiteral("寄存器映射为空"),
                    QStringLiteral("当前端口没有可映射寄存器。请确认分组已绑定该端口、分组为启用状态，"
                                   "并且点位已导入后重新启动连接。"));
        stop();
        return;
    }

    if (profile.connectionType == ConnectionType::Tcp)
    {
        m_server->setConnectionParameter(QModbusDevice::NetworkAddressParameter, profile.tcpHost);
        m_server->setConnectionParameter(QModbusDevice::NetworkPortParameter, profile.tcpPort);
    }
    else
    {
        m_server->setConnectionParameter(QModbusDevice::SerialPortNameParameter, profile.serialPort);
        m_server->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, profile.baudRate);
        m_server->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
        m_server->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);
        const QSerialPort::Parity parity = profile.parity == QLatin1Char('E') ? QSerialPort::EvenParity
            : profile.parity == QLatin1Char('O') ? QSerialPort::OddParity : QSerialPort::NoParity;
        m_server->setConnectionParameter(QModbusDevice::SerialParityParameter, parity);
    }

    connect(m_server, &QModbusDevice::errorOccurred, this, [this](QModbusDevice::Error error)
    {
        if (error != QModbusDevice::NoError && m_server)
        {
            emit failed(QStringLiteral("Modbus 运行时发生错误"), m_server->errorString());
        }
    });
    connect(m_server, &QModbusServer::dataWritten, this, &ModbusRuntimeWorker::handleDataWritten);

    if (!m_server->connectDevice())
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
    emit diagnostics(QStringLiteral("寄存器映射已热更新（连接保持）"));
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
        RegisterPoint mapped = point;
        mapped.enabled = true;
        mapped.registerCount = quint16(qMax(1, int(point.registerCount)));
        mapped.slaveAddress = m_profile.slaveAddress;
        m_points.insert(mapped.id, mapped);
        enabledPoints.append(mapped);
    }

    m_store.build(enabledPoints, StorageType::Holding);
    m_store.build(enabledPoints, StorageType::Input);

    emit diagnostics(QStringLiteral(
        "收到点位 %1，映射 %2；Holding %3 块 %4；Input %5 块 %6")
        .arg(points.size())
        .arg(m_points.size())
        .arg(m_store.blockCount(QModbusDataUnit::HoldingRegisters))
        .arg(m_store.summary(QModbusDataUnit::HoldingRegisters))
        .arg(m_store.blockCount(QModbusDataUnit::InputRegisters))
        .arg(m_store.summary(QModbusDataUnit::InputRegisters)));

    if (m_server)
    {
        QModbusDataUnitMap map;
        auto appendCover = [&](QModbusDataUnit::RegisterType table, StorageType storageType) {
            int start = 65536;
            int end = -1;
            for (const RegisterPoint &point : enabledPoints)
            {
                if (point.storageType != storageType)
                {
                    continue;
                }
                start = qMin(start, int(point.address));
                end = qMax(end, int(point.address) + int(point.registerCount) - 1);
            }
            if (end >= start)
            {
                map.insert(table, QModbusDataUnit(table, start, quint16(end - start + 1)));
            }
        };
        appendCover(QModbusDataUnit::HoldingRegisters, StorageType::Holding);
        appendCover(QModbusDataUnit::InputRegisters, StorageType::Input);
        m_server->setMap(map);
        m_server->setServerAddress(m_profile.slaveAddress);
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
    if (m_server)
    {
        if (m_strategyEngine)
        {
            m_strategyEngine->stop();
            delete m_strategyEngine;
            m_strategyEngine = nullptr;
        }
        m_server->disconnectDevice();
        delete m_server;
        m_server = nullptr;
    }
    m_points.clear();
    m_store.clear();
    emit stopped();
}

void ModbusRuntimeWorker::handleDataWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    for (const RegisterPoint &point : m_points)
    {
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
            if (!m_store.readOne(pointTable, int(point.address) + offset, &registerValue))
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
        if (!m_store.writeOne(table, int(point.address) + index, converted.registers.at(index)))
        {
            emit failed(QStringLiteral("写入寄存器失败"),
                        QStringLiteral("地址 %1 未映射").arg(point.address + index));
            return;
        }
    }
    point.currentValue = value;
    m_points.insert(pointId, point);
    emit valueChanged(pointId, value);
}
