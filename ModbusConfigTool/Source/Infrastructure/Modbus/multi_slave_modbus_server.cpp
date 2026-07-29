#include "multi_slave_modbus_server.h"

#include "Domain/Models/domain_enums.h"

#include <QHostAddress>
#include <QModbusRequest>
#include <QModbusResponse>
#include <QSerialPort>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <cstring>

namespace
{
QByteArray encodePdu(const QModbusPdu &pdu)
{
    QByteArray bytes;
    const quint8 functionByte = pdu.isException()
        ? quint8(quint8(pdu.functionCode()) | 0x80)
        : quint8(pdu.functionCode());
    bytes.append(char(functionByte));
    bytes.append(pdu.data());
    return bytes;
}
}

MultiSlaveModbusServer::MultiSlaveModbusServer(ModbusRegisterStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
}

MultiSlaveModbusServer::~MultiSlaveModbusServer()
{
    stop();
}

bool MultiSlaveModbusServer::start(const ServerProfile &profile)
{
    stop();
    m_errorString.clear();

    const bool ok = profile.connectionType == ConnectionType::Tcp
        ? startTcp(profile)
        : startRtu(profile);
    m_running = ok;
    if (!ok && m_errorString.isEmpty())
    {
        m_errorString = QStringLiteral("多从站服务启动失败");
    }
    return ok;
}

void MultiSlaveModbusServer::stop()
{
    if (m_rtuSilenceTimer)
    {
        m_rtuSilenceTimer->stop();
        m_rtuSilenceTimer->deleteLater();
        m_rtuSilenceTimer = nullptr;
    }
    if (m_serial)
    {
        m_serial->close();
        m_serial->deleteLater();
        m_serial = nullptr;
    }
    const QList<QTcpSocket *> sockets = m_tcpClients.keys();
    m_tcpClients.clear();
    for (QTcpSocket *socket : sockets)
    {
        if (!socket)
        {
            continue;
        }
        socket->disconnect(this);
        socket->deleteLater();
    }
    if (m_tcpServer)
    {
        m_tcpServer->close();
        m_tcpServer->deleteLater();
        m_tcpServer = nullptr;
    }
    m_rtuBuffer.clear();
    m_running = false;
}

bool MultiSlaveModbusServer::startTcp(const ServerProfile &profile)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &MultiSlaveModbusServer::onTcpNewConnection);

    QHostAddress address = QHostAddress::Any;
    const QString host = profile.tcpHost.trimmed();
    if (!host.isEmpty() && host != QStringLiteral("0.0.0.0"))
    {
        const QHostAddress parsed(host);
        if (!parsed.isNull())
        {
            address = parsed;
        }
    }

    if (!m_tcpServer->listen(address, profile.tcpPort))
    {
        m_errorString = m_tcpServer->errorString();
        m_tcpServer->deleteLater();
        m_tcpServer = nullptr;
        return false;
    }
    return true;
}

bool MultiSlaveModbusServer::startRtu(const ServerProfile &profile)
{
    m_serial = new QSerialPort(this);
    m_serial->setPortName(profile.serialPort);
    m_baudRate = profile.baudRate > 0 ? profile.baudRate : 9600;
    m_serial->setBaudRate(m_baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setStopBits(QSerialPort::OneStop);
    const QSerialPort::Parity parity = profile.parity == QLatin1Char('E') ? QSerialPort::EvenParity
        : profile.parity == QLatin1Char('O') ? QSerialPort::OddParity
                                              : QSerialPort::NoParity;
    m_serial->setParity(parity);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite))
    {
        m_errorString = m_serial->errorString();
        m_serial->deleteLater();
        m_serial = nullptr;
        return false;
    }
    m_serial->clear();

    m_rtuSilenceTimer = new QTimer(this);
    m_rtuSilenceTimer->setSingleShot(true);
    connect(m_rtuSilenceTimer, &QTimer::timeout, this, &MultiSlaveModbusServer::onSerialSilenceTimeout);
    connect(m_serial, &QSerialPort::readyRead, this, &MultiSlaveModbusServer::onSerialReadyRead);
    connect(m_serial,
            QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this,
            [this](QSerialPort::SerialPortError error) {
                if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError)
                {
                    return;
                }
                if (m_serial)
                {
                    emit errorOccurred(m_serial->errorString());
                }
            });
    return true;
}

