#include "main_window_view_model.h"

#include "Application/Project/project_service.h"
#include "Application/Registers/register_service.h"
#include "Application/Connections/connection_service.h"
#include "Application/Runtime/runtime_service.h"
#include "Infrastructure/Persistence/csv_register_gateway_impl.h"
#include "Infrastructure/Persistence/json_project_repository.h"
#include "Domain/Validation/validation_service.h"

MainWindowViewModel::MainWindowViewModel(QObject *parent) : QObject(parent)
{
    m_repository = new JsonProjectRepository;
    m_csvGateway = new CsvRegisterGatewayImpl;
    m_projectService = new ProjectService(m_repository, this);
    m_registerService = new RegisterService(m_projectService);
    m_connectionService = new ConnectionService(m_projectService);
    m_runtimeService = new RuntimeService(this);
    connect(m_projectService, &ProjectService::documentChanged,
            this, &MainWindowViewModel::documentChanged);
    connect(m_projectService, &ProjectService::dirtyChanged,
            this, &MainWindowViewModel::dirtyChanged);
    connect(m_projectService, &ProjectService::recentFilesChanged,
            this, &MainWindowViewModel::recentFilesChanged);
    connect(m_projectService, &ProjectService::runtimeValueChanged,
            this, &MainWindowViewModel::runtimeValueChanged);
    connect(m_runtimeService, &RuntimeService::portStateChanged,
            this, &MainWindowViewModel::runtimeStateChanged);
    connect(m_runtimeService, &RuntimeService::portError,
            this, &MainWindowViewModel::runtimeError);
    connect(m_runtimeService, &RuntimeService::portDiagnostics,
            this, &MainWindowViewModel::runtimeDiagnostics);
    connect(m_runtimeService, &RuntimeService::frameCaptured,
            this, &MainWindowViewModel::commFrameCaptured);
    connect(m_runtimeService, &RuntimeService::valueChanged,
            m_projectService, &ProjectService::updateRuntimeValue);
}

MainWindowViewModel::~MainWindowViewModel()
{
    delete m_registerService;
    delete m_connectionService;
    delete m_csvGateway;
    delete m_repository;
}

const ProjectDocument &MainWindowViewModel::document() const { return m_projectService->document(); }
QString MainWindowViewModel::filePath() const { return m_projectService->filePath(); }
bool MainWindowViewModel::isDirty() const { return m_projectService->isDirty(); }
QStringList MainWindowViewModel::recentFiles() const { return m_projectService->recentFiles(); }
void MainWindowViewModel::removeRecentFile(const QString &path) { m_projectService->removeRecentFile(path); }
RuntimeState MainWindowViewModel::portState(const QString &portId) const { return m_runtimeService->portState(portId); }
void MainWindowViewModel::newProject() { m_projectService->newProject(); }
OperationResult MainWindowViewModel::openProject(const QString &path) { return m_projectService->open(path); }
OperationResult MainWindowViewModel::saveProject(const QString &path) { return m_projectService->saveAs(path); }

// 端口
ConnectionPort MainWindowViewModel::makeDefaultPort() const { return m_connectionService->makeDefaultPort(); }
OperationResult MainWindowViewModel::addPort(const ConnectionPort &port) { return m_connectionService->addPort(port); }
OperationResult MainWindowViewModel::updatePort(const ConnectionPort &port) { return m_connectionService->updatePort(port); }
OperationResult MainWindowViewModel::removePort(const QString &portId) { return m_connectionService->removePort(portId); }

void MainWindowViewModel::startPort(const QString &portId)
{
    const OperationResult validation = ValidationService::validateProject(document());
    if (!validation.success)
    {
        emit runtimeError(portId, validation.message, validation.detail);
        return;
    }
    m_runtimeService->startPort(document(), portId);
}
void MainWindowViewModel::stopPort(const QString &portId) { m_runtimeService->stopPort(portId); }
void MainWindowViewModel::stopAllPorts() { m_runtimeService->stopAll(); }

// 分组
OperationResult MainWindowViewModel::addGroup(const RegisterGroup &group) { return m_registerService->addGroup(group); }
OperationResult MainWindowViewModel::updateGroup(const RegisterGroup &group) { return m_registerService->updateGroup(group); }
OperationResult MainWindowViewModel::setGroupEnabled(const QString &groupId, bool enabled)
{
    const OperationResult result = m_registerService->setGroupEnabled(groupId, enabled);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}
