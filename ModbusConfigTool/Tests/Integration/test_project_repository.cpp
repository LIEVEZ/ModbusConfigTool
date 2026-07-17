#include "Infrastructure/Persistence/json_project_repository.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

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
};

REGISTER_TEST(ProjectRepositoryTest);

#include "test_project_repository.moc"
