#include "Infrastructure/Persistence/csv_register_gateway_impl.h"
#include "Domain/Models/project_factory.h"
#include "Application/Registers/register_service.h"
#include "Application/Project/project_service.h"
#include "Infrastructure/Persistence/json_project_repository.h"
#include "test_registry.h"

#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTest>

class CsvGatewayTest : public QObject
{
    Q_OBJECT

private slots:
    void exportsAndImportsQuotedText()
    {
        QTemporaryDir directory;
        ProjectDocument document = ProjectFactory::createEmpty();
        RegisterPoint point = ProjectFactory::createRegister(document.groups.first().id, 0);
        point.name = QStringLiteral("温度,主通道");
        point.protocolKey = QStringLiteral("temperature_main");
        document.registers.append(point);
        CsvRegisterGatewayImpl gateway;
        const QString path = directory.filePath(QStringLiteral("points.csv"));

        QVERIFY(gateway.exportFile(path, document).success);
        const CsvImportResult imported = gateway.importFile(path, ProjectFactory::createEmpty());

        QVERIFY(imported.result.success);
        QCOMPARE(imported.registers.size(), 1);
        QCOMPARE(imported.registers.first().name, QStringLiteral("温度,主通道"));
    }

    void importsEnergyStyleCsvAliases()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("energy_like.csv"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("\xEF\xBB\xBF");
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream << "slave_addr,start_addr,quantity,name,data_type,encode_mode,offset,precision,unit, read,write,protocol_key,upload,label\n";
        stream << "1,101,1,pcsActivePower,INT16,BIG,0,10,,0x04,,pcsActivePowerSum,TRUE,emsData\n";
        stream.flush();
        file.close();

        CsvRegisterGatewayImpl gateway;
        const CsvImportResult imported = gateway.importFile(path, ProjectFactory::createEmpty());
        QVERIFY2(imported.result.success, qPrintable(imported.result.message));
        QCOMPARE(imported.registers.size(), 1);
        const RegisterPoint &point = imported.registers.first();
        QCOMPARE(point.slaveAddress, quint8(1));
        QCOMPARE(point.address, quint16(101));
        QCOMPARE(point.protocolKey, QStringLiteral("pcsActivePowerSum"));
        QCOMPARE(point.dataType, DataType::Int16);
        QCOMPARE(point.endian, Endian::Big);
        QCOMPARE(point.readFunctionCode, quint8(4));
        QCOMPARE(point.storageType, StorageType::Input);
        QCOMPARE(point.label, QStringLiteral("emsData"));
        QCOMPARE(point.precision, 10);
    }

    void importsEnergyGtYhsIfPresent()
    {
        const QString path = QStringLiteral("E:/04_Git/01_github/ModbusConfigTool/energy_gt_yhs.csv");
        if (!QFile::exists(path))
        {
            QSKIP("energy_gt_yhs.csv not found");
        }

        CsvRegisterGatewayImpl gateway;
        const CsvImportResult imported = gateway.importFile(path, ProjectFactory::createEmpty());
        QVERIFY2(imported.result.success,
                 qPrintable(imported.result.message + QLatin1Char(' ') + imported.result.detail));
        QVERIFY(imported.registers.size() >= 300);

        QSet<QString> keys;
        for (const RegisterPoint &point : imported.registers)
        {
            QVERIFY(!point.protocolKey.isEmpty());
            QVERIFY(!keys.contains(point.protocolKey));
            keys.insert(point.protocolKey);
        }
    }

    void importIntoGroupForcesGroupId()
    {
        JsonProjectRepository repository;
        ProjectService projectService(&repository);
        projectService.newProject();
        RegisterService service(&projectService);
        const QString groupId = projectService.document().groups.first().id;

        CsvImportResult imported;
        RegisterPoint a = ProjectFactory::createRegister(groupId, 0);
        a.protocolKey = QStringLiteral("k_a");
        RegisterPoint b = ProjectFactory::createRegister(groupId, 1);
        b.protocolKey = QStringLiteral("k_b");
        imported.registers = {a, b};
        imported.result = OperationResult::ok();

        const OperationResult result = service.importCsvIntoGroup(groupId, imported, false);
        QVERIFY2(result.success, qPrintable(result.message));
        for (const RegisterPoint &point : projectService.document().registers)
        {
            QCOMPARE(point.groupId, groupId);
        }
        QCOMPARE(projectService.document().registers.size(), 2);
    }
};

REGISTER_TEST(CsvGatewayTest);

#include "test_csv_gateway.moc"
