#include "group_card_widget.h"

#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QString portDisplayName(const ConnectionPort &port)
{
    const QString type = port.profile.connectionType == ConnectionType::Tcp
        ? QStringLiteral("TCP") : QStringLiteral("RTU");
    return QStringLiteral("%1（%2）")
        .arg(port.name.isEmpty() ? QStringLiteral("默认端口") : port.name, type);
}

QString tooltipText(const RegisterGroup &group,
                    int registerCount,
                    const QList<RegisterPoint> &points)
{
    QStringList lines;
    lines.append(group.name);
    lines.append(group.description.isEmpty() ? QStringLiteral("无描述") : group.description);
    lines.append(QStringLiteral("%1 条寄存器").arg(registerCount));

    const int summaryCount = qMin(3, points.size());
    for (int index = 0; index < summaryCount; ++index)
    {
        const RegisterPoint &point = points.at(index);
        lines.append(QStringLiteral("%1：%2")
                         .arg(point.name,
                              point.currentValue.toDisplayString(point.precision)));
    }
    return lines.join(QLatin1Char('\n'));
}

QString runtimeTooltipText(const QString &name,
                           const QString &description,
                           int registerCount,
                           const QList<RegisterPoint> &points)
{
    RegisterGroup group;
    group.name = name;
    group.description = description;
    return tooltipText(group, registerCount, points);
}

void makeMouseTransparent(QWidget *widget)
{
    widget->setAttribute(Qt::WA_TransparentForMouseEvents);
}
}

