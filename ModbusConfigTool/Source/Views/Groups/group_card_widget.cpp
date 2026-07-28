#include "group_card_widget.h"

#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"
#include "Domain/Models/register_point.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFrame>

#include <QHelpEvent>
#include <QGraphicsDropShadowEffect>
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

void makeMouseTransparent(QWidget *widget)
{
    widget->setAttribute(Qt::WA_TransparentForMouseEvents);
}
}

class GroupHoverTip : public QFrame
{
public:
    explicit GroupHoverTip()
        : QFrame(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus)
    {
        setObjectName(QStringLiteral("groupHoverTip"));
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 10, 12, 10);
        layout->setSpacing(4);

        m_title = new QLabel(this);
        m_title->setObjectName(QStringLiteral("groupHoverTitle"));
        m_title->setWordWrap(true);

        m_description = new QLabel(this);
        m_description->setObjectName(QStringLiteral("groupHoverDescription"));
        m_description->setWordWrap(true);

        m_count = new QLabel(this);
        m_count->setObjectName(QStringLiteral("groupHoverCount"));

        m_runtime = new QLabel(this);
        m_runtime->setObjectName(QStringLiteral("groupHoverRuntime"));
        m_runtime->setWordWrap(true);

        layout->addWidget(m_title);
        layout->addWidget(m_description);
        layout->addWidget(m_count);
        layout->addWidget(m_runtime);

        setStyleSheet(QStringLiteral(
            "QFrame#groupHoverTip {"
            "  background: #26251e;"
            "  color: #f2f1ed;"
            "  border-radius: 8px;"
            "  border: 1px solid rgba(255,255,255,40);"
            "}"
            "QLabel { background: transparent; color: #f2f1ed; font: 12px \"Segoe UI\", \"Microsoft YaHei\"; }"
            "QLabel#groupHoverTitle { color: #ffb38a; font-weight: 600; }"
            "QLabel#groupHoverDescription, QLabel#groupHoverCount { color: rgba(242,241,237,200); }"
            "QLabel#groupHoverRuntime {"
            "  margin-top: 4px;"
            "  padding-top: 6px;"
            "  border-top: 1px solid rgba(255,255,255,38);"
            "  color: rgba(242,241,237,230);"
            "}"));
        hide();
    }

    void setContent(const QString &name,
                    const QString &description,
                    int registerCount,
                    const QList<RegisterPoint> &points)
    {
        m_title->setText(name);
        m_description->setText(description.isEmpty() ? QStringLiteral("无描述") : description);
        m_count->setText(QStringLiteral("%1 条寄存器").arg(registerCount));

        QStringList runtimeLines;
        const int summaryCount = qMin(3, points.size());
        for (int index = 0; index < summaryCount; ++index)
        {
            const RegisterPoint &point = points.at(index);
            runtimeLines.append(QStringLiteral("%1：%2")
                                    .arg(point.name,
                                         point.currentValue.toDisplayString(point.precision)));
        }
        if (runtimeLines.isEmpty())
        {
            m_runtime->setText(QStringLiteral("暂无实时值"));
        }
        else
        {
            m_runtime->setText(runtimeLines.join(QLatin1Char('\n')));
        }
        adjustSize();
        setFixedWidth(qBound(180, sizeHint().width() + 8, 280));
        adjustSize();
    }

    void moveNear(const QPoint &globalPos)
    {
        move(globalPos + QPoint(16, 18));
        if (!isVisible())
        {
            show();
        }
        raise();
    }

private:
    QLabel *m_title = nullptr;
    QLabel *m_description = nullptr;
    QLabel *m_count = nullptr;
    QLabel *m_runtime = nullptr;
};

