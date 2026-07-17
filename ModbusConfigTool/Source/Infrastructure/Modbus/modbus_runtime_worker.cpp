#include "modbus_runtime_worker.h"

#include "Domain/Values/value_converter.h"

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
    QModbusDataUnitMap map;
    QVector<quint16> holding(65536, 0);
    QVector<quint16> input(65536, 0);
    bool hasHolding = false;
    bool hasInput = false;
    for (const RegisterPoint &point : points)
    {
        if (!point.enabled || point.slaveAddress != profile.slaveAddress) { continue; }
        const ConversionResult encoded = ValueConverter::toRegisters(point.currentValue, point.endian);
        QVector<quint16> &target = point.storageType == StorageType::Holding ? holding : input;
        for (int index = 0; index < encoded.registers.size(); ++index)
        {
            target[int(point.address) + index] = encoded.registers.at(index);
        }
        hasHolding |= point.storageType == StorageType::Holding;
        hasInput |= point.storageType == StorageType::Input;
    }
    if (hasHolding) { map.insert(QModbusDataUnit::HoldingRegisters, QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 0, holding)); }
    if (hasInput) { map.insert(QModbusDataUnit::InputRegisters, QModbusDataUnit(QModbusDataUnit::InputRegisters, 0, input)); }
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
    if (!m_server->connectDevice())
    {
        emit failed(QStringLiteral("无法启动 Modbus 运行时"), m_server->errorString());
        stop();
        return;
    }
    emit started();
}

void ModbusRuntimeWorker::stop()
{
    if (m_server)
    {
        m_server->disconnectDevice();
        delete m_server;
        m_server = nullptr;
    }
    emit stopped();
}
