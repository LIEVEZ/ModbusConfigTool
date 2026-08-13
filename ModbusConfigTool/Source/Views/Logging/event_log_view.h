#ifndef EVENT_LOG_VIEW_H
#define EVENT_LOG_VIEW_H

#include <QList>
#include <QString>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

struct LogEntry
{
    QString time;
    QString level;
    QString module;
    QString message;
};

class EventLogView : public QWidget
{
    Q_OBJECT

public:
    explicit EventLogView(QWidget *parent = nullptr);
    void appendMessage(const QString &level, const QString &module, const QString &message);

private slots:
    void clearLog();

private:
    void renderEntries();
    void appendEntry(const LogEntry &entry);
    bool passesFilter(const LogEntry &entry) const;

    QPlainTextEdit *m_log = nullptr;
    QComboBox *m_levelFilter = nullptr;
    QLineEdit *m_textFilter = nullptr;
    QLabel *m_countLabel = nullptr;
    QPushButton *m_clearButton = nullptr;
    QList<LogEntry> m_entries;
};

#endif
