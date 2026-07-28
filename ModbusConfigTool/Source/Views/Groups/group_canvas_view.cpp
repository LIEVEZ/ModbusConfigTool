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

void GroupCanvasView::setModel(const ProjectDocument &doc,
                               const QHash<QString, RuntimeState> &portStates)
{
    const QSet<QString> previousSelected = m_selectedIds;

    qDeleteAll(m_cards);
    m_cards.clear();
    m_portStates = portStates;
    m_draggedCardId.clear();
    m_dragStartPositions.clear();

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
            group, groupPoints.size(), doc.ports, groupPoints,
            isPortLive(group.portId), this);
        card->adjustSize();

        card->move(group.canvasX, group.canvasY);
        card->show();
        card->raise();
        m_cards.insert(group.id, card);

        connect(card, &GroupCardWidget::dragStarted, this, &GroupCanvasView::onCardDragStarted);
        connect(card, &GroupCardWidget::dragging, this, &GroupCanvasView::onCardDragging);
        connect(card, &GroupCardWidget::dragFinished, this, &GroupCanvasView::onCardDragFinished);
        connect(card, &GroupCardWidget::clicked, this, &GroupCanvasView::onCardClicked);
        connect(card, &GroupCardWidget::enabledChangeRequested,
                this, &GroupCanvasView::groupEnabledChangeRequested);
        connect(card, &GroupCardWidget::portChangeRequested,
                this, &GroupCanvasView::groupPortChangeRequested);
        connect(card, &GroupCardWidget::doubleClicked, this, &GroupCanvasView::groupDoubleClicked);
        connect(card, &GroupCardWidget::contextMenuRequested, this, &GroupCanvasView::groupContextMenuRequested);
    }

    // 刷新后尽量保留多选（仍存在的分组）
    m_selectedIds.clear();
    for (const QString &groupId : previousSelected)
    {
        if (m_cards.contains(groupId))
        {
            m_selectedIds.insert(groupId);
        }
    }
    applySelectionVisuals();
    updateCanvasExtent();
}

void GroupCanvasView::updateRuntimeValue(const ProjectDocument &doc,
                                         const QString &pointId)
{
    if (pointId.isEmpty())
    {
        Q_UNUSED(doc);
        return;
    }

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
    m_selectedIds.clear();
    if (!groupId.isEmpty() && m_cards.contains(groupId))
    {
        m_selectedIds.insert(groupId);
    }
    applySelectionVisuals();
}

QStringList GroupCanvasView::selectedGroupIds() const
{
    return m_selectedIds.values();
}

bool GroupCanvasView::isGroupSelected(const QString &groupId) const
{
    return m_selectedIds.contains(groupId);
}

void GroupCanvasView::applySelectionVisuals()
{
    for (auto it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        if (it.value())
        {
            it.value()->setSelected(m_selectedIds.contains(it.key()));
        }
    }
}

void GroupCanvasView::onCardClicked(const QString &groupId, Qt::KeyboardModifiers modifiers)
{
    if (!m_cards.contains(groupId))
    {
        return;
    }

    if (modifiers.testFlag(Qt::ControlModifier))
    {
        if (m_selectedIds.contains(groupId))
        {
            m_selectedIds.remove(groupId);
        }
        else
        {
            m_selectedIds.insert(groupId);
        }
    }
    else
    {
        m_selectedIds.clear();
        m_selectedIds.insert(groupId);
    }

    applySelectionVisuals();
    emit groupSelected(groupId);
}

void GroupCanvasView::paintEvent(QPaintEvent *)
{
    QPainter p(this);

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
            m_selectedIds.clear();
            applySelectionVisuals();
            emit canvasClicked();
        }
    }
    QWidget::mousePressEvent(event);
}

void GroupCanvasView::onCardDragStarted(const QString &groupId, const QPoint &offset)
{
    if (!m_cards.contains(groupId))
    {
        return;
    }

    m_draggedCardId = groupId;
    m_dragOffset = offset;
    m_dragStartPositions.clear();

    // 拖的不在选中集合里：改为单选该卡
    if (!m_selectedIds.contains(groupId))
    {
        m_selectedIds.clear();
        m_selectedIds.insert(groupId);
        applySelectionVisuals();
    }

    for (const QString &selectedId : m_selectedIds)
    {
        GroupCardWidget *card = m_cards.value(selectedId, nullptr);
        if (!card)
        {
            continue;
        }
        m_dragStartPositions.insert(selectedId, card->pos());
        card->raise();
    }
}

void GroupCanvasView::onCardDragging(const QString &groupId, const QPoint &globalPos)
{
    if (groupId != m_draggedCardId || !m_cards.contains(groupId))
    {
        return;
    }
    if (!m_dragStartPositions.contains(groupId))
    {
        return;
    }

    const QPoint canvasPos = mapFromGlobal(globalPos);
    const QPoint rawPrimary = canvasPos - m_dragOffset;
    QPoint delta = rawPrimary - m_dragStartPositions.value(groupId);

    // 整体平移，避免任一张卡越出左/上边界时打散相对位置
    int minX = 0;
    int minY = 0;
    bool first = true;
    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it)
    {
        const int x = it.value().x() + delta.x();
        const int y = it.value().y() + delta.y();
        if (first)
        {
            minX = x;
            minY = y;
            first = false;
        }
        else
        {
            minX = qMin(minX, x);
            minY = qMin(minY, y);
        }
    }
    if (minX < 0)
    {
        delta.rx() -= minX;
    }
    if (minY < 0)
    {
        delta.ry() -= minY;
    }

    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it)
    {
        GroupCardWidget *card = m_cards.value(it.key(), nullptr);
        if (!card)
        {
            continue;
        }
        card->move(it.value() + delta);
    }
    updateCanvasExtent();
}

void GroupCanvasView::onCardDragFinished(const QString &groupId)
{
    if (groupId != m_draggedCardId)
    {
        return;
    }

    for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it)
    {
        GroupCardWidget *card = m_cards.value(it.key(), nullptr);
        if (!card)
        {
            continue;
        }
        const QPoint pos = card->pos();
        if (pos != it.value())
        {
            emit groupMoved(it.key(), pos.x(), pos.y());
        }
    }

    m_draggedCardId.clear();
    m_dragStartPositions.clear();
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

void GroupCanvasView::updatePortStates(const QHash<QString, RuntimeState> &portStates)
{
    m_portStates = portStates;
    for (auto it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        GroupCardWidget *card = it.value();
        if (!card)
        {
            continue;
        }
        card->setPortLive(isPortLive(card->boundPortId()));
    }
}

bool GroupCanvasView::isPortLive(const QString &portId) const
{
    if (portId.isEmpty())
    {
        return false;
    }
    return m_portStates.value(portId, RuntimeState::Idle) == RuntimeState::Running;
}
