#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class EventLogView;
class CsvRegisterGatewayImpl;
class GroupPanelView;
class JsonProjectRepository;
class ProjectService;
class RegisterConfigView;
class RegisterService;
class RuntimeControlView;
class RuntimeService;
class RuntimeValueView;
class StatusBarView;
struct OperationResult;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildMenus();
    void buildWorkspace();
    void connectActions();
    void refreshDocument();
    bool confirmDiscardChanges();
    void newProject();
    void openProject();
    bool saveProject(bool saveAs = false);
    void addGroup();
    void removeGroup();
    void addRegister();
    void editRegister(const QString &pointId);
    void showResult(const OperationResult &result, const QString &successMessage);

    JsonProjectRepository *m_repository = nullptr;
    CsvRegisterGatewayImpl *m_csvGateway = nullptr;
    ProjectService *m_projectService = nullptr;
    RegisterService *m_registerService = nullptr;
    RuntimeService *m_runtimeService = nullptr;
    RuntimeControlView *m_runtimeControl = nullptr;
    GroupPanelView *m_groupPanel = nullptr;
    RegisterConfigView *m_registerView = nullptr;
    RuntimeValueView *m_runtimeValueView = nullptr;
    EventLogView *m_logView = nullptr;
    StatusBarView *m_statusView = nullptr;
};

#endif
