#include "event_log_view.h"

#include <QDateTime>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

EventLogView::EventLogView(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("事件日志"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    layout->addWidget(title); layout->addWidget(m_log);
    appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), QStringLiteral("工作区已就绪。"));
}

void EventLogView::appendMessage(const QString &level, const QString &module, const QString &message)
{
    m_log->appendPlainText(QStringLiteral("[%1] [%2] [%3] %4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), level, module, message));
}
