#ifndef MAIN_WINDOW_VIEW_MODEL_H
#define MAIN_WINDOW_VIEW_MODEL_H

#include "Application/Registers/register_patch.h"
#include "Domain/Interfaces/csv_register_gateway.h"
#include "Domain/Models/project_document.h"
#include "Domain/Values/operation_result.h"

#include <QObject>

class CsvRegisterGatewayImpl;
class JsonProjectRepository;
class ProjectService;
class RegisterService;
class RuntimeService;

class MainWindowViewModel : public QObject
{
    Q_OBJECT

public:
    explicit MainWindowViewModel(QObject *parent = nullptr);
    ~MainWindowViewModel() override;

    const ProjectDocument &document() const;
    QString filePath() const;
    bool isDirty() const;
    QStringList recentFiles() const;
    void removeRecentFile(const QString &path);
    RuntimeState runtimeState() const;

    void newProject();
    OperationResult openProject(const QString &path);
    OperationResult saveProject(const QString &path);
    OperationResult importCsv(const QString &path, bool replaceRegisters);
    OperationResult exportCsv(const QString &path) const;
    OperationResult updateProfile(const ServerProfile &profile);
    OperationResult addGroup(const QString &name);
    OperationResult removeGroup(const QString &groupId, bool removePoints);
    quint16 nextAddress(const QString &groupId) const;
    OperationResult addRegister(const RegisterPoint &point);
    OperationResult updateRegister(const RegisterPoint &point);
    OperationResult removeRegisters(const QStringList &ids);
    OperationResult setRegistersEnabled(const QStringList &ids, bool enabled);
    OperationResult applyRegisterPatch(const QStringList &ids, const RegisterPatch &patch);
    void startRuntime();
    void stopRuntime();
    void writePoint(const QString &pointId, const RegisterValue &value);

signals:
    void documentChanged();
    void dirtyChanged(bool dirty);
    void recentFilesChanged(const QStringList &paths);
    void runtimeStateChanged(RuntimeState state);
    void runtimeError(const QString &message, const QString &detail);
    void runtimeValueChanged(const QString &pointId);

private:
    JsonProjectRepository *m_repository = nullptr;
    CsvRegisterGatewayImpl *m_csvGateway = nullptr;
    ProjectService *m_projectService = nullptr;
    RegisterService *m_registerService = nullptr;
    RuntimeService *m_runtimeService = nullptr;
};

#endif