GroupCardWidget::GroupCardWidget(const RegisterGroup &group,
                                 int registerCount,
                                 const QList<ConnectionPort> &ports,
                                 const QList<RegisterPoint> &points,
                                 bool portLive,
                                 QWidget *parent)
    : QWidget(parent),
      m_groupId(group.id),
      m_name(group.name),
      m_description(group.description),
      m_registerCount(registerCount),
      m_groupEnabled(group.enabled),
      m_portLive(portLive)
{
    setObjectName(QStringLiteral("groupCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("groupEnabled", m_groupEnabled);
    setProperty("selected", false);
    setFixedWidth(190);
    setMinimumHeight(154);
    setCursor(Qt::OpenHandCursor);
    setMouseTracking(true);
    if (!m_groupEnabled)
    {
        auto *opacity = new QGraphicsOpacityEffect(this);
        opacity->setOpacity(0.62);
        setGraphicsEffect(opacity);
    }

    m_hoverTip = new GroupHoverTip;
    rebuildHoverContent(points);

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
    contentLayout->addWidget(portLabel);

    auto *portRow = new QHBoxLayout;
    portRow->setSpacing(6);
    m_portLiveDot = new QLabel(this);
    m_portLiveDot->setObjectName(QStringLiteral("groupPortLiveDot"));
    m_portLiveDot->setFixedSize(10, 10);
    makeMouseTransparent(m_portLiveDot);
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
    portRow->addWidget(m_portLiveDot, 0, Qt::AlignVCenter);
    portRow->addWidget(m_portCombo, 1);
    contentLayout->addLayout(portRow);
    applyPortLiveStyle();

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

GroupCardWidget::~GroupCardWidget()
{
    hideHoverTip();
    delete m_hoverTip;
    m_hoverTip = nullptr;
}

bool GroupCardWidget::eventFilter(QObject *watched, QEvent *event)
{
    const bool controlObject = watched == m_enabledButton
        || watched == m_portCombo
        || watched == m_portView
        || watched == m_portViewport;
    if (controlObject && event->type() == QEvent::ContextMenu)
    {
        auto *contextEvent = static_cast<QContextMenuEvent *>(event);
        hideHoverTip();
        emit contextMenuRequested(m_groupId, contextEvent->globalPos());
        return true;
    }
    if (controlObject && event->type() == QEvent::MouseButtonPress)
    {
        m_controlInteraction = true;
        m_pressedForDrag = false;
        m_dragging = false;
        hideHoverTip();
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
    rebuildHoverContent(points);
    if (m_hovering && !m_dragging && !m_controlInteraction)
    {
        showHoverTip(QCursor::pos());
    }
}

void GroupCardWidget::rebuildHoverContent(const QList<RegisterPoint> &points)
{
    QStringList lines;
    lines.append(m_name);
    lines.append(m_description.isEmpty() ? QStringLiteral("无描述") : m_description);
    lines.append(QStringLiteral("%1 条寄存器").arg(m_registerCount));
    const int summaryCount = qMin(3, points.size());
    for (int index = 0; index < summaryCount; ++index)
    {
        const RegisterPoint &point = points.at(index);
        lines.append(QStringLiteral("%1：%2")
                         .arg(point.name,
                              point.currentValue.toDisplayString(point.precision)));
    }
    m_hoverSummaryText = lines.join(QLatin1Char('\n'));

    // 仅使用自定义深色悬停层，禁用系统原生 toolTip，避免双提示。
    setToolTip(QString());
    if (m_hoverTip)
    {
        m_hoverTip->setContent(m_name, m_description, m_registerCount, points);
    }
}

void GroupCardWidget::showHoverTip(const QPoint &globalPos)
{
    if (!m_hoverTip || m_dragging || m_controlInteraction)
    {
        return;
    }
    m_hoverTip->moveNear(globalPos);
}

void GroupCardWidget::hideHoverTip()
{
    if (m_hoverTip)
    {
        m_hoverTip->hide();
    }
}

void GroupCardWidget::enterEvent(QEvent *event)
{
    m_hovering = true;
    if (!m_dragging && !m_controlInteraction)
    {
        showHoverTip(QCursor::pos());
    }
    QWidget::enterEvent(event);
}

void GroupCardWidget::leaveEvent(QEvent *event)
{
    m_hovering = false;
    hideHoverTip();
    QWidget::leaveEvent(event);
}
bool GroupCardWidget::event(QEvent *event)
{
    // 吞掉系统 ToolTip 请求，只保留自定义 GroupHoverTip。
    if (event->type() == QEvent::ToolTip)
    {
        return true;
    }
    return QWidget::event(event);
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
            hideHoverTip();
            applyDraggingVisual(true);
            emit dragStarted(m_groupId, m_dragStartPos);
        }
        if (m_dragging)
        {
            emit dragging(m_groupId, event->globalPos());
        }
        event->accept();
        return;
    }

    if (m_hovering && !m_dragging && !m_controlInteraction
        && !(event->buttons() & Qt::LeftButton))
    {
        showHoverTip(event->globalPos());
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
        applyDraggingVisual(false);
        emit dragFinished(m_groupId);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && m_pressedForDrag)
    {
        m_pressedForDrag = false;
        emit clicked(m_groupId, event->modifiers());
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
        hideHoverTip();
        emit doubleClicked(m_groupId);
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void GroupCardWidget::contextMenuEvent(QContextMenuEvent *event)
{
    hideHoverTip();
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


QString GroupCardWidget::boundPortId() const
{
    return m_portCombo ? m_portCombo->currentData().toString() : QString();
}

void GroupCardWidget::setPortLive(bool live)
{
    if (m_portLive == live)
    {
        return;
    }
    m_portLive = live;
    applyPortLiveStyle();
}

void GroupCardWidget::applyPortLiveStyle()
{
    if (!m_portLiveDot)
    {
        return;
    }
    if (m_portLive)
    {
        m_portLiveDot->setStyleSheet(QStringLiteral(
            "background: #1f8a65; border-radius: 5px;"
            "border: 2px solid rgba(31,138,101,80);"));
        m_portLiveDot->setFixedSize(10, 10);
    }
    else
    {
        m_portLiveDot->setStyleSheet(QStringLiteral(
            "background: rgba(38,37,30,40); border-radius: 5px;"
            "border: 2px solid transparent;"));
        m_portLiveDot->setFixedSize(10, 10);
    }
}

void GroupCardWidget::applyDraggingVisual(bool dragging)
{
    setProperty("dragging", dragging);
    if (dragging)
    {
        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(28);
        shadow->setOffset(0, 10);
        shadow->setColor(QColor(38, 37, 30, 70));
        setGraphicsEffect(shadow);
        raise();
    }
    else
    {
        restoreIdleGraphicsEffect();
    }
    refreshStyle();
}

void GroupCardWidget::restoreIdleGraphicsEffect()
{
    if (!m_groupEnabled)
    {
        auto *opacity = new QGraphicsOpacityEffect(this);
        opacity->setOpacity(0.62);
        setGraphicsEffect(opacity);
    }
    else
    {
        setGraphicsEffect(nullptr);
    }
}

void GroupCardWidget::refreshStyle()
{
    style()->unpolish(this);
    style()->polish(this);
    update();
}

