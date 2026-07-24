#include "connection_port_list_view.h"

#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QString stateColor(RuntimeState state)
{
    switch (state)
    {
    case RuntimeState::Running:
        return QStringLiteral("#1f8a65");
    case RuntimeState::Starting:
    case RuntimeState::Stopping:
        return QStringLiteral("#c08532");
    case RuntimeState::Fault:
        return QStringLiteral("#cf2d56");
    case RuntimeState::Idle:
    default:
        return QStringLiteral("#9a9992");
    }
}

void applyStateDot(QLabel *dot, RuntimeState state)
{
    if (!dot)
    {
        return;
    }
    const QString color = stateColor(state);
    if (state == RuntimeState::Running)
    {
        dot->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background: %1;"
            "  border-radius: 7px;"
            "  border: 2px solid rgba(31,138,101,70);"
            "}").arg(color));
        dot->setFixedSize(14, 14);
        return;
    }
    if (state == RuntimeState::Fault)
    {
        dot->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background: %1;"
            "  border-radius: 7px;"
            "  border: 2px solid rgba(207,45,86,70);"
            "}").arg(color));
        dot->setFixedSize(14, 14);
        return;
    }
    if (state == RuntimeState::Starting || state == RuntimeState::Stopping)
    {
        dot->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background: %1;"
            "  border-radius: 7px;"
            "  border: 2px solid rgba(192,133,50,70);"
            "}").arg(color));
        dot->setFixedSize(14, 14);
        return;
    }

    dot->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background: %1;"
        "  border-radius: 7px;"
        "  border: 2px solid transparent;"
        "}").arg(color));
    dot->setFixedSize(14, 14);
}

void applyToggleStyle(QPushButton *button, RuntimeState state)
{
    if (!button)
    {
        return;
    }

    button->setProperty("runtimeState", static_cast<int>(state));
    button->setProperty("connAction",
                        state == RuntimeState::Running
                            ? QStringLiteral("disconnect")
                            : QStringLiteral("connect"));
    button->setText(state == RuntimeState::Running
                        ? QStringLiteral("断开")
                        : QStringLiteral("连接"));
    button->setEnabled(state != RuntimeState::Starting
                       && state != RuntimeState::Stopping);

    if (state == RuntimeState::Running)
    {
        button->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #cf2d56;"
            "  background: #f7f7f4;"
            "  border: 1px solid rgba(207,45,86,110);"
            "  border-radius: 7px;"
            "  padding: 6px 10px;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(207,45,86,20);"
            "  border-color: #cf2d56;"
            "}"
            "QPushButton:disabled {"
            "  color: #999892;"
            "  background: #e6e5e0;"
            "  border-color: rgba(38,37,30,30);"
            "}"));
    }
    else if (state == RuntimeState::Fault)
    {
        button->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #c08532;"
            "  background: #f7f7f4;"
            "  border: 1px solid rgba(192,133,50,120);"
            "  border-radius: 7px;"
            "  padding: 6px 10px;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(192,133,50,24);"
            "  border-color: #c08532;"
            "}"
            "QPushButton:disabled {"
            "  color: #999892;"
            "  background: #e6e5e0;"
            "  border-color: rgba(38,37,30,30);"
            "}"));
    }
    else
    {
        button->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  color: #1f8a65;"
            "  background: #f7f7f4;"
            "  border: 1px solid rgba(31,138,101,110);"
            "  border-radius: 7px;"
            "  padding: 6px 10px;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(31,138,101,20);"
            "  border-color: #1f8a65;"
            "}"
            "QPushButton:disabled {"
            "  color: #999892;"
            "  background: #e6e5e0;"
            "  border-color: rgba(38,37,30,30);"
            "}"));
    }

    if (button->style())
    {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
}

QString connectionSummary(const ConnectionPort &port)
{
    if (port.profile.connectionType == ConnectionType::Tcp)
    {
        return QStringLiteral("%1:%2")
            .arg(port.profile.tcpHost)
            .arg(port.profile.tcpPort);
    }

    return QStringLiteral("%1 · %2 %3%4%5")
        .arg(port.profile.serialPort)
        .arg(port.profile.baudRate)
        .arg(port.profile.dataBits)
        .arg(port.profile.parity)
        .arg(port.profile.stopBits);
}

int countBindings(const QList<RegisterGroup> &groups, const QString &portId)
{
    int count = 0;
    for (const RegisterGroup &group : groups)
    {
        if (group.portId == portId)
        {
            ++count;
        }
    }
    return count;
}
}

