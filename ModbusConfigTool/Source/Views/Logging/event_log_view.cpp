#include "event_log_view.h"

#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

namespace
{
constexpr int kMaxEntries = 2000;

QString levelLabel(const QString &level)
{
    if (level == QStringLiteral("RUNTIME"))
    {
        return QStringLiteral("运行");
    }
    if (level == QStringLiteral("INFO"))
    {
        return QStringLiteral("信息");
    }
    if (level == QStringLiteral("WARNING"))
    {
        return QStringLiteral("警告");
    }
    if (level == QStringLiteral("ERROR"))
    {
        return QStringLiteral("错误");
    }
    return level;
}

QString levelColor(const QString &level)
{
    if (level == QStringLiteral("RUNTIME"))
    {
        return QStringLiteral("#1f8a65");
    }
    if (level == QStringLiteral("WARNING"))
    {
        return QStringLiteral("#c08532");
    }
    if (level == QStringLiteral("ERROR"))
    {
        return QStringLiteral("#cf2d56");
    }
    return QStringLiteral("#6b6a63");
}
} // namespace

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
    headerLayout->setContentsMargins(14, 12, 14, 10);
    headerLayout->setSpacing(6);

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

    // 过滤器：按级别、按关键字快速定位日志
    auto *filterRow = new QWidget(header);
    filterRow->setObjectName(QStringLiteral("eventLogFilterRow"));
    auto *filterLayout = new QHBoxLayout(filterRow);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    m_levelFilter = new QComboBox(filterRow);
    m_levelFilter->setObjectName(QStringLiteral("eventLogFilterLevel"));
    m_levelFilter->addItem(QStringLiteral("全部级别"), QString());
    m_levelFilter->addItem(QStringLiteral("运行"), QStringLiteral("RUNTIME"));
    m_levelFilter->addItem(QStringLiteral("信息"), QStringLiteral("INFO"));
    m_levelFilter->addItem(QStringLiteral("警告"), QStringLiteral("WARNING"));
    m_levelFilter->addItem(QStringLiteral("错误"), QStringLiteral("ERROR"));

    m_textFilter = new QLineEdit(filterRow);
    m_textFilter->setObjectName(QStringLiteral("eventLogFilterText"));
    m_textFilter->setPlaceholderText(QStringLiteral("搜索日志内容…"));
    m_textFilter->setClearButtonEnabled(true);

    m_countLabel = new QLabel(filterRow);
    m_countLabel->setObjectName(QStringLiteral("eventLogFilterCount"));
    m_countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    filterLayout->addWidget(m_levelFilter, 0);
    filterLayout->addWidget(m_textFilter, 1);
    filterLayout->addWidget(m_countLabel, 0);

    headerLayout->addWidget(eyebrow);
    headerLayout->addWidget(titleRow);
    headerLayout->addWidget(filterRow);

    // Log console（HTML 富文本，按级别着色）
    m_log = new QPlainTextEdit(this);
    m_log->setObjectName(QStringLiteral("eventLogConsole"));
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(kMaxEntries);
    QFont monoFont(QStringLiteral("Consolas"));
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPixelSize(12);
    m_log->document()->setDefaultFont(monoFont);

    layout->addWidget(header);
    layout->addWidget(m_log);

    connect(m_clearButton, &QPushButton::clicked, this, &EventLogView::clearLog);
    connect(m_levelFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { renderEntries(); });
    connect(m_textFilter, &QLineEdit::textChanged,
            this, [this](const QString &) { renderEntries(); });

    appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), QStringLiteral("工作区已就绪。"));
}

void EventLogView::appendMessage(const QString &level, const QString &module, const QString &message)
{
    LogEntry entry;
    entry.time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    entry.level = level;
    entry.module = module;
    entry.message = message;
    m_entries.append(entry);
    while (m_entries.size() > kMaxEntries)
    {
        m_entries.removeFirst();
    }
    appendEntry(entry);
}

void EventLogView::renderEntries()
{
    m_log->clear();
    for (const LogEntry &entry : m_entries)
    {
        appendEntry(entry);
    }
}

void EventLogView::appendEntry(const LogEntry &entry)
{
    if (!passesFilter(entry))
    {
        return;
    }
    // 用户停留在历史位置时不强制滚动到底部
    QScrollBar *bar = m_log->verticalScrollBar();
    const bool atBottom = bar->value() >= bar->maximum() - 4;

    const QString html = QStringLiteral(
                             "<span style=\"color:#9a9992\">%1</span> "
                             "<b><span style=\"color:%2\">[%3]</span></b> "
                             "<span style=\"color:#7a786e\">%4</span>  "
                             "<span style=\"color:#26251e\">%5</span>")
                             .arg(entry.time,
                                  levelColor(entry.level),
                                  levelLabel(entry.level),
                                  entry.module.toHtmlEscaped(),
                                  entry.message.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>")));
    m_log->appendHtml(html);

    if (atBottom)
    {
        bar->setValue(bar->maximum());
    }

    m_countLabel->setText(QStringLiteral("共 %1 条").arg(m_entries.size()));
}

bool EventLogView::passesFilter(const LogEntry &entry) const
{
    const QString level = m_levelFilter->currentData().toString();
    if (!level.isEmpty() && entry.level != level)
    {
        return false;
    }
    const QString needle = m_textFilter->text().trimmed();
    if (needle.isEmpty())
    {
        return true;
    }
    return entry.message.contains(needle, Qt::CaseInsensitive)
           || entry.module.contains(needle, Qt::CaseInsensitive)
           || levelLabel(entry.level).contains(needle);
}

void EventLogView::clearLog()
{
    m_entries.clear();
    m_log->clear();
    appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), QStringLiteral("事件日志已清除。"));
}
