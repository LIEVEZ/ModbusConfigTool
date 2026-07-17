#include "Domain/Validation/validation_service.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QTest>

class ValidationTest : public QObject
{
    Q_OBJECT

private slots:
    void rejectsOverlappingRanges()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        RegisterPoint first = ProjectFactory::createRegister(document.groups.first().id, 10);
        RegisterPoint second = ProjectFactory::createRegister(document.groups.first().id, 10);
        second.protocolKey = QStringLiteral("overlap_second");
        document.registers = {first, second};

        const OperationResult result = ValidationService::validateProject(document);
        QVERIFY(!result.success);
        QCOMPARE(result.code, QStringLiteral("address_overlap"));
    }

    void rejectsDuplicateProtocolKey()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        RegisterPoint first = ProjectFactory::createRegister(document.groups.first().id, 1);
        RegisterPoint second = ProjectFactory::createRegister(document.groups.first().id, 2);
        second.protocolKey = first.protocolKey;
        document.registers = {first, second};

        QCOMPARE(ValidationService::validateProject(document).code,
                 QStringLiteral("duplicate_protocol_key"));
    }
};

REGISTER_TEST(ValidationTest);

#include "test_validation.moc"