OperationResult MainWindowViewModel::setGroupPort(const QString &groupId, const QString &portId)
{
    const OperationResult result = m_registerService->setGroupPort(groupId, portId);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}

OperationResult MainWindowViewModel::setGroupSlaveAddress(const QString &groupId, quint8 slaveAddress)
{
    const OperationResult result = m_registerService->setGroupSlaveAddress(groupId, slaveAddress);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}
OperationResult MainWindowViewModel::moveGroup(const QString &groupId, int x, int y) { return m_registerService->moveGroup(groupId, x, y); }
OperationResult MainWindowViewModel::removeGroup(const QString &groupId, bool removePoints)
{
    const OperationResult result = m_registerService->removeGroup(groupId, removePoints);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}

// 分组内寄存器 / CSV
quint16 MainWindowViewModel::nextAddress(const QString &groupId) const { return m_registerService->nextAddress(groupId); }
OperationResult MainWindowViewModel::addRegister(const RegisterPoint &point)
{
    const OperationResult result = m_registerService->addRegister(point);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}
OperationResult MainWindowViewModel::updateRegister(const RegisterPoint &point)
{
    const OperationResult result = m_registerService->updateRegister(point);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}
OperationResult MainWindowViewModel::removeRegisters(const QStringList &ids)
{
    const OperationResult result = m_registerService->removeRegisters(ids);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}

OperationResult MainWindowViewModel::importCsvIntoGroup(const QString &groupId,
                                                        const QString &path,
                                                        bool replaceGroup,
                                                        const QString &groupName)
{
    const CsvImportResult imported = m_csvGateway->importFile(path, document());
    if (!imported.result.success) { return imported.result; }
    OperationResult result = m_registerService->importCsvIntoGroup(groupId, imported, replaceGroup, groupName);
    if (result.success)
    {
        if (!imported.result.message.isEmpty())
        {
            result.message = imported.result.message;
        }
        else
        {
            result.message = QStringLiteral("已导入 %1 条寄存器").arg(imported.registers.size());
        }
        result.detail = imported.result.detail;
        syncRuntimeMaps();
    }
    return result;
}

OperationResult MainWindowViewModel::importGroupFromCsv(const QString &path, const RegisterGroup &group)
{
    const CsvImportResult imported = m_csvGateway->importFile(path, document());
    if (!imported.result.success)
    {
        return imported.result;
    }

    OperationResult result = m_registerService->importCsvAsNewGroup(group, imported);
    if (result.success)
    {
        if (result.message.isEmpty())
        {
            if (!imported.result.message.isEmpty())
            {
                result.message = imported.result.message;
            }
            else
            {
                result.message = QStringLiteral("已导入分组，共 %1 条寄存器")
                                     .arg(imported.registers.size());
            }
        }
        if (result.detail.isEmpty())
        {
            result.detail = imported.result.detail;
        }
        syncRuntimeMaps();
    }
    return result;
}

OperationResult MainWindowViewModel::exportGroupCsv(const QString &groupId, const QString &path) const
{
    ProjectDocument subset = document();
    for (int i = subset.registers.size() - 1; i >= 0; --i)
    {
        if (subset.registers.at(i).groupId != groupId) { subset.registers.removeAt(i); }
    }
    return m_csvGateway->exportFile(path, subset);
}

// 寄存器批量操作
OperationResult MainWindowViewModel::setRegistersEnabled(const QStringList &ids, bool enabled)
{
    const OperationResult result = m_registerService->setEnabled(ids, enabled);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}
OperationResult MainWindowViewModel::applyRegisterPatch(const QStringList &ids, const RegisterPatch &patch)
{
    const OperationResult result = m_registerService->applyPatch(ids, patch);
    if (result.success) { syncRuntimeMaps(); }
    return result;
}

void MainWindowViewModel::writePoint(const QString &pointId, const RegisterValue &value)
{
    m_projectService->updateRuntimeValue(pointId, value);
    m_runtimeService->writePoint(pointId, value);
}

void MainWindowViewModel::writePoints(const QList<QPair<QString, RegisterValue>> &values)
{
    if (values.isEmpty())
    {
        return;
    }

    m_projectService->updateRuntimeValues(values);
    for (const auto &item : values)
    {
        m_runtimeService->writePoint(item.first, item.second);
    }
}

void MainWindowViewModel::syncRuntimeMaps()
{
    m_runtimeService->reloadRunningPorts(document());
}
