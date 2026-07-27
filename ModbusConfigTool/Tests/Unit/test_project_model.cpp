#include "Domain/Models/connection_port.h"
#include "Domain/Models/project_factory.h"
#include "Domain/Models/register_group.h"
#include "test_registry.h"

#include <QTest>

class ProjectModelTest : public QObject
{
    Q_OBJECT

private slots:
    void createsExpectedDefaults()
    {
        const ProjectDocument document = ProjectFactory::createEmpty();

        QCOMPARE(document.schemaVersion, 2);
        QCOMPARE(document.ports.size(), 1);
        QCOMPARE(document.ports.first().profile.tcpHost, QStringLiteral("127.0.0.1"));
        QCOMPARE(document.ports.first().profile.tcpPort, quint16(5020));
        QVERIFY(!document.ports.first().id.isEmpty());
        QCOMPARE(document.groups.size(), 1);
        QVERIFY(document.groups.first().isDefault);
        QCOMPARE(document.groups.first().portId, document.ports.first().id);
    }

    void mapsDataTypeToRegisterCount()
    {
        QCOMPARE(ProjectFactory::registerCountFor(DataType::UInt16), quint16(1));
        QCOMPARE(ProjectFactory::registerCountFor(DataType::Float32), quint16(2));
        QCOMPARE(ProjectFactory::registerCountFor(DataType::UInt64), quint16(4));
    }

    void registerGroupHasCanvasAndBindingDefaults()
    {
        RegisterGroup group;
        QCOMPARE(group.enabled, true);
        QVERIFY(group.portId.isEmpty());
        QCOMPARE(group.canvasX, 0);
        QCOMPARE(group.canvasY, 0);
    }

    void connectionPortEmbedsServerProfileDefaults()
    {
        ConnectionPort port;
        QVERIFY(port.id.isEmpty());
        QVERIFY(port.name.isEmpty());
        QCOMPARE(port.profile.tcpPort, quint16(5020));
        QCOMPARE(port.profile.connectionType, ConnectionType::Tcp);
    }
};

REGISTER_TEST(ProjectModelTest);

#include "test_project_model.moc"
