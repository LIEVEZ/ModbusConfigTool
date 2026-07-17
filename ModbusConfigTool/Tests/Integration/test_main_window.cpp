#include "Views/Main/main_window.h"
#include "Views/Groups/group_panel_view.h"
#include "Views/Registers/register_config_view.h"
#include "Views/RuntimeControl/runtime_control_view.h"
#include "Views/RuntimeValues/runtime_value_view.h"
#include "test_registry.h"

#include <QTest>

class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void containsIndependentWorkspaceViews()
    {
        MainWindow window;
        window.show();
        QTest::qWait(20);

        QVERIFY(window.findChild<RuntimeControlView *>());
        QVERIFY(window.findChild<GroupPanelView *>());
        QVERIFY(window.findChild<RegisterConfigView *>());
        QVERIFY(window.findChild<RuntimeValueView *>());
        QVERIFY(window.width() >= 1100);
    }
};

REGISTER_TEST(MainWindowTest);

#include "test_main_window.moc"
