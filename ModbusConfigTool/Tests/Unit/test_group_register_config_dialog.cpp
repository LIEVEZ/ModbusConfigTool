#include "Domain/Models/project_factory.h"
#include "Views/Groups/group_register_config_dialog.h"
#include "test_registry.h"

#include <QTableView>
#include <QTest>

class GroupRegisterConfigDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void refreshesRowsAfterDocumentChange()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        const QString groupId = document.groups.first().id;
        GroupRegisterConfigDialog dialog(groupId, document);
        auto *table = dialog.findChild<QTableView *>(QStringLiteral("groupRegisterTable"));
        QVERIFY(table);
        QCOMPARE(table->model()->rowCount(), 0);

        document.registers.append(ProjectFactory::createRegister(groupId, 1));
        dialog.setDocument(document);

        QCOMPARE(table->model()->rowCount(), 1);
    }
};

REGISTER_TEST(GroupRegisterConfigDialogTest);

#include "test_group_register_config_dialog.moc"
