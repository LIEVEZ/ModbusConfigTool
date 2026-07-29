#ifndef MULTI_SLAVE_MODBUS_SERVER_H
#define MULTI_SLAVE_MODBUS_SERVER_H

#include "Domain/Models/comm_frame.h"
#include "Domain/Models/server_profile.h"
#include "Infrastructure/Modbus/modbus_register_store.h"

#include <QByteArray>
#include <QHash>
#include <QModbusDataUnit>
#include <QModbusPdu>
#include <QObject>

class QSerialPort;
class QTcpServer;
class QTcpSocket;
class QTimer;

// 多从站 Modbus Server/Slave 后端：
// - 同一 TCP 端口 / 同一串口上按 unit id 路由到不同从站映射
// - 未知从站保持静默（不应答）
class MultiSlaveModbusServer : public QObject
{
    Q_OBJECT

public:
    explicit MultiSlaveModbusServer(ModbusRegisterStore *store, QObject *parent = nullptr);
    ~MultiSlaveModbusServer() override;

    bool start(const ServerProfile &profile);
    void stop();
    bool isRunning() const { return m_running; }
    QString errorString() const { return m_errorString; }

signals:
    void dataWritten(quint8 slaveAddress, QModbusDataUnit::RegisterType table, int address, int size);
    void frameCaptured(const CommFrame &frame);
    void errorOccurred(const QString &message);

private slots:
    void onSerialReadyRead();
    void onSerialSilenceTimeout();
    void onTcpNewConnection();
    void onTcpReadyRead();
    void onTcpDisconnected();

private:
    struct TcpClientState
    {
        QByteArray buffer;
    };

    bool startTcp(const ServerProfile &profile);
    bool startRtu(const ServerProfile &profile);
    void processRtuBuffer();
    void processTcpBuffer(QTcpSocket *socket);
    bool dispatchRequest(quint8 slaveAddress, const QModbusPdu &request, QModbusResponse *response);
    static quint16 crc16(const char *data, int length);
    static QByteArray buildRtuAdu(quint8 slaveAddress, const QModbusResponse &response);
    static QByteArray buildTcpAdu(quint16 transactionId, quint8 unitId, const QModbusResponse &response);
    static QByteArray pduToBytes(const QModbusPdu &pdu);
    int rtuSilenceMs() const;

    ModbusRegisterStore *m_store = nullptr;
    QSerialPort *m_serial = nullptr;
    QTcpServer *m_tcpServer = nullptr;
    QTimer *m_rtuSilenceTimer = nullptr;
    QByteArray m_rtuBuffer;
    QHash<QTcpSocket *, TcpClientState> m_tcpClients;
    QString m_errorString;
    bool m_running = false;
    int m_baudRate = 9600;
};

#endif
