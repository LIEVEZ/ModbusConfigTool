#include "Domain/Models/project_factory.h"
#include "Views/Groups/group_canvas_view.h"
#include "Views/Groups/group_card_widget.h"
#include "test_registry.h"

#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

class GroupCanvasViewTest : public QObject
{
    Q_OBJECT

private slots:
    void expandsForRemoteCards()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.groups.first().canvasX = 1200;
        document.groups.first().canvasY = 700;

        GroupCanvasView canvas;
        canvas.setModel(document);

        QVERIFY(canvas.minimumWidth() >= 1470);
        QVERIFY(canvas.minimumHeight() >= 934);
    }

    void forwardsCardEnabledRequest()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        document.groups.first().enabled = false;

        GroupCanvasView canvas;
        QSignalSpy enabledSpy(&canvas, &GroupCanvasView::groupEnabledChangeRequested);
        canvas.setModel(document);

        auto *card = canvas.findChild<GroupCardWidget *>();
        QVERIFY(card);
        QTest::mouseClick(
            card->findChild<QPushButton *>(QStringLiteral("groupEnabledToggle")),
            Qt::LeftButton);
        QCOMPARE(enabledSpy.count(), 1);
        QCOMPARE(enabledSpy.first().at(0).toString(), document.groups.first().id);
        QCOMPARE(enabledSpy.first().at(1).toBool(), true);
    }

    void selectsCardOnPlainClick()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        GroupCanvasView canvas;
        QSignalSpy selectedSpy(&canvas, &GroupCanvasView::groupSelected);
        canvas.setModel(document);

        auto *card = canvas.findChild<GroupCardWidget *>();
        QVERIFY(card);
        QTest::mouseClick(card, Qt::LeftButton, Qt::NoModifier, QPoint(20, 70));

        QCOMPARE(selectedSpy.count(), 1);
        QCOMPARE(selectedSpy.first().at(0).toString(), document.groups.first().id);
        QCOMPARE(card->property("selected").toBool(), true);

        QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, QPoint(700, 500));
        QCOMPARE(card->property("selected").toBool(), false);
    }

    void refreshesTooltipForRuntimeValue()
    {
        ProjectDocument document = ProjectFactory::createEmpty();
        RegisterPoint point = ProjectFactory::createRegister(
            document.groups.first().id, 0);
        point.name = QStringLiteral("电压");
        point.currentValue = RegisterValue::fromUnsigned64(12, DataType::UInt16);
        document.registers.append(point);

        GroupCanvasView canvas;
        canvas.setModel(document);
        auto *card = canvas.findChild<GroupCardWidget *>();
        QVERIFY(card->hoverSummaryText().contains(QStringLiteral("12")));

        document.registers.first().currentValue =
            RegisterValue::fromUnsigned64(34, DataType::UInt16);
        canvas.updateRuntimeValue(document, point.id);

        QVERIFY(card->hoverSummaryText().contains(QStringLiteral("34")));
    }
};

REGISTER_TEST(GroupCanvasViewTest);

#include "test_group_canvas_view.moc"
