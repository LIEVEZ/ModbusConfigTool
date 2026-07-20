#ifndef RUNTIME_SERVICE_H
#define RUNTIME_SERVICE_H

#include "Domain/Models/project_document.h"

#include <QHash>
#include <QObject>

class ModbusRuntimeWorker;
class QThread;

class RuntimeService : public QObject
{
    Q_OBJECT

public:
    explicit RuntimeService(QObject *parent = nullptr);
    ~RuntimeService() override;

    RuntimeState portState(const QString &portId) const;

    void startPort(const ProjectDocument &document, const QString &portId);
    void stopPort(const QString &portId);
    void stopAll();
    void writePoint(const QString &pointId, const RegisterValue &value);

signals:
    void portStateChanged(const QString &portId, RuntimeState state);
    void portError(const QString &portId, const QString &message, const QString &detail);
    void valueChanged(const QString &pointId, const RegisterValue &value);

private:
    struct PortRuntime
    {
        QThread *thread = nullptr;
        ModbusRuntimeWorker *worker = nullptr;
        RuntimeState state = RuntimeState::Idle;
    };

    static QList<RegisterPoint> collectPointsForPort(const ProjectDocument &document,
                                                     const QString &portId);
    void setPortState(const QString &portId, RuntimeState state);
    void teardownPort(const QString &portId);

    QHash<QString, PortRuntime> m_ports;
};

#endif
