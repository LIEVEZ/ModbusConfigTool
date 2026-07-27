#include "Application/Connections/connection_service.h"
#include "Application/Project/project_service.h"
#include "Infrastructure/Persistence/json_project_repository.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QTest>

class ConnectionServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void addPortAppendsAndAssignsId()
    {
        JsonProjectRepository repository;
        ProjectService projectService(&repository);
        projectService.newProject();
        ConnectionService service(&projectService);

        ConnectionPort port;
        port.name = QStringLiteral("现场 PLC");
        const OperationResult result = service.addPort(port);

        QVERIFY(result.success);
        QCOMPARE(projectService.document().ports.size(), 2); // 默认端口 + 新端口
        QVERIFY(!projectService.document().ports.last().id.isEmpty());
    }

    void removePortUnbindsGroups()
    {
        JsonProjectRepository repository;
        ProjectService projectService(&repository);
        projectService.newProject();
        ConnectionService service(&projectService);

        const QString portId = projectService.document().ports.first().id;
        QVERIFY(service.removePort(portId).success);

        QCOMPARE(projectService.document().ports.size(), 0);
        for (const RegisterGroup &group : projectService.document().groups)
        {
            QVERIFY(group.portId.isEmpty()); // 引用被解除
        }
    }
};

REGISTER_TEST(ConnectionServiceTest);

#include "test_connection_service.moc"
