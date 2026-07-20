#include "event_log_view.h"

#include <QDateTime>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

EventLogView::EventLogView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("eventLogView"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header
    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("eventLogHeader"));
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(14, 12, 14, 12);
    headerLayout->setSpacing(3);

    auto *eyebrow = new QLabel(QStringLiteral("系统"), header);
    eyebrow->setObjectName(QStringLiteral("eventLogEyebrow"));

    auto *title = new QLabel(QStringLiteral("事件日志"), header);
    title->setObjectName(QStringLiteral("eventLogTitle"));

    headerLayout->addWidget(eyebrow);
    headerLayout->addWidget(title);

    // Log console
    m_log = new QPlainTextEdit(this);
    m_log->setObjectName(QStringLiteral("eventLogConsole"));
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);

    layout->addWidget(header);
    layout->addWidget(m_log);

    appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), QStringLiteral("工作区已就绪。"));
}

void EventLogView::appendMessage(const QString &level, const QString &module, const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_log->appendPlainText(QStringLiteral("[%1] [%2] [%3] %4").arg(timestamp, level, module, message));
}
