#ifndef RUNTIME_CONTROL_VIEW_H
#define RUNTIME_CONTROL_VIEW_H

#include "Domain/Models/domain_enums.h"
#include "Domain/Models/server_profile.h"

#include <QWidget>

class QLabel;
class QPushButton;

class RuntimeControlView : public QWidget
{
    Q_OBJECT

public:
    explicit RuntimeControlView(QWidget *parent = nullptr);
    void setProfile(const ServerProfile &profile);
    void setDirty(bool dirty);
    void setRuntimeState(RuntimeState state);

signals:
    void configureRequested();
    void startRequested();
    void stopRequested();

private:
    QLabel *m_summary = nullptr;
    QLabel *m_savedBadge = nullptr;
    QLabel *m_runtimeBadge = nullptr;
    QPushButton *m_configureButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
};

#endif
