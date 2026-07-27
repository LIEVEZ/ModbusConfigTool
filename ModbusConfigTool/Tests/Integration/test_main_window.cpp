#include "Views/Connection/connection_port_list_view.h"
#include "Views/Groups/group_canvas_view.h"
#include "Views/Groups/group_card_widget.h"
#include "Views/Logging/event_log_view.h"
#include "Views/Main/main_window.h"
#include "test_registry.h"

#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QScrollArea>
#include <QSplitter>
#include <QTest>
#include <QToolBar>
#include <QToolButton>

class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void containsGroupCanvasWorkspace()
    {
        MainWindow window;
        window.show();
        QTest::qWait(20);

        auto *splitter = window.findChild<QSplitter *>(QStringLiteral("workspaceSplitter"));
        QVERIFY(splitter);
        QCOMPARE(splitter->count(), 3);
        QVERIFY(window.findChild<ConnectionPortListView *>());
        QVERIFY(window.findChild<GroupCanvasView *>());
        QVERIFY(window.findChild<EventLogView *>());
        QVERIFY(window.findChild<QToolBar *>(QStringLiteral("workspaceToolBar")));
        QVERIFY(window.findChild<QLabel *>(QStringLiteral("groupCountBadge")));
        QVERIFY(window.findChild<QLabel *>(QStringLiteral("workspaceTitle")));
        QVERIFY(window.findChild<QLabel *>(QStringLiteral("workspaceHintBadge")));
        QVERIFY(window.findChild<QScrollArea *>(QStringLiteral("groupCanvasScrollArea")));
        QVERIFY(window.width() >= 1200);
    }

    void placesGroupActionsAtToolbarLeft()
    {
        MainWindow window;
        window.show();
        QTest::qWait(20);

        auto *title = window.findChild<QLabel *>(QStringLiteral("workspaceTitle"));
        auto *groupCount =
            window.findChild<QLabel *>(QStringLiteral("groupCountBadge"));
        auto *hint = window.findChild<QLabel *>(QStringLiteral("workspaceHintBadge"));
        auto *addButton =
            window.findChild<QToolButton *>(QStringLiteral("addGroupToolButton"));
        QVERIFY(title);
        QVERIFY(groupCount);
        QVERIFY(hint);
        QVERIFY(addButton);
        // 左侧：标题 -> 新增 -> 数量；右侧：操作提示
        QVERIFY(title->mapToGlobal(QPoint()).x()
                < addButton->mapToGlobal(QPoint()).x());
        QVERIFY(addButton->mapToGlobal(QPoint()).x()
                < groupCount->mapToGlobal(QPoint()).x());
        QVERIFY(groupCount->mapToGlobal(QPoint()).x()
                < hint->mapToGlobal(QPoint()).x());
        QCOMPARE(groupCount->height(), addButton->height());
        QVERIFY(qAbs(groupCount->mapToGlobal(QPoint()).y()
                     - addButton->mapToGlobal(QPoint()).y()) <= 1);
    }


    void containsConnectionAndHelpMenus()
    {
        MainWindow window;
        window.show();
        QTest::qWait(20);

        auto *connectionMenu = window.findChild<QMenu *>(QStringLiteral("connectionConfigMenu"));
        auto *helpMenu = window.findChild<QMenu *>(QStringLiteral("helpMenu"));
        auto *addPortAction = window.findChild<QAction *>(QStringLiteral("addPortMenuAction"));
        auto *managePortsAction = window.findChild<QAction *>(QStringLiteral("managePortsMenuAction"));
        auto *aboutAction = window.findChild<QAction *>(QStringLiteral("aboutAction"));
        QVERIFY(connectionMenu);
        QVERIFY(helpMenu);
        QVERIFY(addPortAction);
        QVERIFY(managePortsAction);
        QVERIFY(aboutAction);
    }

    void cardIsNotDeletedInsideItsClickHandler()
    {
        MainWindow window;
        window.show();
        QTest::qWait(20);

        QPointer<GroupCardWidget> card = window.findChild<GroupCardWidget *>();
        QVERIFY(card);
        auto *enabledButton =
            card->findChild<QPushButton *>(QStringLiteral("groupEnabledToggle"));
        QVERIFY(enabledButton);

        QTest::mouseClick(enabledButton, Qt::LeftButton);
        QVERIFY(card);
        QTRY_VERIFY(card.isNull());
    }
};

REGISTER_TEST(MainWindowTest);

#include "test_main_window.moc"
