#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "Application/Runtime/runtime_service.h"

#include <QHash>
#include <QMainWindow>

class EventLogView;
class ConnectionPortListView;
class GroupCanvasView;
class GroupRealtimePanel;
class GroupRegisterConfigDialog;
class MainWindowViewModel;
class StatusBarView;
class QLabel;
class QMenu;
class QSplitter;
class QToolBar;
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
    void buildToolBar();
    void buildWorkspace();
    void connectActions();
    void scheduleRefresh();
    void refreshDocument();
    void refreshStatus();
    bool confirmDiscardChanges();
    void newProject();
    void openProject();
    bool saveProject(bool saveAs = false);
    void addGroup();
    void addPort();
    void editPort(const QString &portId);
    void removePort(const QString &portId);
    void startPort(const QString &portId);
    void stopPort(const QString &portId);
    void showGroupRealtime(const QString &groupId);
    void showGroupConfig(const QString &groupId);
    void editGroup(const QString &groupId);
    void removeGroup(const QString &groupId);
    void importGroupCsv(const QString &groupId);
    void exportGroupCsv(const QString &groupId);
    void addRegisterToGroup(const QString &groupId);
    void editRegister(const QString &registerId);
    void removeRegisters(const QStringList &registerIds);
    void showResult(const OperationResult &result, const QString &successMessage);
    void rebuildRecentMenu();
    void updateGroupCount(int count);

    MainWindowViewModel *m_viewModel = nullptr;
    ConnectionPortListView *m_portListView = nullptr;
    GroupCanvasView *m_canvasView = nullptr;
    EventLogView *m_logView = nullptr;
    StatusBarView *m_statusView = nullptr;
    QToolBar *m_workspaceToolBar = nullptr;
    QLabel *m_groupCountBadge = nullptr;
    QSplitter *m_workspaceSplitter = nullptr;
    QMenu *m_recentMenu = nullptr;
    QHash<QString, RuntimeState> m_portStates;
    QString m_selectedGroupId;
    bool m_refreshScheduled = false;
};

#endif
