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
OperationResult MainWindowViewModel::setGroupEnabled(const QString &groupId, bool enabled) { return m_registerService->setGroupEnabled(groupId, enabled); }
OperationResult MainWindowViewModel::setGroupPort(const QString &groupId, const QString &portId) { return m_registerService->setGroupPort(groupId, portId); }
OperationResult MainWindowViewModel::moveGroup(const QString &groupId, int x, int y) { return m_registerService->moveGroup(groupId, x, y); }
OperationResult MainWindowViewModel::removeGroup(const QString &groupId, bool removePoints) { return m_registerService->removeGroup(groupId, removePoints); }

// 分组内寄存器 / CSV
quint16 MainWindowViewModel::nextAddress(const QString &groupId) const { return m_registerService->nextAddress(groupId); }
OperationResult MainWindowViewModel::addRegister(const RegisterPoint &point) { return m_registerService->addRegister(point); }
OperationResult MainWindowViewModel::updateRegister(const RegisterPoint &point) { return m_registerService->updateRegister(point); }
OperationResult MainWindowViewModel::removeRegisters(const QStringList &ids) { return m_registerService->removeRegisters(ids); }

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
OperationResult MainWindowViewModel::setRegistersEnabled(const QStringList &ids, bool enabled) { return m_registerService->setEnabled(ids, enabled); }
OperationResult MainWindowViewModel::applyRegisterPatch(const QStringList &ids, const RegisterPatch &patch) { return m_registerService->applyPatch(ids, patch); }

void MainWindowViewModel::writePoint(const QString &pointId, const RegisterValue &value)
{
    m_projectService->updateRuntimeValue(pointId, value);
    m_runtimeService->writePoint(pointId, value);
    m_projectService->markDirty();
}
