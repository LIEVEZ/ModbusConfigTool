#include "Application/Runtime/runtime_service.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QModbusReply>
#include <QModbusTcpClient>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTest>

class ModbusRuntimeTest : public QObject
{
    Q_OBJECT

private slots:
    void tcpClientReadsHoldingRegister()
    {
        QTcpServer portProbe;
        QVERIFY(portProbe.listen(QHostAddress::LocalHost, 0));
        const quint16 port = portProbe.serverPort();
        portProbe.close();

        ProjectDocument document = ProjectFactory::createEmpty();
        document.serverProfile.tcpHost = QStringLiteral("127.0.0.1");
        document.serverProfile.tcpPort = port;
        document.serverProfile.slaveAddress = 1;
        RegisterPoint point = ProjectFactory::createRegister(document.groups.first().id, 0);
        point.currentValue = RegisterValue::fromUnsigned64(42, DataType::UInt16);
        document.registers.append(point);

        RuntimeService service;
        service.start(document);
        QTRY_COMPARE_WITH_TIMEOUT(service.state(), RuntimeState::Running, 1000);

        QModbusTcpClient client;
        client.setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                      QStringLiteral("127.0.0.1"));
        client.setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
        QVERIFY(client.connectDevice());
        QTRY_COMPARE_WITH_TIMEOUT(client.state(), QModbusDevice::ConnectedState, 1000);

        QModbusReply *reply = client.sendReadRequest(
            QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 0, 1), 1);
        QVERIFY(reply);
        QSignalSpy finishedSpy(reply, &QModbusReply::finished);
        if (!reply->isFinished()) { QVERIFY(finishedSpy.wait(1000)); }
        QCOMPARE(reply->error(), QModbusDevice::NoError);
        QCOMPARE(reply->result().value(0), quint16(42));
        reply->deleteLater();
        client.disconnectDevice();

        service.stop();
        QTRY_COMPARE_WITH_TIMEOUT(service.state(), RuntimeState::Idle, 1000);
    }
};

REGISTER_TEST(ModbusRuntimeTest);

#include "test_modbus_runtime.moc"
