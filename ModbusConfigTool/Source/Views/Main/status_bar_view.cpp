#include "status_bar_view.h"

#include "Domain/Models/connection_port.h"
#include "Domain/Models/register_point.h"

#include <QHBoxLayout>
#include <QLabel>

StatusBarView::StatusBarView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("statusBarView"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(14);

    m_message = new QLabel(QStringLiteral("工作区已就绪。"), this);
    m_message->setObjectName(QStringLiteral("statusMessage"));
    m_runtime = new QLabel(this);
    m_runtime->setObjectName(QStringLiteral("statusRuntime"));
    m_connection = new QLabel(this);
    m_connection->setObjectName(QStringLiteral("statusConnection"));
    m_statistics = new QLabel(this);
    m_statistics->setObjectName(QStringLiteral("statusStatistics"));
    m_saved = new QLabel(this);
    m_saved->setObjectName(QStringLiteral("statusSaved"));

    layout->addWidget(m_message, 1);
    layout->addWidget(m_runtime);
    layout->addWidget(m_connection);
    layout->addWidget(m_statistics);
    layout->addWidget(m_saved);
}

void StatusBarView::updateStatus(const ProjectDocument &document,
                                 bool dirty,
                                 const QHash<QString, RuntimeState> &portStates)
{
    int enabled = 0;
    for (const RegisterPoint &point : document.registers)
    {
        if (point.enabled)
        {
            ++enabled;
        }
    }

    int running = 0;
    int fault = 0;
    int starting = 0;
    int idle = 0;
    for (const ConnectionPort &port : document.ports)
    {
        const RuntimeState state = portStates.value(port.id, RuntimeState::Idle);
        switch (state)
        {
        case RuntimeState::Running:
            ++running;
            break;
        case RuntimeState::Fault:
            ++fault;
            break;
        case RuntimeState::Starting:
        case RuntimeState::Stopping:
            ++starting;
            break;
        case RuntimeState::Idle:
        default:
            ++idle;
            break;
        }
    }

    const int totalPorts = document.ports.size();
    if (totalPorts == 0)
    {
        m_runtime->setText(QStringLiteral("运行: 无端口"));
        m_connection->setText(QStringLiteral("连接: 无端口"));
    }
    else
    {
        QStringList runtimeParts;
        runtimeParts.append(QStringLiteral("运行中 %1").arg(running));
        if (starting > 0)
        {
            runtimeParts.append(QStringLiteral("过渡 %1").arg(starting));
        }
        if (fault > 0)
        {
            runtimeParts.append(QStringLiteral("故障 %1").arg(fault));
        }
        runtimeParts.append(QStringLiteral("空闲 %1").arg(idle));
        m_runtime->setText(QStringLiteral("运行: %1").arg(runtimeParts.join(QStringLiteral(" · "))));

        // 连接摘要：优先展示运行中端口，否则展示首个端口
        const ConnectionPort *summaryPort = nullptr;
        for (const ConnectionPort &port : document.ports)
        {
            if (portStates.value(port.id, RuntimeState::Idle) == RuntimeState::Running)
            {
                summaryPort = &port;
                break;
            }
        }
        if (!summaryPort)
        {
            summaryPort = &document.ports.first();
        }

        QString detail;
        if (summaryPort->profile.connectionType == ConnectionType::Tcp)
        {
            detail = QStringLiteral("%1 %2:%3")
                         .arg(summaryPort->name.isEmpty()
                                  ? QStringLiteral("TCP")
                                  : summaryPort->name,
                              summaryPort->profile.tcpHost)
                         .arg(summaryPort->profile.tcpPort);
        }
        else
        {
            detail = QStringLiteral("%1 %2")
                         .arg(summaryPort->name.isEmpty()
                                  ? QStringLiteral("RTU")
                                  : summaryPort->name,
                              summaryPort->profile.serialPort);
        }
        m_connection->setText(QStringLiteral("端口: %1 个 · %2").arg(totalPorts).arg(detail));
    }

    m_statistics->setText(
        QStringLiteral("寄存器: %1 | 启用: %2 | 分组: %3")
            .arg(document.registers.size())
            .arg(enabled)
            .arg(document.groups.size()));
    m_saved->setText(dirty ? QStringLiteral("保存: 未保存") : QStringLiteral("保存: 已保存"));
}

void StatusBarView::showMessage(const QString &message)
{
    m_message->setText(message);
}
