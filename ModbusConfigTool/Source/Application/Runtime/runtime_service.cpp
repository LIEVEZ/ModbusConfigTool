#include "runtime_service.h"

#include "Infrastructure/Modbus/modbus_runtime_worker.h"

RuntimeService::RuntimeService(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<ServerProfile>();
    qRegisterMetaType<QList<RegisterPoint>>();
    m_worker = new ModbusRuntimeWorker;
    m_worker->moveToThread(&m_thread);
    connect(this, &RuntimeService::startWorker, m_worker, &ModbusRuntimeWorker::start);
    connect(this, &RuntimeService::stopWorker, m_worker, &ModbusRuntimeWorker::stop);
    connect(m_worker, &ModbusRuntimeWorker::started, this, [this]() { setState(RuntimeState::Running); });
    connect(m_worker, &ModbusRuntimeWorker::stopped, this, [this]()
    {
        if (m_state == RuntimeState::Stopping) { setState(RuntimeState::Idle); }
    });
    connect(m_worker, &ModbusRuntimeWorker::failed, this, [this](const QString &message, const QString &detail)
    {
        setState(RuntimeState::Fault); emit errorOccurred(message, detail);
    });
    m_thread.start();
}

RuntimeService::~RuntimeService()
{
    if (m_state == RuntimeState::Running || m_state == RuntimeState::Starting) { stop(); }
    m_thread.quit(); m_thread.wait(); delete m_worker;
}

RuntimeState RuntimeService::state() const { return m_state; }

void RuntimeService::start(const ProjectDocument &document)
{
    if (m_state != RuntimeState::Idle && m_state != RuntimeState::Fault) { return; }
    setState(RuntimeState::Starting); emit startWorker(document.serverProfile, document.registers);
}

void RuntimeService::stop()
{
    if (m_state != RuntimeState::Running && m_state != RuntimeState::Starting) { return; }
    setState(RuntimeState::Stopping); emit stopWorker();
}

void RuntimeService::setState(RuntimeState state)
{
    if (m_state == state) { return; }
    m_state = state; emit stateChanged(m_state);
}
