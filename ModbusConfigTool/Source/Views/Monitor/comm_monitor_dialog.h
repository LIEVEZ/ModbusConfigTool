#ifndef COMM_MONITOR_DIALOG_H
#define COMM_MONITOR_DIALOG_H

#include <QDialog>

class CommMonitorView;

class CommMonitorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommMonitorDialog(QWidget *parent = nullptr);
    CommMonitorView *view() const;

private:
    CommMonitorView *m_view = nullptr;
};

#endif
