#ifndef EVENT_LOG_VIEW_H
#define EVENT_LOG_VIEW_H

#include <QWidget>

class QPlainTextEdit;
class QPushButton;

class EventLogView : public QWidget
{
    Q_OBJECT

public:
    explicit EventLogView(QWidget *parent = nullptr);
    void appendMessage(const QString &level, const QString &module, const QString &message);

private slots:
    void clearLog();

private:
    QPlainTextEdit *m_log = nullptr;
    QPushButton *m_clearButton = nullptr;
};

#endif
