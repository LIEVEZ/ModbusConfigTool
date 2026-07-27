#include "Domain/Validation/validation_service.h"
#include "Domain/Models/project_factory.h"
#include "Domain/Models/connection_port.h"
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

    void rejectsGroupBoundToMissingPort()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.groups.first().portId = QStringLiteral("no-such-port");
        const OperationResult result = ValidationService::validateProject(document);
        QVERIFY(!result.success);
        QCOMPARE(result.code, QStringLiteral("missing_port"));
    }

    void rejectsDuplicatePortId()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        ConnectionPort dup = document.ports.first();
        document.ports.append(dup);
        const OperationResult result = ValidationService::validateProject(document);
        QVERIFY(!result.success);
        QCOMPARE(result.code, QStringLiteral("duplicate_port"));
    }

    void rejectsNegativeCanvasCoordinate()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.groups.first().canvasX = -5;
        const OperationResult result = ValidationService::validateProject(document);
        QVERIFY(!result.success);
        QCOMPARE(result.code, QStringLiteral("invalid_canvas_coord"));
    }

    void acceptsEmptyPortId()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.groups.first().portId.clear();
        const OperationResult result = ValidationService::validateProject(document);
        QVERIFY(result.success);
    }
};

REGISTER_TEST(ValidationTest);

#include "test_validation.moc"