int MultiSlaveModbusServer::rtuSilenceMs() const
{
    const int charBits = 11;
    return qMax(5, (charBits * 3500 + m_baudRate - 1) / m_baudRate);
}

quint16 MultiSlaveModbusServer::crc16(const char *data, int length)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < length; ++i)
    {
        crc ^= static_cast<quint8>(data[i]);
        for (int bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x0001)
            {
                crc = quint16((crc >> 1) ^ 0xA001);
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

QByteArray MultiSlaveModbusServer::pduToBytes(const QModbusPdu &pdu)
{
    return encodePdu(pdu);
}

QByteArray MultiSlaveModbusServer::buildRtuAdu(quint8 slaveAddress, const QModbusResponse &response)
{
    QByteArray adu;
    adu.append(char(slaveAddress));
    adu.append(encodePdu(response));
    const quint16 crc = crc16(adu.constData(), adu.size());
    adu.append(char(crc & 0xFF));
    adu.append(char((crc >> 8) & 0xFF));
    return adu;
}

QByteArray MultiSlaveModbusServer::buildTcpAdu(quint16 transactionId,
                                              quint8 unitId,
                                              const QModbusResponse &response)
{
    const QByteArray pdu = encodePdu(response);
    QByteArray adu(7 + pdu.size(), Qt::Uninitialized);
    adu[0] = char((transactionId >> 8) & 0xFF);
    adu[1] = char(transactionId & 0xFF);
    adu[2] = 0;
    adu[3] = 0;
    const quint16 length = quint16(pdu.size() + 1);
    adu[4] = char((length >> 8) & 0xFF);
    adu[5] = char(length & 0xFF);
    adu[6] = char(unitId);
    memcpy(adu.data() + 7, pdu.constData(), size_t(pdu.size()));
    return adu;
}

bool MultiSlaveModbusServer::dispatchRequest(quint8 slaveAddress,
                                             const QModbusPdu &request,
                                             QModbusResponse *response)
{
    if (!m_store || !response)
    {
        return false;
    }

    // 广播地址：记录请求但不回复
    if (slaveAddress == 0)
    {
        const CommFrame rx = CommFrameFactory::fromRequest(request);
        emit frameCaptured(rx);
        return false;
    }

    const CommFrame rx = CommFrameFactory::fromRequest(request);
    emit frameCaptured(rx);

    *response = m_store->processRequest(slaveAddress, request);
    if (!response->isValid())
    {
        return false;
    }

    emit frameCaptured(CommFrameFactory::fromResponse(*response, rx));

    if (!response->isException())
    {
        const auto functionCode = request.functionCode();
        if (functionCode == QModbusPdu::WriteSingleRegister)
        {
            quint16 address = 0;
            quint16 value = 0;
            request.decodeData(&address, &value);
            Q_UNUSED(value);
            emit dataWritten(slaveAddress, QModbusDataUnit::HoldingRegisters, int(address), 1);
        }
        else if (functionCode == QModbusPdu::WriteMultipleRegisters)
        {
            quint16 address = 0;
            quint16 count = 0;
            quint8 byteCount = 0;
            request.decodeData(&address, &count, &byteCount);
            Q_UNUSED(byteCount);
            emit dataWritten(slaveAddress, QModbusDataUnit::HoldingRegisters, int(address), int(count));
        }
    }
    return true;
}

void MultiSlaveModbusServer::onSerialReadyRead()
{
    if (!m_serial)
    {
        return;
    }
    m_rtuBuffer.append(m_serial->readAll());
    if (m_rtuSilenceTimer)
    {
        m_rtuSilenceTimer->start(rtuSilenceMs());
    }
}

void MultiSlaveModbusServer::onSerialSilenceTimeout()
{
    processRtuBuffer();
}

void MultiSlaveModbusServer::processRtuBuffer()
{
    while (m_rtuBuffer.size() >= 4)
    {
        const quint8 slaveAddress = quint8(m_rtuBuffer.at(0));
        const quint8 rawFunction = quint8(m_rtuBuffer.at(1));
        const QModbusPdu::FunctionCode functionCode =
            QModbusPdu::FunctionCode(rawFunction & 0x7F);
        const QModbusRequest probe(functionCode, m_rtuBuffer.mid(2));
        const int dataSize = QModbusRequest::calculateDataSize(probe);
        if (dataSize < 0)
        {
            m_rtuBuffer.remove(0, 1);
            continue;
        }

        const int frameSize = 2 + dataSize + 2;
        if (m_rtuBuffer.size() < frameSize)
        {
            return;
        }

        const QByteArray frame = m_rtuBuffer.left(frameSize);
        m_rtuBuffer.remove(0, frameSize);

        const quint16 gotCrc = quint16(quint8(frame.at(frameSize - 2)))
            | (quint16(quint8(frame.at(frameSize - 1))) << 8);
        const quint16 calcCrc = crc16(frame.constData(), frameSize - 2);
        if (gotCrc != calcCrc)
        {
            continue;
        }

        const QModbusRequest request(functionCode, frame.mid(2, dataSize));
        QModbusResponse response;
        if (!dispatchRequest(slaveAddress, request, &response))
        {
            continue;
        }
        if (!m_serial || !m_serial->isOpen())
        {
            return;
        }
        m_serial->write(buildRtuAdu(slaveAddress, response));
    }
}

void MultiSlaveModbusServer::onTcpNewConnection()
{
    if (!m_tcpServer)
    {
        return;
    }
    while (m_tcpServer->hasPendingConnections())
    {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        if (!socket)
        {
            continue;
        }
        m_tcpClients.insert(socket, TcpClientState());
        connect(socket, &QTcpSocket::readyRead, this, &MultiSlaveModbusServer::onTcpReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &MultiSlaveModbusServer::onTcpDisconnected);
    }
}

void MultiSlaveModbusServer::onTcpDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
    {
        return;
    }
    m_tcpClients.remove(socket);
    socket->deleteLater();
}

void MultiSlaveModbusServer::onTcpReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket || !m_tcpClients.contains(socket))
    {
        return;
    }
    m_tcpClients[socket].buffer.append(socket->readAll());
    processTcpBuffer(socket);
}

