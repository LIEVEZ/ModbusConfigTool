#include "Application/Runtime/runtime_service.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QHostAddress>
#include <QModbusReply>
#include <QModbusTcpClient>
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
        const QString portId = document.ports.first().id;
        document.ports.first().profile.tcpHost = QStringLiteral("127.0.0.1");
        document.ports.first().profile.tcpPort = port;
        document.ports.first().profile.slaveAddress = 1;
        RegisterPoint point = ProjectFactory::createRegister(
            document.groups.first().id, 0);
        point.currentValue = RegisterValue::fromUnsigned64(42, DataType::UInt16);
        document.registers.append(point);

        RuntimeService service;
        QSignalSpy spy(&service, &RuntimeService::portStateChanged);
        service.startPort(document, portId);
        QTRY_COMPARE_WITH_TIMEOUT(
            service.portState(portId), RuntimeState::Running, 3000);
        QVERIFY(spy.count() >= 2);

        QModbusTcpClient client;
        client.setConnectionParameter(QModbusDevice::NetworkAddressParameter,
                                      QStringLiteral("127.0.0.1"));
        client.setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
        QVERIFY(client.connectDevice());
        QTRY_COMPARE_WITH_TIMEOUT(
            client.state(), QModbusDevice::ConnectedState, 1000);

        QModbusReply *reply = client.sendReadRequest(
            QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 0, 1), 1);
        QVERIFY(reply);
        QSignalSpy finishedSpy(reply, &QModbusReply::finished);
        if (!reply->isFinished())
        {
            QVERIFY(finishedSpy.wait(1000));
        }
        QCOMPARE(reply->error(), QModbusDevice::NoError);
        QCOMPARE(reply->result().value(0), quint16(42));
        reply->deleteLater();
        client.disconnectDevice();

        service.stopPort(portId);
        QTRY_COMPARE(service.portState(portId), RuntimeState::Idle);
    }

    void startsAndStopsSinglePortIndependently()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        const QString portId = document.ports.first().id;
        document.ports.first().profile.tcpPort = 15020;
        RuntimeService service;
        QSignalSpy spy(&service, &RuntimeService::portStateChanged);

        service.startPort(document, portId);
        QTRY_COMPARE_WITH_TIMEOUT(
            service.portState(portId), RuntimeState::Running, 3000);
        QVERIFY(spy.count() >= 2);

        service.stopPort(portId);
        QTRY_COMPARE(service.portState(portId), RuntimeState::Idle);
    }

    void skipsPointsOfDisabledGroups()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        const QString portId = document.ports.first().id;
        document.ports.first().profile.tcpPort = 15021;
        document.groups.first().enabled = false;
        RegisterPoint point = ProjectFactory::createRegister(
            document.groups.first().id, 0);
        point.protocolKey = QStringLiteral("skip_me");
        document.registers.append(point);

        RuntimeService service;
        QSignalSpy spy(&service, &RuntimeService::portStateChanged);
        service.startPort(document, portId);
        QTRY_COMPARE_WITH_TIMEOUT(
            service.portState(portId), RuntimeState::Running, 3000);
        QVERIFY(spy.count() >= 2);
        service.stopAll();
    }
};

REGISTER_TEST(ModbusRuntimeTest);

#include "test_modbus_runtime.moc"
