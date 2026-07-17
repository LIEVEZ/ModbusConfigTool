#include "modbus_runtime_worker.h"

#include "Domain/Values/value_converter.h"
#include "Infrastructure/Strategy/strategy_engine.h"

#include <QModbusDataUnitMap>
#include <QModbusRtuSerialSlave>
#include <QModbusTcpServer>
#include <QSerialPort>

ModbusRuntimeWorker::ModbusRuntimeWorker(QObject *parent) : QObject(parent) {}

void ModbusRuntimeWorker::start(const ServerProfile &profile,
                                const QList<RegisterPoint> &points)
{
    stop();
    m_server = profile.connectionType == ConnectionType::Tcp
        ? static_cast<QModbusServer *>(new QModbusTcpServer(this))
        : static_cast<QModbusServer *>(new QModbusRtuSerialSlave(this));
    m_server->setServerAddress(profile.slaveAddress);
    m_points.clear();
    QModbusDataUnitMap map;
    int holdingStart = 65536;
    int holdingEnd = -1;
    int inputStart = 65536;
    int inputEnd = -1;
    for (const RegisterPoint &point : points)
    {
        if (!point.enabled || point.slaveAddress != profile.slaveAddress) { continue; }
        const int end = int(point.address) + point.registerCount - 1;
        if (point.storageType == StorageType::Holding)
        {
            holdingStart = qMin(holdingStart, int(point.address));
            holdingEnd = qMax(holdingEnd, end);
        }
        else
        {
            inputStart = qMin(inputStart, int(point.address));
            inputEnd = qMax(inputEnd, end);
        }
    }
    QVector<quint16> holding(qMax(0, holdingEnd - holdingStart + 1), 0);
    QVector<quint16> input(qMax(0, inputEnd - inputStart + 1), 0);
    for (const RegisterPoint &point : points)
    {
        if (!point.enabled || point.slaveAddress != profile.slaveAddress) { continue; }
        m_points.insert(point.id, point);
        const ConversionResult encoded = ValueConverter::toRegisters(point.currentValue, point.endian);
        QVector<quint16> &target = point.storageType == StorageType::Holding ? holding : input;
        const int start = point.storageType == StorageType::Holding ? holdingStart : inputStart;
        for (int index = 0; index < encoded.registers.size(); ++index)
        {
            target[int(point.address) - start + index] = encoded.registers.at(index);
        }
    }
    if (!holding.isEmpty()) { map.insert(QModbusDataUnit::HoldingRegisters, QModbusDataUnit(QModbusDataUnit::HoldingRegisters, holdingStart, holding)); }
    if (!input.isEmpty()) { map.insert(QModbusDataUnit::InputRegisters, QModbusDataUnit(QModbusDataUnit::InputRegisters, inputStart, input)); }
    m_server->setMap(map);
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
    connect(m_server, &QModbusServer::dataWritten, this,
            [this](QModbusDataUnit::RegisterType table, int address, int size)
    {
        for (const RegisterPoint &point : m_points)
        {
            const QModbusDataUnit::RegisterType pointTable = point.storageType == StorageType::Holding
                ? QModbusDataUnit::HoldingRegisters : QModbusDataUnit::InputRegisters;
            const int pointEnd = int(point.address) + point.registerCount - 1;
            const int writeEnd = address + size - 1;
            if (pointTable != table || point.address > writeEnd || address > pointEnd) { continue; }
            QVector<quint16> registers;
            for (int offset = 0; offset < point.registerCount; ++offset)
            {
                quint16 registerValue = 0;
                if (!m_server->data(pointTable, point.address + offset, &registerValue)) { registers.clear(); break; }
                registers.append(registerValue);
            }
            if (registers.size() == point.registerCount)
            {
                const ValueResult decoded = ValueConverter::fromRegisters(point.dataType, point.endian, registers);
                if (decoded.result.success) { emit valueChanged(point.id, decoded.value); }
            }
        }
    });
    if (!m_server->connectDevice())
    {
        emit failed(QStringLiteral("无法启动 Modbus 运行时"), m_server->errorString());
        stop();
        return;
    }
    m_strategyEngine = new StrategyEngine(this);
    connect(m_strategyEngine, &StrategyEngine::valueReady,
            this, &ModbusRuntimeWorker::writePoint);
    m_strategyEngine->start(m_points.values());
    emit started();
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
    emit stopped();
}

void ModbusRuntimeWorker::writePoint(const QString &pointId,
                                     const RegisterValue &value)
{
    if (!m_server || !m_points.contains(pointId)) { return; }
    RegisterPoint point = m_points.value(pointId);
    const ConversionResult converted = ValueConverter::toRegisters(value, point.endian);
    if (!converted.result.success) { return; }
    const QModbusDataUnit::RegisterType table = point.storageType == StorageType::Holding
        ? QModbusDataUnit::HoldingRegisters : QModbusDataUnit::InputRegisters;
    for (int index = 0; index < converted.registers.size(); ++index)
    {
        if (!m_server->setData(table, point.address + index, converted.registers.at(index)))
        {
            emit failed(QStringLiteral("写入寄存器失败"), m_server->errorString());
            return;
        }
    }
    point.currentValue = value;
    m_points.insert(pointId, point);
    emit valueChanged(pointId, value);
}