ConnectionPortListView::ConnectionPortListView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("connectionPortList"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("portListScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->viewport()->setAutoFillBackground(true);
    scrollArea->viewport()->setStyleSheet(QStringLiteral("background: #f7f7f4;"));
    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("portListContent"));
    m_itemLayout = new QVBoxLayout(scrollContent);
    m_itemLayout->setContentsMargins(10, 10, 10, 10);
    m_itemLayout->setSpacing(12);
    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(scrollArea, 1);
}

void ConnectionPortListView::setModel(const QList<ConnectionPort> &ports,
                                      const QList<RegisterGroup> &groups,
                                      const QHash<QString, RuntimeState> &states)
{
    const QString previousSelected = m_selectedPortId;
    while (QLayoutItem *item = m_itemLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
    m_portIds.clear();

    if (ports.isEmpty())
    {
        m_selectedPortId.clear();
        auto *emptyLabel = new QLabel(
            QStringLiteral("暂无连接端口\n点击工具栏“＋ 端口”创建"), this);
        emptyLabel->setObjectName(QStringLiteral("emptyPortHint"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_itemLayout->addWidget(emptyLabel);
        m_itemLayout->addStretch();
        return;
    }

    for (const ConnectionPort &port : ports)
    {
        m_portIds.append(port.id);
        const RuntimeState state = states.value(port.id, RuntimeState::Idle);
        auto *card = new QWidget(this);
        card->setObjectName(QStringLiteral("portCard_") + port.id);
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setCursor(Qt::PointingHandCursor);
        card->setProperty("portCard", true);
        card->setProperty("portId", port.id);
        card->setProperty("selected", false);
        card->setProperty("runtimeState", static_cast<int>(state));
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(7);

        auto *titleRow = new QHBoxLayout;
        titleRow->setSpacing(8);
        auto *dot = new QLabel(card);
        dot->setObjectName(QStringLiteral("portState_") + port.id);
        applyStateDot(dot, state);
        auto *nameLabel = new QLabel(
            port.name.isEmpty() ? QStringLiteral("默认端口") : port.name, card);
        nameLabel->setObjectName(QStringLiteral("portName"));
        nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        nameLabel->setMinimumWidth(0);
        nameLabel->setWordWrap(false);
        auto *typeLabel = new QLabel(
            port.profile.connectionType == ConnectionType::Tcp
                ? QStringLiteral("TCP") : QStringLiteral("RTU"), card);
        typeLabel->setObjectName(QStringLiteral("portTypeBadge"));
        typeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        typeLabel->setAlignment(Qt::AlignCenter);
        titleRow->addWidget(dot, 0, Qt::AlignVCenter);
        titleRow->addWidget(nameLabel, 1);
        titleRow->addWidget(typeLabel, 0, Qt::AlignVCenter);

        auto *connectionLabel = new QLabel(connectionSummary(port), card);
        connectionLabel->setObjectName(QStringLiteral("portConnectionSummary"));
        connectionLabel->setWordWrap(true);
        connectionLabel->setMinimumWidth(0);
        connectionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *metaLabel = new QLabel(
            QStringLiteral("轮询 %1 ms · 从站 %2 · 绑定 %3 分组")
                .arg(port.profile.pollIntervalMs)
                .arg(port.profile.slaveAddress)
                .arg(countBindings(groups, port.id)), card);
        metaLabel->setObjectName(QStringLiteral("portMeta"));
        metaLabel->setWordWrap(true);
        metaLabel->setMinimumWidth(0);
        metaLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *bindingLabel = new QLabel(
            QStringLiteral("绑定 %1 分组").arg(countBindings(groups, port.id)), card);
        bindingLabel->setObjectName(QStringLiteral("bindingCount_") + port.id);
        bindingLabel->hide();

        auto *buttonRow = new QHBoxLayout;
        buttonRow->setSpacing(7);
        auto *toggleButton = new QPushButton(card);
        toggleButton->setObjectName(QStringLiteral("togglePort_") + port.id);
        applyToggleStyle(toggleButton, state);
        auto *editButton = new QPushButton(QStringLiteral("编辑"), card);
        editButton->setObjectName(QStringLiteral("editPort_") + port.id);
        editButton->setFixedHeight(30);
        auto *deleteButton = new QPushButton(QStringLiteral("删除"), card);
        deleteButton->setObjectName(QStringLiteral("deletePort_") + port.id);
        deleteButton->setProperty("dangerAction", true);
        deleteButton->setFixedHeight(30);
        toggleButton->setFixedHeight(30);
        toggleButton->setMinimumWidth(64);
        editButton->setFixedWidth(52);
        deleteButton->setFixedWidth(52);
        buttonRow->addWidget(toggleButton, 1);
        buttonRow->addWidget(editButton, 0);
        buttonRow->addWidget(deleteButton, 0);

        cardLayout->addLayout(titleRow);
        cardLayout->addWidget(connectionLabel);
        cardLayout->addWidget(metaLabel);
        cardLayout->addWidget(bindingLabel);
        cardLayout->addLayout(buttonRow);
        m_itemLayout->addWidget(card);

        // 点击卡片空白区域选中；按钮点击不改变选中由按钮自身处理前先选中
        card->installEventFilter(this);
        nameLabel->installEventFilter(this);
        typeLabel->installEventFilter(this);
        connectionLabel->installEventFilter(this);
        metaLabel->installEventFilter(this);
        bindingLabel->installEventFilter(this);
        dot->installEventFilter(this);
        connect(toggleButton, &QPushButton::clicked, this, [this, port]() {
            setSelectedPort(port.id);
        });
        connect(editButton, &QPushButton::clicked, this, [this, port]() {
            setSelectedPort(port.id);
        });
        connect(deleteButton, &QPushButton::clicked, this, [this, port]() {
            setSelectedPort(port.id);
        });

        connect(toggleButton, &QPushButton::clicked, this, [this, port, toggleButton]()
        {
            const RuntimeState currentState = static_cast<RuntimeState>(
                toggleButton->property("runtimeState").toInt());
            if (currentState == RuntimeState::Running)
            {
                emit stopPortRequested(port.id);
            }
            else
            {
                emit startPortRequested(port.id);
            }
        });
        connect(editButton, &QPushButton::clicked, this, [this, port]()
        {
            emit editPortRequested(port.id);
        });
        connect(deleteButton, &QPushButton::clicked, this, [this, port]()
        {
            emit removePortRequested(port.id);
        });
    }

    m_itemLayout->addStretch();

    if (!previousSelected.isEmpty() && m_portIds.contains(previousSelected))
    {
        setSelectedPort(previousSelected);
    }
    else if (!m_portIds.isEmpty())
    {
        setSelectedPort(m_portIds.first());
    }
}

void ConnectionPortListView::updatePortState(const QString &portId, RuntimeState state)
{
    QLabel *dot = findChild<QLabel *>(QStringLiteral("portState_") + portId);
    applyStateDot(dot, state);

    QPushButton *toggleButton =
        findChild<QPushButton *>(QStringLiteral("togglePort_") + portId);
    applyToggleStyle(toggleButton, state);
}

void ConnectionPortListView::setSelectedPort(const QString &portId)
{
    if (m_selectedPortId == portId && !portId.isEmpty())
    {
        applySelectionStyles();
        return;
    }
    m_selectedPortId = portId;
    applySelectionStyles();
    if (!portId.isEmpty())
    {
        emit portSelected(portId);
    }
}

void ConnectionPortListView::focusPortPanel()
{
    raise();
    setFocus(Qt::OtherFocusReason);
    if (m_selectedPortId.isEmpty() && !m_portIds.isEmpty())
    {
        setSelectedPort(m_portIds.first());
    }
    else
    {
        applySelectionStyles();
    }
    // 轻微高亮整栏，提示用户焦点在连接端口
    setStyleSheet(QStringLiteral(
        "QWidget#connectionPortList {"
        "  border: 1px solid rgba(245,78,0,120);"
        "}"
        "QWidget#connectionPortList QScrollArea#portListScrollArea,"
        "QWidget#connectionPortList QWidget#portListContent {"
        "  border: 0;"
        "}"));
    // 1.2s 后去掉临时描边
    QTimer::singleShot(1200, this, [this]() {
        setStyleSheet(QString());
    });
}

void ConnectionPortListView::applySelectionStyles()
{
    for (const QString &portId : m_portIds)
    {
        QWidget *card = findChild<QWidget *>(QStringLiteral("portCard_") + portId);
        if (!card)
        {
            continue;
        }
        const bool selected = (portId == m_selectedPortId);
        card->setProperty("selected", selected);
        card->style()->unpolish(card);
        card->style()->polish(card);
        card->update();
    }
}

bool ConnectionPortListView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->button() == Qt::LeftButton)
        {
            QWidget *widget = qobject_cast<QWidget *>(watched);
            while (widget)
            {
                if (widget->property("portCard").toBool())
                {
                    const QString portId = widget->property("portId").toString();
                    if (!portId.isEmpty())
                    {
                        setSelectedPort(portId);
                    }
                    break;
                }
                widget = widget->parentWidget();
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
