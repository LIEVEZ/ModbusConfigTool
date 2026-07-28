#include "event_log_view.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
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

    auto *titleRow = new QWidget(header);
    titleRow->setObjectName(QStringLiteral("eventLogTitleRow"));
    auto *titleRowLayout = new QHBoxLayout(titleRow);
    titleRowLayout->setContentsMargins(0, 0, 0, 0);
    titleRowLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("事件日志"), titleRow);
    title->setObjectName(QStringLiteral("eventLogTitle"));

    m_clearButton = new QPushButton(QStringLiteral("清除"), titleRow);
    m_clearButton->setObjectName(QStringLiteral("eventLogClearButton"));
    m_clearButton->setCursor(Qt::PointingHandCursor);
    m_clearButton->setToolTip(QStringLiteral("清除事件日志"));
    m_clearButton->setFixedHeight(26);

    titleRowLayout->addWidget(title, 1);
    titleRowLayout->addWidget(m_clearButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(eyebrow);
    headerLayout->addWidget(titleRow);

    // Log console
    m_log = new QPlainTextEdit(this);
    m_log->setObjectName(QStringLiteral("eventLogConsole"));
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);

    layout->addWidget(header);
    layout->addWidget(m_log);

    connect(m_clearButton, &QPushButton::clicked, this, &EventLogView::clearLog);

    appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), QStringLiteral("工作区已就绪。"));
}

void EventLogView::appendMessage(const QString &level, const QString &module, const QString &message)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_log->appendPlainText(QStringLiteral("[%1] [%2] [%3] %4").arg(timestamp, level, module, message));
}

void EventLogView::clearLog()
{
    m_log->clear();
    appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), QStringLiteral("事件日志已清除。"));
}