GroupCardWidget::GroupCardWidget(const RegisterGroup &group,
                                 int registerCount,
                                 const QList<ConnectionPort> &ports,
                                 const QList<RegisterPoint> &points,
                                 QWidget *parent)
    : QWidget(parent),
      m_groupId(group.id),
      m_name(group.name),
      m_description(group.description),
      m_registerCount(registerCount),
      m_groupEnabled(group.enabled)
{
    setObjectName(QStringLiteral("groupCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("groupEnabled", m_groupEnabled);
    setProperty("selected", false);
    setFixedWidth(210);
    setMinimumHeight(154);
    setCursor(Qt::OpenHandCursor);
    setMouseTracking(true);
    setToolTip(tooltipText(group, registerCount, points));
    if (!m_groupEnabled)
    {
        auto *opacity = new QGraphicsOpacityEffect(this);
        opacity->setOpacity(0.62);
        setGraphicsEffect(opacity);
    }

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *colorBar = new QWidget(this);
    colorBar->setObjectName(QStringLiteral("groupColorBar"));
    colorBar->setFixedHeight(8);
    colorBar->setStyleSheet(QStringLiteral("background: %1;").arg(group.color));
    makeMouseTransparent(colorBar);
    layout->addWidget(colorBar);

    auto *content = new QWidget(this);
    content->setObjectName(QStringLiteral("groupCardContent"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(14, 8, 14, 12);
    contentLayout->setSpacing(7);
    layout->addWidget(content);

    auto *titleRow = new QHBoxLayout;
    titleRow->setSpacing(8);
    auto *nameLabel = new QLabel(group.name, this);
    nameLabel->setObjectName(QStringLiteral("groupName"));
    makeMouseTransparent(nameLabel);
    m_enabledButton = new QPushButton(
        m_groupEnabled ? QStringLiteral("启用") : QStringLiteral("已停用"), this);
    m_enabledButton->setObjectName(QStringLiteral("groupEnabledToggle"));
    m_enabledButton->setProperty("groupEnabled", m_groupEnabled);
    titleRow->addWidget(nameLabel, 1);
    titleRow->addWidget(m_enabledButton);
    contentLayout->addLayout(titleRow);

    auto *descriptionLabel = new QLabel(
        group.description.isEmpty() ? QStringLiteral("无描述") : group.description, this);
    descriptionLabel->setObjectName(QStringLiteral("groupDescription"));
    descriptionLabel->setWordWrap(true);
    makeMouseTransparent(descriptionLabel);
    contentLayout->addWidget(descriptionLabel);

    auto *countRow = new QHBoxLayout;
    countRow->setSpacing(6);
    m_countLabel = new QLabel(QString::number(registerCount), this);
    m_countLabel->setObjectName(QStringLiteral("groupRegisterCount"));
    auto *countUnit = new QLabel(QStringLiteral("寄存器"), this);
    countUnit->setObjectName(QStringLiteral("groupRegisterUnit"));
    makeMouseTransparent(m_countLabel);
    makeMouseTransparent(countUnit);
    countRow->addWidget(m_countLabel);
    countRow->addWidget(countUnit);
    countRow->addStretch();
    contentLayout->addLayout(countRow);

    auto *portLabel = new QLabel(QStringLiteral("通信端口"), this);
    portLabel->setObjectName(QStringLiteral("groupPortLabel"));
    makeMouseTransparent(portLabel);
    m_portCombo = new QComboBox(this);
    m_portCombo->setObjectName(QStringLiteral("groupPortCombo"));
    m_portCombo->addItem(QStringLiteral("未绑定端口"), QString());
    for (const ConnectionPort &port : ports)
    {
        m_portCombo->addItem(portDisplayName(port), port.id);
    }
    const int selectedPortIndex = m_portCombo->findData(group.portId);
    m_portCombo->setCurrentIndex(selectedPortIndex >= 0 ? selectedPortIndex : 0);
    m_portView = m_portCombo->view();
    m_portViewport = m_portView->viewport();
    m_enabledButton->installEventFilter(this);
    m_portCombo->installEventFilter(this);
    m_portView->installEventFilter(this);
    m_portViewport->installEventFilter(this);
    contentLayout->addWidget(portLabel);
    contentLayout->addWidget(m_portCombo);

    connect(m_enabledButton, &QPushButton::clicked, this, [this]()
    {
        emit enabledChangeRequested(m_groupId, !m_groupEnabled);
    });
    connect(m_portCombo,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int)
    {
        const QString portId = m_portCombo->currentData().toString();
        queuePortChange(portId);
    });
}

bool GroupCardWidget::eventFilter(QObject *watched, QEvent *event)
{
    const bool controlObject = watched == m_enabledButton
        || watched == m_portCombo
        || watched == m_portView
        || watched == m_portViewport;
    if (controlObject && event->type() == QEvent::MouseButtonPress)
    {
        m_controlInteraction = true;
        m_pressedForDrag = false;
        m_dragging = false;
    }
    else if ((watched == m_portView || watched == m_portViewport)
             && event->type() == QEvent::Hide)
    {
        m_controlInteraction = false;
        if (m_portChangePending)
        {
            QTimer::singleShot(0, this, &GroupCardWidget::emitPendingPortChange);
        }
    }
    else if (controlObject && event->type() == QEvent::MouseButtonRelease
             && !m_portView->isVisible())
    {
        QTimer::singleShot(0, this, [this]() { m_controlInteraction = false; });
    }
    return QWidget::eventFilter(watched, event);
}

void GroupCardWidget::setSelected(bool selected)
{
    if (m_selected == selected)
    {
        return;
    }
    m_selected = selected;
    setProperty("selected", selected);
    refreshStyle();
}

void GroupCardWidget::updateRegisterCount(int count)
{
    m_registerCount = count;
    if (m_countLabel)
    {
        m_countLabel->setText(QString::number(count));
    }
}

void GroupCardWidget::updateRuntimeSummary(const QList<RegisterPoint> &points)
{
    setToolTip(runtimeTooltipText(
        m_name, m_description, m_registerCount, points));
}

void GroupCardWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget *pressedWidget = QApplication::widgetAt(event->globalPos());
    if (event->button() == Qt::LeftButton
        && !m_controlInteraction
        && !isInteractiveChild(pressedWidget))
    {
        m_dragging = false;
        m_pressedForDrag = true;
        m_dragStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    m_dragging = false;
    m_pressedForDrag = false;
    QWidget::mousePressEvent(event);
}

void GroupCardWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_controlInteraction
        && m_pressedForDrag
        && (event->buttons() & Qt::LeftButton))
    {
        if (!m_dragging
            && (event->pos() - m_dragStartPos).manhattanLength()
                >= QApplication::startDragDistance())
        {
            m_dragging = true;
            emit dragStarted(m_groupId, m_dragStartPos);
        }
        if (m_dragging)
        {
            emit dragging(m_groupId, event->globalPos());
        }
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void GroupCardWidget::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    if (m_dragging)
    {
        m_dragging = false;
        m_pressedForDrag = false;
        emit dragFinished(m_groupId);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && m_pressedForDrag)
    {
        m_pressedForDrag = false;
        emit clicked(m_groupId);
        event->accept();
        return;
    }
    m_pressedForDrag = false;
    QWidget::mouseReleaseEvent(event);
}

void GroupCardWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton
        && !isInteractiveChild(childAt(event->pos())))
    {
        emit doubleClicked(m_groupId);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void GroupCardWidget::contextMenuEvent(QContextMenuEvent *event)
{
    emit contextMenuRequested(m_groupId, event->globalPos());
    event->accept();
}

void GroupCardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QStyleOption option;
    option.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

bool GroupCardWidget::isInteractiveChild(QWidget *widget) const
{
    if (!widget)
    {
        return false;
    }
    return widget == m_enabledButton
        || m_enabledButton->isAncestorOf(widget)
        || widget == m_portCombo
        || m_portCombo->isAncestorOf(widget);
}

void GroupCardWidget::queuePortChange(const QString &portId)
{
    m_pendingPortId = portId;
    if (m_portChangePending)
    {
        return;
    }

    m_portChangePending = true;
    if (!m_portView->isVisible())
    {
        QTimer::singleShot(0, this, &GroupCardWidget::emitPendingPortChange);
    }
}

void GroupCardWidget::emitPendingPortChange()
{
    if (!m_portChangePending || (m_portView && m_portView->isVisible()))
    {
        return;
    }

    m_portChangePending = false;
    emit portChangeRequested(m_groupId, m_pendingPortId);
}

void GroupCardWidget::refreshStyle()
{
    style()->unpolish(this);
    style()->polish(this);
    update();
}
