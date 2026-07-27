#include "Views/Registers/register_filter_proxy_model.h"
#include "Views/Registers/register_table_model.h"
#include "Views/RuntimeValues/runtime_value_table_model.h"
#include "Domain/Models/project_factory.h"
#include "test_registry.h"

#include <QTest>

class TableModelsTest : public QObject
{
    Q_OBJECT

private slots:
    void exposesFourteenConfigurationColumns()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.registers.append(ProjectFactory::createRegister(document.groups.first().id, 12));
        RegisterTableModel model;
        model.setDocument(&document);

        QCOMPARE(model.columnCount(), 14);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.headerData(8, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("协议键"));
    }

    void filtersByName()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        RegisterPoint point = ProjectFactory::createRegister(document.groups.first().id, 1);
        point.name = QStringLiteral("母线电压");
        document.registers.append(point);
        RegisterTableModel model;
        model.setDocument(&document);
        RegisterFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setSearchMode(RegisterFilterProxyModel::SearchMode::Name);
        proxy.setSearchText(QStringLiteral("电压"));

        QCOMPARE(proxy.rowCount(), 1);
        proxy.setSearchText(QStringLiteral("电流"));
        QCOMPARE(proxy.rowCount(), 0);
    }

    void exposesSevenRuntimeColumns()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.registers.append(ProjectFactory::createRegister(document.groups.first().id, 2));
        RuntimeValueTableModel model;
        model.setDocument(&document);

        QCOMPARE(model.columnCount(), 7);
        QCOMPARE(model.headerData(4, Qt::Horizontal, Qt::DisplayRole).toString(), QStringLiteral("当前值"));
    }
};

REGISTER_TEST(TableModelsTest);

#include "test_table_models.moc"

