#include "Infrastructure/Persistence/csv_register_gateway_impl.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QTemporaryDir>
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
};

REGISTER_TEST(CsvGatewayTest);

#include "test_csv_gateway.moc"
