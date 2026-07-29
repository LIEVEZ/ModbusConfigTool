#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "Application/Runtime/runtime_service.h"
#include "Domain/Models/comm_frame.h"

#include <QHash>
#include <QMainWindow>

class EventLogView;
class ConnectionPortListView;
class CommMonitorDialog;
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
    void importGroup();
    QString uniqueGroupName(const QString &preferredName, const QString &ignoreGroupId = QString()) const;
    void addPort();
    void editPort(const QString &portId);
    void removePort(const QString &portId);
    void startPort(const QString &portId);
    void stopPort(const QString &portId);
    void showGroupRealtime(const QString &groupId);
    void showGroupConfig(const QString &groupId);
    void editGroup(const QString &groupId);
    void editGroupSlaveAddress(const QString &groupId);
    void copyGroup(const QString &groupId);
    void removeGroup(const QString &groupId);
    void importGroupCsv(const QString &groupId);
    void exportGroupCsv(const QString &groupId);
    void addRegisterToGroup(const QString &groupId);
    void editRegister(const QString &registerId);
    void removeRegisters(const QStringList &registerIds);
    void showResult(const OperationResult &result, const QString &successMessage);
    void rebuildRecentMenu();
    void tryOpenStartupProject();
    void updateGroupCount(int count);
    void updatePortCount(int count);
    void openCommMonitor();
    void setMonitoredPort(const QString &portId);
    void setMonitoredPort(const QString &portId, bool syncCombo);
    void syncCommMonitorPorts();
    void updateCommMonitorPort();
    void onCommFrameCaptured(const QString &portId, const CommFrame &frame);
    QString portNameById(const QString &portId) const;

    MainWindowViewModel *m_viewModel = nullptr;
    ConnectionPortListView *m_portListView = nullptr;
    GroupCanvasView *m_canvasView = nullptr;
    EventLogView *m_logView = nullptr;
    StatusBarView *m_statusView = nullptr;
    CommMonitorDialog *m_commMonitor = nullptr;
    QToolBar *m_workspaceToolBar = nullptr;
    QLabel *m_groupCountBadge = nullptr;
    QLabel *m_portCountBadge = nullptr;
    QSplitter *m_workspaceSplitter = nullptr;
    QMenu *m_recentMenu = nullptr;
    QHash<QString, RuntimeState> m_portStates;
    QString m_selectedGroupId;
    QString m_monitoredPortId;
    QString m_monitoredPortName;
    bool m_refreshScheduled = false;
};

#endif
