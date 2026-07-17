#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class EventLogView;
class GroupPanelView;
class MainWindowViewModel;
class RegisterConfigView;
class RuntimeControlView;
class RuntimeValueView;
class StatusBarView;
class QMenu;
struct OperationResult;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(MainWindowViewModel *viewModel = nullptr,
                        QWidget *parent = nullptr);
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
    void rebuildRecentMenu();

    MainWindowViewModel *m_viewModel = nullptr;
    RuntimeControlView *m_runtimeControl = nullptr;
    GroupPanelView *m_groupPanel = nullptr;
    RegisterConfigView *m_registerView = nullptr;
    RuntimeValueView *m_runtimeValueView = nullptr;
    EventLogView *m_logView = nullptr;
    StatusBarView *m_statusView = nullptr;
    QMenu *m_recentMenu = nullptr;
};

#endif
