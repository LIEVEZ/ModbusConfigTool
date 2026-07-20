#include "runtime_service.h"

#include "Infrastructure/Modbus/modbus_runtime_worker.h"

#include <QMetaObject>
#include <QSet>
#include <QThread>

RuntimeService::RuntimeService(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<ServerProfile>();
    qRegisterMetaType<QList<RegisterPoint>>();
    qRegisterMetaType<RegisterValue>();
    qRegisterMetaType<RuntimeState>();
}

RuntimeService::~RuntimeService()
{
    stopAll();
}

RuntimeState RuntimeService::portState(const QString &portId) const
{
    return m_ports.value(portId).state;
}

QList<RegisterPoint> RuntimeService::collectPointsForPort(const ProjectDocument &document,
                                                          const QString &portId)
{
    QSet<QString> enabledGroups;
    for (const RegisterGroup &group : document.groups)
    {
        if (group.portId == portId && group.enabled) { enabledGroups.insert(group.id); }
    }
    QList<RegisterPoint> points;
    for (const RegisterPoint &point : document.registers)
    {
        if (enabledGroups.contains(point.groupId) && point.enabled) { points.append(point); }
    }
    return points;
}

void RuntimeService::startPort(const ProjectDocument &document, const QString &portId)
{
    PortRuntime &runtime = m_ports[portId];
    if (runtime.state == RuntimeState::Running || runtime.state == RuntimeState::Starting) { return; }

    const ConnectionPort *port = nullptr;
    for (const ConnectionPort &candidate : document.ports)
    {
        if (candidate.id == portId) { port = &candidate; break; }
    }
    if (!port)
    {
        setPortState(portId, RuntimeState::Fault);
        emit portError(portId, QStringLiteral("missing_port"), QStringLiteral("端口不存在"));
        return;
    }

    if (!runtime.thread)
    {
        runtime.thread = new QThread(this);
        runtime.worker = new ModbusRuntimeWorker;
        runtime.worker->moveToThread(runtime.thread);

        connect(runtime.worker, &ModbusRuntimeWorker::started, this,
                [this, portId]() { setPortState(portId, RuntimeState::Running); });
        connect(runtime.worker, &ModbusRuntimeWorker::stopped, this,
                [this, portId]() { setPortState(portId, RuntimeState::Idle); });
        connect(runtime.worker, &ModbusRuntimeWorker::failed, this,
                [this, portId](const QString &message, const QString &detail)
        {
            setPortState(portId, RuntimeState::Fault);
            emit portError(portId, message, detail);
        });
        connect(runtime.worker, &ModbusRuntimeWorker::valueChanged,
                this, &RuntimeService::valueChanged);

        runtime.thread->start();
    }

    setPortState(portId, RuntimeState::Starting);
    const QList<RegisterPoint> points = collectPointsForPort(document, portId);
    QMetaObject::invokeMethod(runtime.worker, "start", Qt::QueuedConnection,
                              Q_ARG(ServerProfile, port->profile),
                              Q_ARG(QList<RegisterPoint>, points));
}

void RuntimeService::stopPort(const QString &portId)
{
    if (!m_ports.contains(portId)) { return; }
    PortRuntime &runtime = m_ports[portId];
    if (!runtime.worker) { return; }
    if (runtime.state == RuntimeState::Running || runtime.state == RuntimeState::Starting)
    {
        QMetaObject::invokeMethod(runtime.worker, "stop", Qt::QueuedConnection);
    }
}

void RuntimeService::stopAll()
{
    const QList<QString> ids = m_ports.keys();
    for (const QString &portId : ids)
    {
        PortRuntime runtime = m_ports.value(portId);
        if (runtime.worker && (runtime.state == RuntimeState::Running || runtime.state == RuntimeState::Starting))
        {
            QMetaObject::invokeMethod(runtime.worker, "stop", Qt::BlockingQueuedConnection);
        }
        teardownPort(portId);
    }
}

void RuntimeService::writePoint(const QString &pointId, const RegisterValue &value)
{
    for (auto it = m_ports.constBegin(); it != m_ports.constEnd(); ++it)
    {
        if (it.value().worker && it.value().state == RuntimeState::Running)
        {
            QMetaObject::invokeMethod(it.value().worker, "writePoint", Qt::QueuedConnection,
                                      Q_ARG(QString, pointId), Q_ARG(RegisterValue, value));
        }
    }
}

void RuntimeService::teardownPort(const QString &portId)
{
    PortRuntime runtime = m_ports.value(portId);
    if (runtime.thread)
    {
        runtime.thread->quit();
        runtime.thread->wait();
        delete runtime.worker;
        delete runtime.thread;
    }
    m_ports.remove(portId);
}

void RuntimeService::setPortState(const QString &portId, RuntimeState state)
{
    PortRuntime &runtime = m_ports[portId];
    if (runtime.state == state) { return; }
    runtime.state = state;
    emit portStateChanged(portId, state);
}
