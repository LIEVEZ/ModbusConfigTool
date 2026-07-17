#include "main_window_view_model.h"

#include "Application/Project/project_service.h"
#include "Application/Registers/register_service.h"
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
    m_runtimeService = new RuntimeService(this);
    connect(m_projectService, &ProjectService::documentChanged,
            this, &MainWindowViewModel::documentChanged);
    connect(m_projectService, &ProjectService::dirtyChanged,
            this, &MainWindowViewModel::dirtyChanged);
    connect(m_projectService, &ProjectService::recentFilesChanged,
            this, &MainWindowViewModel::recentFilesChanged);
    connect(m_projectService, &ProjectService::runtimeValueChanged,
            this, &MainWindowViewModel::runtimeValueChanged);
    connect(m_runtimeService, &RuntimeService::stateChanged,
            this, &MainWindowViewModel::runtimeStateChanged);
    connect(m_runtimeService, &RuntimeService::errorOccurred,
            this, &MainWindowViewModel::runtimeError);
    connect(m_runtimeService, &RuntimeService::valueChanged,
            m_projectService, &ProjectService::updateRuntimeValue);
}

MainWindowViewModel::~MainWindowViewModel()
{
    delete m_registerService;
    delete m_csvGateway;
    delete m_repository;
}

const ProjectDocument &MainWindowViewModel::document() const { return m_projectService->document(); }
QString MainWindowViewModel::filePath() const { return m_projectService->filePath(); }
bool MainWindowViewModel::isDirty() const { return m_projectService->isDirty(); }
QStringList MainWindowViewModel::recentFiles() const { return m_projectService->recentFiles(); }
void MainWindowViewModel::removeRecentFile(const QString &path) { m_projectService->removeRecentFile(path); }
RuntimeState MainWindowViewModel::runtimeState() const { return m_runtimeService->state(); }
void MainWindowViewModel::newProject() { m_projectService->newProject(); }
OperationResult MainWindowViewModel::openProject(const QString &path) { return m_projectService->open(path); }
OperationResult MainWindowViewModel::saveProject(const QString &path) { return m_projectService->saveAs(path); }

OperationResult MainWindowViewModel::importCsv(const QString &path,
                                               bool replaceRegisters)
{
    const CsvImportResult imported = m_csvGateway->importFile(path, document());
    if (!imported.result.success) { return imported.result; }
    ProjectDocument &target = m_projectService->editableDocument();
    target.groups = imported.groups;
    target.registers = replaceRegisters ? imported.registers
                                        : target.registers + imported.registers;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult MainWindowViewModel::exportCsv(const QString &path) const
{
    return m_csvGateway->exportFile(path, document());
}

OperationResult MainWindowViewModel::updateProfile(const ServerProfile &profile)
{
    m_projectService->editableDocument().serverProfile = profile;
    m_projectService->markDirty();
    return OperationResult::ok();
}

OperationResult MainWindowViewModel::addGroup(const QString &name) { return m_registerService->addGroup(name); }
OperationResult MainWindowViewModel::removeGroup(const QString &id, bool removePoints) { return m_registerService->removeGroup(id, removePoints); }
quint16 MainWindowViewModel::nextAddress(const QString &id) const { return m_registerService->nextAddress(id); }
OperationResult MainWindowViewModel::addRegister(const RegisterPoint &point) { return m_registerService->addRegister(point); }
OperationResult MainWindowViewModel::updateRegister(const RegisterPoint &point) { return m_registerService->updateRegister(point); }
OperationResult MainWindowViewModel::removeRegisters(const QStringList &ids) { return m_registerService->removeRegisters(ids); }
OperationResult MainWindowViewModel::setRegistersEnabled(const QStringList &ids, bool enabled) { return m_registerService->setEnabled(ids, enabled); }
OperationResult MainWindowViewModel::applyRegisterPatch(const QStringList &ids, const RegisterPatch &patch) { return m_registerService->applyPatch(ids, patch); }
void MainWindowViewModel::startRuntime()
{
    const OperationResult validation = ValidationService::validateProject(document());
    if (!validation.success)
    {
        emit runtimeError(validation.message, validation.detail);
        return;
    }
    m_runtimeService->start(document());
}
void MainWindowViewModel::stopRuntime() { m_runtimeService->stop(); }

void MainWindowViewModel::writePoint(const QString &pointId, const RegisterValue &value)
{
    if (runtimeState() == RuntimeState::Running) { m_runtimeService->writePoint(pointId, value); }
    else { m_projectService->updateRuntimeValue(pointId, value); m_projectService->markDirty(); }
}
