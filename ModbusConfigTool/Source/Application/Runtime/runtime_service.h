#ifndef RUNTIME_SERVICE_H
#define RUNTIME_SERVICE_H

#include "Domain/Models/project_document.h"

#include <QObject>
#include <QThread>

class ModbusRuntimeWorker;

class RuntimeService : public QObject
{
    Q_OBJECT

public:
    explicit RuntimeService(QObject *parent = nullptr);
    ~RuntimeService() override;
    RuntimeState state() const;
    void start(const ProjectDocument &document);
    void stop();

signals:
    void startWorker(const ServerProfile &profile, const QList<RegisterPoint> &points);
    void stopWorker();
    void stateChanged(RuntimeState state);
    void errorOccurred(const QString &message, const QString &detail);

private:
    void setState(RuntimeState state);
    QThread m_thread;
    ModbusRuntimeWorker *m_worker = nullptr;
    RuntimeState m_state = RuntimeState::Idle;
};

#endif
