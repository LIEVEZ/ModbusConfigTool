#ifndef MAIN_WINDOW_VIEW_MODEL_H
#define MAIN_WINDOW_VIEW_MODEL_H

#include "Application/Registers/register_patch.h"
#include "Domain/Interfaces/csv_register_gateway.h"
#include "Domain/Models/project_document.h"
#include "Domain/Models/connection_port.h"
#include "Domain/Values/operation_result.h"

#include <QObject>

class CsvRegisterGatewayImpl;
class JsonProjectRepository;
class ProjectService;
class RegisterService;
class ConnectionService;
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
    RuntimeState portState(const QString &portId) const;

    void newProject();
    OperationResult openProject(const QString &path);
    OperationResult saveProject(const QString &path);

    // 端口
    ConnectionPort makeDefaultPort() const;
    OperationResult addPort(const ConnectionPort &port);
    OperationResult updatePort(const ConnectionPort &port);
    OperationResult removePort(const QString &portId);
    void startPort(const QString &portId);
    void stopPort(const QString &portId);
    void stopAllPorts();

    // 分组（完整结构）
    OperationResult addGroup(const RegisterGroup &group);
    OperationResult updateGroup(const RegisterGroup &group);
    OperationResult setGroupEnabled(const QString &groupId, bool enabled);
    OperationResult setGroupPort(const QString &groupId, const QString &portId);
    OperationResult moveGroup(const QString &groupId, int x, int y);
    OperationResult removeGroup(const QString &groupId, bool removePoints);

    // 分组内寄存器 / CSV
    quint16 nextAddress(const QString &groupId) const;
    OperationResult addRegister(const RegisterPoint &point);
    OperationResult updateRegister(const RegisterPoint &point);
    OperationResult removeRegisters(const QStringList &ids);
    OperationResult importCsvIntoGroup(const QString &groupId, const QString &path, bool replaceGroup);
    OperationResult importGroupFromCsv(const QString &path, const RegisterGroup &group);
    OperationResult exportGroupCsv(const QString &groupId, const QString &path) const;

    // 寄存器批量操作
    OperationResult setRegistersEnabled(const QStringList &ids, bool enabled);
    OperationResult applyRegisterPatch(const QStringList &ids, const RegisterPatch &patch);

    // 运行时写点位
    void writePoint(const QString &pointId, const RegisterValue &value);

signals:
    void documentChanged();
    void dirtyChanged(bool dirty);
    void recentFilesChanged(const QStringList &paths);
    void runtimeStateChanged(const QString &portId, RuntimeState state);
    void runtimeError(const QString &portId, const QString &message, const QString &detail);
    void runtimeValueChanged(const QString &pointId);

private:
    JsonProjectRepository *m_repository = nullptr;
    CsvRegisterGatewayImpl *m_csvGateway = nullptr;
    ProjectService *m_projectService = nullptr;
    RegisterService *m_registerService = nullptr;
    ConnectionService *m_connectionService = nullptr;
    RuntimeService *m_runtimeService = nullptr;
};

#endif
