#include "runtime_service.h"

#include "Infrastructure/Modbus/modbus_runtime_worker.h"

#include <QMetaObject>
#include <QSet>
#include <QThread>

#include <algorithm>

RuntimeService::RuntimeService(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<ServerProfile>();
    qRegisterMetaType<RegisterPoint>();
    qRegisterMetaType<QList<RegisterPoint>>();
    qRegisterMetaType<RegisterValue>();
    qRegisterMetaType<RuntimeState>();
    qRegisterMetaType<CommFrame>();
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
        if (group.portId == portId && group.enabled)
        {
            enabledGroups.insert(group.id);
        }
    }
    QList<RegisterPoint> points;
    for (const RegisterPoint &point : document.registers)
    {
        // 点位启用字段已废弃：只要分组启用，导入点位全部参与映射。
        if (enabledGroups.contains(point.groupId))
        {
            points.append(point);
        }
    }
    return points;
}

QHash<QString, QString> RuntimeService::groupNamesForPort(const ProjectDocument &document,
                                                          const QString &portId)
{
    QHash<QString, QString> names;
    for (const RegisterGroup &group : document.groups)
    {
        if (group.portId == portId && group.enabled)
        {
            names.insert(group.id, group.name);
        }
    }
    return names;
}

void RuntimeService::startPort(const ProjectDocument &document, const QString &portId)
{
    PortRuntime &runtime = m_ports[portId];
    if (runtime.state == RuntimeState::Running || runtime.state == RuntimeState::Starting)
    {
        // 已在运行：热更新映射，避免分组启停后仍使用旧地址表
        reloadRunningPorts(document);
        return;
    }

    const ConnectionPort *port = nullptr;
    for (const ConnectionPort &candidate : document.ports)
    {
        if (candidate.id == portId)
        {
            port = &candidate;
            break;
        }
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
        connect(runtime.worker, &ModbusRuntimeWorker::diagnostics, this,
                [this, portId](const QString &message)
        {
            emit portDiagnostics(portId, message);
        });
        connect(runtime.worker, &ModbusRuntimeWorker::frameCaptured, this,
                [this, portId](const CommFrame &frame)
        {
            emit frameCaptured(portId, frame);
        });

        runtime.thread->start();
    }

    setPortState(portId, RuntimeState::Starting);
    const QList<RegisterPoint> points = collectPointsForPort(document, portId);
    const QHash<QString, QString> groupNames = groupNamesForPort(document, portId);
    QStringList groupNameList = groupNames.values();
    std::sort(groupNameList.begin(), groupNameList.end());
    emit portDiagnostics(portId,
                         QStringLiteral("准备启动：绑定启用分组点位 %1 个（%2），%3 %4:%5")
                             .arg(points.size())
                             .arg(groupNameList.isEmpty() ? QStringLiteral("无")
                                                          : groupNameList.join(QStringLiteral("、")))
                             .arg(port->profile.connectionType == ConnectionType::Tcp
                                      ? QStringLiteral("TCP")
                                      : QStringLiteral("RTU"))
                             .arg(port->profile.connectionType == ConnectionType::Tcp
                                      ? port->profile.tcpHost
                                      : port->profile.serialPort)
                             .arg(port->profile.connectionType == ConnectionType::Tcp
                                      ? port->profile.tcpPort
                                      : port->profile.baudRate));

    // 使用 functor 排队调用，避免 QList<RegisterPoint> 通过 Q_ARG 跨线程拷贝异常
    const ServerProfile profile = port->profile;
    ModbusRuntimeWorker *worker = runtime.worker;
    const bool invoked = QMetaObject::invokeMethod(worker, [worker, profile, points, groupNames]() {
        worker->start(profile, points, groupNames);
    }, Qt::QueuedConnection);
    if (!invoked)
    {
        setPortState(portId, RuntimeState::Fault);
        emit portError(portId,
                       QStringLiteral("无法调度运行时启动"),
                       QStringLiteral("QMetaObject::invokeMethod 失败"));
    }
}


void RuntimeService::reloadRunningPorts(const ProjectDocument &document)
{
    for (auto it = m_ports.begin(); it != m_ports.end(); ++it)
    {
        PortRuntime &runtime = it.value();
        if (!runtime.worker)
        {
            continue;
        }
        if (runtime.state != RuntimeState::Running && runtime.state != RuntimeState::Starting)
        {
            continue;
        }

        const QString portId = it.key();
        const QList<RegisterPoint> points = collectPointsForPort(document, portId);
        const QHash<QString, QString> groupNames = groupNamesForPort(document, portId);
        ModbusRuntimeWorker *worker = runtime.worker;
        const bool invoked = QMetaObject::invokeMethod(worker, [worker, points, groupNames]() {
            worker->reloadPoints(points, groupNames);
        }, Qt::QueuedConnection);
        if (!invoked)
        {
            emit portError(portId,
                           QStringLiteral("无法热更新寄存器映射"),
                           QStringLiteral("QMetaObject::invokeMethod 失败"));
            continue;
        }
        QStringList groupNameList = groupNames.values();
        std::sort(groupNameList.begin(), groupNameList.end());
        emit portDiagnostics(portId,
                             QStringLiteral("热更新映射：启用绑定分组点位 %1 个（%2）")
                                 .arg(points.size())
                                 .arg(groupNameList.isEmpty() ? QStringLiteral("无")
                                                              : groupNameList.join(QStringLiteral("、"))));
    }
}

void RuntimeService::stopPort(const QString &portId)
{
    if (!m_ports.contains(portId))
    {
        return;
    }
    PortRuntime &runtime = m_ports[portId];
    if (!runtime.worker)
    {
        return;
    }
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
    if (runtime.state == state)
    {
        return;
    }
    runtime.state = state;
    emit portStateChanged(portId, state);
}
