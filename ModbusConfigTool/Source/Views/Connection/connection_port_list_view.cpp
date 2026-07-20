#include "connection_port_list_view.h"

#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_group.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
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

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("portListHeader"));
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(14, 12, 14, 12);
    headerLayout->setSpacing(3);

    auto *eyebrow = new QLabel(QStringLiteral("连接"), header);
    eyebrow->setObjectName(QStringLiteral("portListEyebrow"));
    auto *titleRow = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("连接端口"), header);
    title->setObjectName(QStringLiteral("portListTitle"));
    auto *addButton = new QPushButton(QStringLiteral("＋ 端口"), header);
    addButton->setObjectName(QStringLiteral("addPortButton"));
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(addButton);
    headerLayout->addWidget(eyebrow);
    headerLayout->addLayout(titleRow);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("portListScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("portListContent"));
    m_itemLayout = new QVBoxLayout(scrollContent);
    m_itemLayout->setContentsMargins(12, 12, 12, 12);
    m_itemLayout->setSpacing(12);
    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(header);
    mainLayout->addWidget(scrollArea, 1);
    connect(addButton, &QPushButton::clicked,
            this, &ConnectionPortListView::addPortRequested);
}

void ConnectionPortListView::setModel(const QList<ConnectionPort> &ports,
                                      const QList<RegisterGroup> &groups,
                                      const QHash<QString, RuntimeState> &states)
{
    while (QLayoutItem *item = m_itemLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    if (ports.isEmpty())
    {
        auto *emptyLabel = new QLabel(
            QStringLiteral("暂无连接端口\n点击上方“＋ 端口”创建"), this);
        emptyLabel->setObjectName(QStringLiteral("emptyPortHint"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_itemLayout->addWidget(emptyLabel);
        m_itemLayout->addStretch();
        return;
    }

    for (const ConnectionPort &port : ports)
    {
        const RuntimeState state = states.value(port.id, RuntimeState::Idle);
        auto *card = new QWidget(this);
        card->setObjectName(QStringLiteral("portCard_") + port.id);
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setProperty("portCard", true);
        card->setProperty("runtimeState", static_cast<int>(state));
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(7);

        auto *titleRow = new QHBoxLayout;
        titleRow->setSpacing(8);
        auto *dot = new QLabel(card);
        dot->setObjectName(QStringLiteral("portState_") + port.id);
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QStringLiteral("background: %1; border-radius: 5px;")
                           .arg(stateColor(state)));
        auto *nameLabel = new QLabel(
            port.name.isEmpty() ? QStringLiteral("默认端口") : port.name, card);
        nameLabel->setObjectName(QStringLiteral("portName"));
        auto *typeLabel = new QLabel(
            port.profile.connectionType == ConnectionType::Tcp
                ? QStringLiteral("TCP") : QStringLiteral("RTU"), card);
        typeLabel->setObjectName(QStringLiteral("portTypeBadge"));
        titleRow->addWidget(dot);
        titleRow->addWidget(nameLabel, 1);
        titleRow->addWidget(typeLabel);

        auto *connectionLabel = new QLabel(connectionSummary(port), card);
        connectionLabel->setObjectName(QStringLiteral("portConnectionSummary"));
        auto *metaLabel = new QLabel(
            QStringLiteral("轮询 %1 ms · 从站 %2")
                .arg(port.profile.pollIntervalMs)
                .arg(port.profile.slaveAddress), card);
        metaLabel->setObjectName(QStringLiteral("portMeta"));
        auto *bindingLabel = new QLabel(
            QStringLiteral("绑定 %1 分组").arg(countBindings(groups, port.id)), card);
        bindingLabel->setObjectName(QStringLiteral("bindingCount_") + port.id);

        auto *buttonRow = new QHBoxLayout;
        buttonRow->setSpacing(7);
        auto *toggleButton = new QPushButton(
            state == RuntimeState::Running ? QStringLiteral("断开") : QStringLiteral("连接"), card);
        toggleButton->setObjectName(QStringLiteral("togglePort_") + port.id);
        toggleButton->setProperty("runtimeState", static_cast<int>(state));
        toggleButton->setEnabled(state != RuntimeState::Starting
                                 && state != RuntimeState::Stopping);
        auto *editButton = new QPushButton(QStringLiteral("编辑"), card);
        editButton->setObjectName(QStringLiteral("editPort_") + port.id);
        auto *deleteButton = new QPushButton(QStringLiteral("删除"), card);
        deleteButton->setObjectName(QStringLiteral("deletePort_") + port.id);
        deleteButton->setProperty("dangerAction", true);
        buttonRow->addWidget(toggleButton, 1);
        buttonRow->addWidget(editButton);
        buttonRow->addWidget(deleteButton);

        cardLayout->addLayout(titleRow);
        cardLayout->addWidget(connectionLabel);
        cardLayout->addWidget(metaLabel);
        cardLayout->addWidget(bindingLabel);
        cardLayout->addLayout(buttonRow);
        m_itemLayout->addWidget(card);

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
}

void ConnectionPortListView::updatePortState(const QString &portId, RuntimeState state)
{
    QLabel *dot = findChild<QLabel *>(QStringLiteral("portState_") + portId);
    if (dot)
    {
        dot->setStyleSheet(QStringLiteral("background: %1; border-radius: 5px;")
                           .arg(stateColor(state)));
    }

    QPushButton *toggleButton =
        findChild<QPushButton *>(QStringLiteral("togglePort_") + portId);
    if (toggleButton)
    {
        toggleButton->setProperty("runtimeState", static_cast<int>(state));
        toggleButton->setText(
            state == RuntimeState::Running ? QStringLiteral("断开") : QStringLiteral("连接"));
        toggleButton->setEnabled(state != RuntimeState::Starting
                                 && state != RuntimeState::Stopping);
    }
}
