#include "Application/Runtime/runtime_service.h"
#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"
#include "Views/Connection/connection_port_list_view.h"
#include "test_registry.h"

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

class ConnectionPortListViewTest : public QObject
{
    Q_OBJECT

private slots:
    void displaysBindingCountAndRunningState()
    {
        ConnectionPort port;
        port.id = QStringLiteral("port-1");
        port.name = QStringLiteral("现场 PLC");

        RegisterGroup firstGroup;
        firstGroup.id = QStringLiteral("group-1");
        firstGroup.portId = port.id;
        RegisterGroup secondGroup;
        secondGroup.id = QStringLiteral("group-2");
        secondGroup.portId = port.id;

        QHash<QString, RuntimeState> states;
        states.insert(port.id, RuntimeState::Running);

        ConnectionPortListView view;
        view.setModel({port}, {firstGroup, secondGroup}, states);

        auto *bindingLabel = view.findChild<QLabel *>(QStringLiteral("bindingCount_") + port.id);
        auto *toggleButton = view.findChild<QPushButton *>(QStringLiteral("togglePort_") + port.id);
        QVERIFY(bindingLabel);
        QVERIFY(toggleButton);
        QCOMPARE(bindingLabel->text(), QStringLiteral("绑定 2 分组"));
        QCOMPARE(toggleButton->text(), QStringLiteral("断开"));
    }

    void emitsStartOrStopFromRuntimeState()
    {
        ConnectionPort port;
        port.id = QStringLiteral("port-1");

        ConnectionPortListView view;
        QSignalSpy startSpy(&view, &ConnectionPortListView::startPortRequested);
        QSignalSpy stopSpy(&view, &ConnectionPortListView::stopPortRequested);

        view.setModel({port}, {}, {{port.id, RuntimeState::Running}});
        QTest::mouseClick(view.findChild<QPushButton *>(QStringLiteral("togglePort_") + port.id),
                          Qt::LeftButton);
        QCOMPARE(stopSpy.count(), 1);
        QCOMPARE(startSpy.count(), 0);

        view.setModel({port}, {}, {{port.id, RuntimeState::Idle}});
        QTest::mouseClick(view.findChild<QPushButton *>(QStringLiteral("togglePort_") + port.id),
                          Qt::LeftButton);
        QCOMPARE(startSpy.count(), 1);
    }


    void selectsPortCardOnClick()
    {
        ConnectionPort first;
        first.id = QStringLiteral("port-1");
        first.name = QStringLiteral("A");
        ConnectionPort second;
        second.id = QStringLiteral("port-2");
        second.name = QStringLiteral("B");

        ConnectionPortListView view;
        QSignalSpy selectSpy(&view, &ConnectionPortListView::portSelected);
        view.setModel({first, second}, {}, {});

        // setModel selects first by default
        QCOMPARE(view.selectedPortId(), first.id);

        QWidget *secondCard = view.findChild<QWidget *>(QStringLiteral("portCard_") + second.id);
        QVERIFY(secondCard);
        QTest::mouseClick(secondCard, Qt::LeftButton);
        QCOMPARE(view.selectedPortId(), second.id);
        QVERIFY(secondCard->property("selected").toBool());
        QVERIFY(selectSpy.count() >= 1);
    }

    void runtimeUpdateChangesToggleBehavior()
    {
        ConnectionPort port;
        port.id = QStringLiteral("port-1");

        ConnectionPortListView view;
        QSignalSpy startSpy(&view, &ConnectionPortListView::startPortRequested);
        QSignalSpy stopSpy(&view, &ConnectionPortListView::stopPortRequested);
        view.setModel({port}, {}, {{port.id, RuntimeState::Idle}});

        view.updatePortState(port.id, RuntimeState::Running);
        QTest::mouseClick(view.findChild<QPushButton *>(QStringLiteral("togglePort_") + port.id),
                          Qt::LeftButton);

        QCOMPARE(stopSpy.count(), 1);
        QCOMPARE(startSpy.count(), 0);
    }
};

REGISTER_TEST(ConnectionPortListViewTest);

#include "test_connection_port_list_view.moc"
