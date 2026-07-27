#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"
#include "Domain/Models/project_factory.h"
#include "Views/Groups/group_canvas_view.h"
#include "Views/Groups/group_card_widget.h"
#include "test_registry.h"

#include <QComboBox>
#include <QApplication>
#include <QAbstractItemView>
#include <QFile>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalSpy>
#include <QTest>

class GroupCardWidgetTest : public QObject
{
    Q_OBJECT

private slots:
    void exposesEnabledAndPortControls()
    {
        RegisterGroup group;
        group.id = QStringLiteral("group-1");
        group.name = QStringLiteral("电能参数");
        group.enabled = false;
        group.portId = QStringLiteral("port-1");

        ConnectionPort port;
        port.id = group.portId;
        port.name = QStringLiteral("现场 PLC");

        GroupCardWidget card(group, 3, {port}, {}, false);
        auto *enabledButton =
            card.findChild<QPushButton *>(QStringLiteral("groupEnabledToggle"));
        auto *portCombo = card.findChild<QComboBox *>(QStringLiteral("groupPortCombo"));

        QVERIFY(enabledButton);
        QVERIFY(portCombo);
        QVERIFY(card.testAttribute(Qt::WA_StyledBackground));
        QCOMPARE(card.property("groupEnabled").toBool(), false);
        QCOMPARE(enabledButton->text(), QStringLiteral("已停用"));
        QCOMPARE(portCombo->currentData().toString(), port.id);
    }

    void emitsEnabledAndPortChanges()
    {
        RegisterGroup group;
        group.id = QStringLiteral("group-1");
        group.enabled = false;

        ConnectionPort port;
        port.id = QStringLiteral("port-1");
        port.name = QStringLiteral("现场 PLC");

        GroupCardWidget card(group, 0, {port}, {}, false);
        QSignalSpy enabledSpy(&card, &GroupCardWidget::enabledChangeRequested);
        QSignalSpy portSpy(&card, &GroupCardWidget::portChangeRequested);

        QTest::mouseClick(
            card.findChild<QPushButton *>(QStringLiteral("groupEnabledToggle")),
            Qt::LeftButton);
        QCOMPARE(enabledSpy.count(), 1);
        QCOMPARE(enabledSpy.first().at(0).toString(), group.id);
        QCOMPARE(enabledSpy.first().at(1).toBool(), true);

        card.show();
        auto *portCombo = card.findChild<QComboBox *>(QStringLiteral("groupPortCombo"));
        portCombo->showPopup();
        QTest::qWait(20);
        portCombo->setCurrentIndex(portCombo->findData(port.id));
        portCombo->hidePopup();
        // 端口变更在下拉关闭后（或控件完成选择后）提交
        QTRY_COMPARE(portSpy.count(), 1);
        QCOMPARE(portSpy.first().at(0).toString(), group.id);
        QCOMPARE(portSpy.first().at(1).toString(), port.id);
    }

    void controlsDoNotStartCardDragging()
    {
        RegisterGroup group;
        group.id = QStringLiteral("group-1");
        ConnectionPort port;
        port.id = QStringLiteral("port-1");
        port.name = QStringLiteral("现场 PLC");

        GroupCardWidget card(group, 0, {port}, {}, false);
        card.show();
        QSignalSpy dragSpy(&card, &GroupCardWidget::dragStarted);
        auto *portCombo = card.findChild<QComboBox *>(QStringLiteral("groupPortCombo"));
        auto *enabledButton =
            card.findChild<QPushButton *>(QStringLiteral("groupEnabledToggle"));

        QTest::mousePress(portCombo, Qt::LeftButton, Qt::NoModifier, portCombo->rect().center());
        QTest::mouseMove(portCombo, QPoint(portCombo->width() - 2, portCombo->height() - 2));
        QTest::mouseRelease(portCombo, Qt::LeftButton,
                            Qt::NoModifier, portCombo->rect().center());
        QCOMPARE(dragSpy.count(), 0);

        QTest::mouseClick(enabledButton, Qt::LeftButton);
        QCOMPARE(dragSpy.count(), 0);

        QTest::mouseClick(&card, Qt::LeftButton, Qt::NoModifier, QPoint(20, 70));
        QMouseEvent orphanMove(QEvent::MouseMove,
                               QPointF(90, 110),
                               card.mapToGlobal(QPoint(90, 110)),
                               Qt::NoButton,
                               Qt::LeftButton,
                               Qt::NoModifier);
        QApplication::sendEvent(&card, &orphanMove);
        QCOMPARE(dragSpy.count(), 0);

        QTest::mousePress(&card, Qt::LeftButton, Qt::NoModifier, QPoint(20, 70));
        portCombo->showPopup();
        QTest::qWait(20);
        QTest::mousePress(portCombo->view()->viewport(),
                          Qt::LeftButton,
                          Qt::NoModifier,
                          portCombo->view()->visualRect(
                              portCombo->model()->index(1, 0)).center());
        QApplication::sendEvent(&card, &orphanMove);
        QCOMPARE(dragSpy.count(), 0);
        portCombo->hidePopup();
    }

    void portPopupUsesComboGlobalPosition()
    {
        QString styleSheet;
        for (const QString &path : {QStringLiteral(":/styles/base.qss"),
                                    QStringLiteral(":/styles/controls.qss")})
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
            styleSheet += QString::fromUtf8(file.readAll());
        }
        const QString previousStyleSheet = qApp->styleSheet();
        qApp->setStyleSheet(styleSheet);

        ConnectionPort port;
        port.id = QStringLiteral("port-1");
        port.name = QStringLiteral("现场 PLC");

        ProjectDocument document = ProjectFactory::createEmpty();
        document.ports = {port};
        document.groups.first().canvasX = 170;
        document.groups.first().canvasY = 40;

        QScrollArea host;
        host.resize(800, 600);
        host.setWidgetResizable(true);
        auto *canvas = new GroupCanvasView;
        canvas->setModel(document);
        host.setWidget(canvas);
        host.show();
        QTest::qWait(20);

        auto *portCombo = canvas->findChild<QComboBox *>(QStringLiteral("groupPortCombo"));
        portCombo->showPopup();
        QTest::qWait(20);

        QWidget *popup = nullptr;
        for (QWidget *topLevel : QApplication::topLevelWidgets())
        {
            if (topLevel->inherits("QComboBoxPrivateContainer"))
            {
                popup = topLevel;
                break;
            }
        }
        QVERIFY(popup);
        QVERIFY(qAbs(popup->x() - portCombo->mapToGlobal(QPoint()).x()) <= 2);
        portCombo->hidePopup();
        qApp->setStyleSheet(previousStyleSheet);
    }
};

REGISTER_TEST(GroupCardWidgetTest);

#include "test_group_card_widget.moc"

