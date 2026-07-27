#include "Infrastructure/Persistence/json_project_repository.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <limits>

class ProjectRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsUnsigned64WithoutPrecisionLoss()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ProjectDocument document = ProjectFactory::createEmpty();
        RegisterPoint point = ProjectFactory::createRegister(document.groups.first().id, 0);
        point.dataType = DataType::UInt64;
        point.registerCount = 4;
        point.minimumValue = RegisterValue::fromUnsigned64(0, DataType::UInt64);
        point.maximumValue = RegisterValue::fromUnsigned64(std::numeric_limits<quint64>::max(), DataType::UInt64);
        point.currentValue = RegisterValue::fromUnsigned64(18446744073709551615ULL, DataType::UInt64);
        document.registers.append(point);
        JsonProjectRepository repository;
        const QString path = directory.filePath(QStringLiteral("roundtrip.mctproj"));

        QVERIFY(repository.save(path, document).success);
        const ProjectLoadResult loaded = repository.load(path);

        QVERIFY(loaded.result.success);
        QCOMPARE(loaded.document.registers.first().currentValue.toStorageString(),
                 QStringLiteral("18446744073709551615"));
    }

    void migratesSchemaV1ToV2()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("legacy_v1.mctproj"));

        // 手写一个最小 v1 工程文件（单 serverProfile + 无坐标分组）
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{\n"
                   "  \"schemaVersion\": 1,\n"
                   "  \"project\": { \"name\": \"旧工程\", \"description\": \"\", "
                   "\"createdAt\": \"\", \"updatedAt\": \"\" },\n"
                   "  \"serverProfile\": { \"connectionType\": \"TCP\", "
                   "\"tcpHost\": \"10.0.0.5\", \"tcpPort\": 502,\n"
                   "                     \"serialPort\": \"\", \"baudRate\": 9600, "
                   "\"parity\": \"N\", \"dataBits\": 8,\n"
                   "                     \"stopBits\": 1, \"pollIntervalMs\": 1000, "
                   "\"slaveAddress\": 1 },\n"
                   "  \"groups\": [ { \"id\": \"grp-a\", \"name\": \"分组A\", "
                   "\"color\": \"#f54e00\", \"description\": \"\",\n"
                   "                \"isDefault\": true } ],\n"
                   "  \"registers\": [],\n"
                   "  \"uiState\": { \"width\": 1440, \"height\": 900, "
                   "\"selectedGroupId\": \"grp-a\" }\n"
                   "}");
        file.close();

        JsonProjectRepository repository;
        const ProjectLoadResult loaded = repository.load(path);

        QVERIFY(loaded.result.success);
        QCOMPARE(loaded.document.schemaVersion, 2);
        QCOMPARE(loaded.document.ports.size(), 1);
        QCOMPARE(loaded.document.ports.first().profile.tcpHost, QStringLiteral("10.0.0.5"));
        QCOMPARE(loaded.document.ports.first().profile.tcpPort, quint16(502));
        // 所有分组绑定到迁移出的默认端口
        QCOMPARE(loaded.document.groups.first().portId, loaded.document.ports.first().id);
        QVERIFY(loaded.document.groups.first().enabled);
    }

    void roundTripsV2PortsAndGroupCanvas()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ProjectDocument document = ProjectFactory::createEmpty();
        document.ports.first().profile.tcpHost = QStringLiteral("192.168.1.20");
        document.groups.first().canvasX = 120;
        document.groups.first().canvasY = 80;
        document.groups.first().enabled = false;
        JsonProjectRepository repository;
        const QString path = directory.filePath(QStringLiteral("v2.mctproj"));

        QVERIFY(repository.save(path, document).success);
        const ProjectLoadResult loaded = repository.load(path);

        QVERIFY(loaded.result.success);
        QCOMPARE(loaded.document.schemaVersion, 2);
        QCOMPARE(loaded.document.ports.first().profile.tcpHost, QStringLiteral("192.168.1.20"));
        QCOMPARE(loaded.document.groups.first().canvasX, 120);
        QCOMPARE(loaded.document.groups.first().canvasY, 80);
        QVERIFY(!loaded.document.groups.first().enabled);
    }
};

REGISTER_TEST(ProjectRepositoryTest);

#include "test_project_repository.moc"
