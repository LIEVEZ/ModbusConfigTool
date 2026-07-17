#include "status_bar_view.h"

#include <QHBoxLayout>
#include <QLabel>

StatusBarView::StatusBarView(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 2, 6, 2);
    m_message = new QLabel(QStringLiteral("工作区已就绪。"), this);
    m_runtime = new QLabel(this); m_connection = new QLabel(this);
    m_statistics = new QLabel(this); m_saved = new QLabel(this);
    layout->addWidget(m_message, 1); layout->addWidget(m_runtime); layout->addWidget(m_connection);
    layout->addWidget(m_statistics); layout->addWidget(m_saved);
}

void StatusBarView::updateStatus(const ProjectDocument &document, bool dirty, RuntimeState state)
{
    int enabled = 0;
    for (const RegisterPoint &point : document.registers) { if (point.enabled) { ++enabled; } }
    m_runtime->setText(QStringLiteral("运行: %1").arg(runtimeStateToString(state)));
    m_connection->setText(document.serverProfile.connectionType == ConnectionType::Tcp
        ? QStringLiteral("连接: TCP %1:%2").arg(document.serverProfile.tcpHost).arg(document.serverProfile.tcpPort)
        : QStringLiteral("连接: RTU %1").arg(document.serverProfile.serialPort));
    m_statistics->setText(QStringLiteral("寄存器: %1 | 启用: %2 | 分组: %3").arg(document.registers.size()).arg(enabled).arg(document.groups.size()));
    m_saved->setText(dirty ? QStringLiteral("保存: 未保存") : QStringLiteral("保存: 已保存"));
}

void StatusBarView::showMessage(const QString &message) { m_message->setText(message); }
