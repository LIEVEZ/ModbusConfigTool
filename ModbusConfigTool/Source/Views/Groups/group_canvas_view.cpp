#include "group_canvas_view.h"

#include "group_card_widget.h"
#include "Domain/Models/project_document.h"

#include <QMouseEvent>
#include <QPainter>

GroupCanvasView::GroupCanvasView(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(800, 600);
    setMouseTracking(true);
}

void GroupCanvasView::setModel(const ProjectDocument &doc)
{
    // Remove old cards
    qDeleteAll(m_cards);
    m_cards.clear();

    for (const RegisterGroup &group : doc.groups)
    {
        QList<RegisterPoint> groupPoints;
        for (const RegisterPoint &point : doc.registers)
        {
            if (point.groupId == group.id)
            {
                groupPoints.append(point);
            }
        }
        auto *card = new GroupCardWidget(
            group, groupPoints.size(), doc.ports, groupPoints, this);
        card->adjustSize();

        card->move(group.canvasX, group.canvasY);
        card->show();
        card->raise();
        m_cards.insert(group.id, card);

        connect(card, &GroupCardWidget::dragStarted, this, &GroupCanvasView::onCardDragStarted);
        connect(card, &GroupCardWidget::dragging, this, &GroupCanvasView::onCardDragging);
        connect(card, &GroupCardWidget::dragFinished, this, &GroupCanvasView::onCardDragFinished);
        connect(card, &GroupCardWidget::clicked, this, [this](const QString &groupId)
        {
            setSelectedGroup(groupId);
            emit groupSelected(groupId);
        });
        connect(card, &GroupCardWidget::enabledChangeRequested,
                this, &GroupCanvasView::groupEnabledChangeRequested);
        connect(card, &GroupCardWidget::portChangeRequested,
                this, &GroupCanvasView::groupPortChangeRequested);
        connect(card, &GroupCardWidget::doubleClicked, this, &GroupCanvasView::groupDoubleClicked);
        connect(card, &GroupCardWidget::contextMenuRequested, this, &GroupCanvasView::groupContextMenuRequested);
    }
    updateCanvasExtent();
}

void GroupCanvasView::updateRuntimeValue(const ProjectDocument &doc,
                                         const QString &pointId)
{
    QString groupId;
    for (const RegisterPoint &point : doc.registers)
    {
        if (point.id == pointId)
        {
            groupId = point.groupId;
            break;
        }
    }
    GroupCardWidget *card = m_cards.value(groupId, nullptr);
    if (!card)
    {
        return;
    }

    QList<RegisterPoint> groupPoints;
    for (const RegisterPoint &point : doc.registers)
    {
        if (point.groupId == groupId)
        {
            groupPoints.append(point);
        }
    }
    card->updateRuntimeSummary(groupPoints);
}

void GroupCanvasView::setSelectedGroup(const QString &groupId)
{
    for (auto it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        it.value()->setSelected(it.key() == groupId);
    }
}

void GroupCanvasView::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    // Grid background
    p.fillRect(rect(), QColor(236, 235, 230));
    p.setPen(QPen(QColor(38, 37, 30, 10), 1));
    const int gridSize = 26;
    for (int x = 0; x < width(); x += gridSize)
    {
        p.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += gridSize)
    {
        p.drawLine(0, y, width(), y);
    }

    // Hint text if empty
    if (m_cards.isEmpty())
    {
        p.setPen(QColor(38, 37, 30, 90));
        QFont font = p.font();
        font.setPixelSize(14);
        p.setFont(font);
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("画布空白 — 点击工具栏「＋ 新增分组」创建卡片"));
    }
}

void GroupCanvasView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // Check if clicked on empty canvas
        bool hitCard = false;
        for (GroupCardWidget *card : m_cards)
        {
            if (card->geometry().contains(event->pos()))
            {
                hitCard = true;
                break;
            }
        }
        if (!hitCard)
        {
            setSelectedGroup(QString());
            emit canvasClicked();
        }
    }
    QWidget::mousePressEvent(event);
}

void GroupCanvasView::onCardDragStarted(const QString &groupId, const QPoint &offset)
{
    m_draggedCardId = groupId;
    m_dragOffset = offset;
    if (m_cards.contains(groupId))
    {
        m_cards[groupId]->raise();
    }
}

void GroupCanvasView::onCardDragging(const QString &groupId, const QPoint &globalPos)
{
    if (groupId != m_draggedCardId || !m_cards.contains(groupId)) return;

    const QPoint canvasPos = mapFromGlobal(globalPos);
    const QPoint newPos = canvasPos - m_dragOffset;
    const int x = qMax(0, newPos.x());
    const int y = qMax(0, newPos.y());
    m_cards[groupId]->move(x, y);
    updateCanvasExtent();
}

void GroupCanvasView::onCardDragFinished(const QString &groupId)
{
    if (groupId != m_draggedCardId || !m_cards.contains(groupId)) return;

    const QPoint pos = m_cards[groupId]->pos();
    emit groupMoved(groupId, pos.x(), pos.y());
    m_draggedCardId.clear();
}

void GroupCanvasView::updateCanvasExtent()
{
    int requiredWidth = 800;
    int requiredHeight = 600;
    for (GroupCardWidget *card : m_cards)
    {
        requiredWidth = qMax(requiredWidth, card->x() + card->width() + 80);
        requiredHeight = qMax(requiredHeight, card->y() + card->height() + 80);
    }
    setMinimumSize(requiredWidth, requiredHeight);
    updateGeometry();
}
