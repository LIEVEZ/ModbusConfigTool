#include "runtime_control_view.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

RuntimeControlView::RuntimeControlView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("runtimeControlCard"));
    auto *layout = new QHBoxLayout(this);
    m_summary = new QLabel(this);
    m_savedBadge = new QLabel(QStringLiteral("已保存"), this);
    m_runtimeBadge = new QLabel(QStringLiteral("空闲"), this);
    m_configureButton = new QPushButton(QStringLiteral("连接配置"), this);
    m_startButton = new QPushButton(QStringLiteral("启动运行"), this);
    m_stopButton = new QPushButton(QStringLiteral("停止运行"), this);
    m_savedBadge->setObjectName(QStringLiteral("successBadge"));
    m_runtimeBadge->setObjectName(QStringLiteral("warningBadge"));
    m_startButton->setObjectName(QStringLiteral("primaryButton"));
    m_stopButton->setObjectName(QStringLiteral("dangerButton"));
    layout->addWidget(m_summary, 1);
    layout->addWidget(m_savedBadge);
    layout->addWidget(m_runtimeBadge);
    layout->addWidget(m_configureButton);
    layout->addWidget(m_startButton);
    layout->addWidget(m_stopButton);
    connect(m_configureButton, &QPushButton::clicked, this, &RuntimeControlView::configureRequested);
    connect(m_startButton, &QPushButton::clicked, this, &RuntimeControlView::startRequested);
    connect(m_stopButton, &QPushButton::clicked, this, &RuntimeControlView::stopRequested);
    setRuntimeState(RuntimeState::Idle);
}

void RuntimeControlView::setProfile(const ServerProfile &profile)
{
    m_summary->setText(profile.connectionType == ConnectionType::Tcp
        ? QStringLiteral("TCP %1:%2 | 轮询 %3 ms").arg(profile.tcpHost).arg(profile.tcpPort).arg(profile.pollIntervalMs)
        : QStringLiteral("RTU %1 | %2 baud | 轮询 %3 ms").arg(profile.serialPort).arg(profile.baudRate).arg(profile.pollIntervalMs));
}

void RuntimeControlView::setDirty(bool dirty)
{
    m_savedBadge->setText(dirty ? QStringLiteral("未保存") : QStringLiteral("已保存"));
    m_savedBadge->setObjectName(dirty ? QStringLiteral("warningBadge") : QStringLiteral("successBadge"));
    style()->unpolish(m_savedBadge); style()->polish(m_savedBadge);
}

void RuntimeControlView::setRuntimeState(RuntimeState state)
{
    m_runtimeBadge->setText(runtimeStateToString(state));
    const bool idle = state == RuntimeState::Idle || state == RuntimeState::Fault;
    m_configureButton->setEnabled(idle);
    m_startButton->setEnabled(idle);
    m_stopButton->setEnabled(state == RuntimeState::Starting || state == RuntimeState::Running);
}