void MultiSlaveModbusServer::processTcpBuffer(QTcpSocket *socket)
{
    if (!socket || !m_tcpClients.contains(socket))
    {
        return;
    }

    QByteArray &buffer = m_tcpClients[socket].buffer;
    while (buffer.size() >= 7)
    {
        const quint16 transactionId = quint16((quint8(buffer.at(0)) << 8) | quint8(buffer.at(1)));
        const quint16 protocolId = quint16((quint8(buffer.at(2)) << 8) | quint8(buffer.at(3)));
        const quint16 length = quint16((quint8(buffer.at(4)) << 8) | quint8(buffer.at(5)));
        if (protocolId != 0 || length < 2)
        {
            buffer.remove(0, 1);
            continue;
        }
        if (buffer.size() < int(6 + length))
        {
            return;
        }

        const QByteArray packet = buffer.left(6 + length);
        buffer.remove(0, 6 + length);

        const quint8 unitId = quint8(packet.at(6));
        if (length < 2)
        {
            continue;
        }
        const quint8 rawFunction = quint8(packet.at(7));
        const QByteArray pduData = packet.mid(8, length - 2);
        const QModbusRequest request(QModbusPdu::FunctionCode(rawFunction & 0x7F), pduData);

        QModbusResponse response;
        if (!dispatchRequest(unitId, request, &response))
        {
            continue;
        }
        if (socket->state() != QAbstractSocket::ConnectedState)
        {
            return;
        }
        socket->write(buildTcpAdu(transactionId, unitId, response));
    }
}
