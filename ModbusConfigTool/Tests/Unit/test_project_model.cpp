#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QTest>

class ProjectModelTest : public QObject
{
    Q_OBJECT

private slots:
    void createsExpectedDefaults()
    {
        const ProjectDocument document = ProjectFactory::createEmpty();

        QCOMPARE(document.schemaVersion, 1);
        QCOMPARE(document.serverProfile.tcpHost, QStringLiteral("127.0.0.1"));
        QCOMPARE(document.serverProfile.tcpPort, quint16(5020));
        QCOMPARE(document.groups.size(), 1);
        QVERIFY(document.groups.first().isDefault);
    }

    void mapsDataTypeToRegisterCount()
    {
        QCOMPARE(ProjectFactory::registerCountFor(DataType::UInt16), quint16(1));
        QCOMPARE(ProjectFactory::registerCountFor(DataType::Float32), quint16(2));
        QCOMPARE(ProjectFactory::registerCountFor(DataType::UInt64), quint16(4));
    }
};

REGISTER_TEST(ProjectModelTest);

#include "test_project_model.moc"
