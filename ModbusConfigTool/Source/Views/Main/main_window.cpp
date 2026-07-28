#include "main_window.h"

#include "Domain/Models/project_factory.h"
#include "ViewModels/Main/main_window_view_model.h"
#include "Views/Connection/connection_port_list_view.h"
#include "Views/Dialogs/connection_config_dialog.h"
#include "Views/Dialogs/group_editor_dialog.h"
#include "Views/Dialogs/register_editor_dialog.h"
#include "Views/Groups/group_canvas_view.h"
#include "Views/Groups/group_realtime_panel.h"
#include "Views/Groups/group_register_config_dialog.h"
#include "Views/Logging/event_log_view.h"
#include "Views/Monitor/comm_monitor_dialog.h"
#include "Views/Monitor/comm_monitor_view.h"
#include "Views/Main/status_bar_view.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QSet>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QUuid>
#include <QPair>
#include <QVector>

MainWindow::MainWindow(MainWindowViewModel *viewModel, QWidget *parent)
    : QMainWindow(parent),
      m_viewModel(viewModel)
{
    if (!m_viewModel)
    {
        m_viewModel = new MainWindowViewModel(this);
    }
    buildWorkspace();
    buildMenus();
    buildToolBar();
    connectActions();
    refreshDocument();
    tryOpenStartupProject();
    resize(1600, 900);
    setMinimumSize(1200, 700);
    setWindowTitle(QStringLiteral("Modbus 配置工具"));
}

void MainWindow::buildToolBar()
{
    m_workspaceToolBar = addToolBar(QStringLiteral("工作区"));
    m_workspaceToolBar->setObjectName(QStringLiteral("workspaceToolBar"));
    m_workspaceToolBar->setMovable(false);
    m_workspaceToolBar->setFloatable(false);

    auto *title = new QLabel(QStringLiteral("分组画布"), m_workspaceToolBar);
    title->setObjectName(QStringLiteral("workspaceTitle"));
    m_workspaceToolBar->addWidget(title);

    QAction *addGroupAction = m_workspaceToolBar->addAction(QStringLiteral("＋ 新增分组"));
    addGroupAction->setObjectName(QStringLiteral("addGroupToolAction"));
    if (auto *addGroupButton = qobject_cast<QToolButton *>(
            m_workspaceToolBar->widgetForAction(addGroupAction)))
    {
        addGroupButton->setObjectName(QStringLiteral("addGroupToolButton"));
        addGroupButton->setFixedHeight(32);
    }

    m_groupCountBadge = new QLabel(m_workspaceToolBar);
    m_groupCountBadge->setObjectName(QStringLiteral("groupCountBadge"));
    m_groupCountBadge->setFixedHeight(32);
    m_groupCountBadge->setAlignment(Qt::AlignCenter);
    m_workspaceToolBar->addWidget(m_groupCountBadge);

    QAction *addPortAction = m_workspaceToolBar->addAction(QStringLiteral("＋ 端口"));
    addPortAction->setObjectName(QStringLiteral("addPortToolAction"));
    if (auto *addPortButton = qobject_cast<QToolButton *>(
            m_workspaceToolBar->widgetForAction(addPortAction)))
    {
        addPortButton->setObjectName(QStringLiteral("addPortToolButton"));
        addPortButton->setFixedHeight(32);
    }

    m_portCountBadge = new QLabel(m_workspaceToolBar);
    m_portCountBadge->setObjectName(QStringLiteral("portCountBadge"));
    m_portCountBadge->setFixedHeight(32);
    m_portCountBadge->setAlignment(Qt::AlignCenter);
    m_workspaceToolBar->addWidget(m_portCountBadge);

    QAction *importGroupAction = m_workspaceToolBar->addAction(QStringLiteral("导入分组"));
    importGroupAction->setObjectName(QStringLiteral("importGroupToolAction"));
    if (auto *importGroupButton = qobject_cast<QToolButton *>(
            m_workspaceToolBar->widgetForAction(importGroupAction)))
    {
        importGroupButton->setObjectName(QStringLiteral("importGroupToolButton"));
        importGroupButton->setFixedHeight(32);
    }

    auto *spacer = new QWidget(m_workspaceToolBar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_workspaceToolBar->addWidget(spacer);

    auto *hint = new QLabel(
        QStringLiteral("悬停看摘要 · 双击看实时数值 · 右键更多操作 · 拖动移动位置"),
        m_workspaceToolBar);
    hint->setObjectName(QStringLiteral("workspaceHintBadge"));
    hint->setFixedHeight(32);
    hint->setAlignment(Qt::AlignCenter);
    m_workspaceToolBar->addWidget(hint);

    connect(addGroupAction, &QAction::triggered, this, &MainWindow::addGroup);
    connect(addPortAction, &QAction::triggered, this, &MainWindow::addPort);
    connect(importGroupAction, &QAction::triggered, this, &MainWindow::importGroup);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildMenus()
{
    QMenu *project = menuBar()->addMenu(QStringLiteral("项目"));
    QAction *newAction = project->addAction(QStringLiteral("新建工程"));
    newAction->setObjectName(QStringLiteral("newProjectAction"));
    QAction *openAction = project->addAction(QStringLiteral("打开工程"));
    openAction->setObjectName(QStringLiteral("openProjectAction"));
    m_recentMenu = project->addMenu(QStringLiteral("最近工程"));
    project->addSeparator();
    QAction *saveAction = project->addAction(QStringLiteral("保存工程"));
    saveAction->setObjectName(QStringLiteral("saveProjectAction"));
    QAction *saveAsAction = project->addAction(QStringLiteral("工程另存为"));
    project->addSeparator();
    QAction *closeAction = project->addAction(QStringLiteral("关闭程序"));

    QMenu *organization = menuBar()->addMenu(QStringLiteral("组织"));
    QAction *addGroupAction = organization->addAction(QStringLiteral("新增分组"));
    QAction *importGroupMenuAction = organization->addAction(QStringLiteral("导入分组"));
    importGroupMenuAction->setObjectName(QStringLiteral("importGroupMenuAction"));


    QMenu *commMonitorMenu = menuBar()->addMenu(QStringLiteral("通信监控"));
    commMonitorMenu->setObjectName(QStringLiteral("commMonitorMenu"));
    QAction *commMonitorAction = commMonitorMenu->addAction(QStringLiteral("打开通信监控"));
    commMonitorAction->setObjectName(QStringLiteral("commMonitorAction"));
    commMonitorAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+M")));
    QMenu *connection = menuBar()->addMenu(QStringLiteral("连接配置"));
    connection->setObjectName(QStringLiteral("connectionConfigMenu"));
    QAction *addPortAction = connection->addAction(QStringLiteral("新增端口"));
    addPortAction->setObjectName(QStringLiteral("addPortMenuAction"));
    QAction *managePortsAction = connection->addAction(QStringLiteral("管理连接端口"));
    managePortsAction->setObjectName(QStringLiteral("managePortsMenuAction"));

    QMenu *help = menuBar()->addMenu(QStringLiteral("帮助"));
    help->setObjectName(QStringLiteral("helpMenu"));
    QAction *aboutAction = help->addAction(QStringLiteral("关于"));
    aboutAction->setObjectName(QStringLiteral("aboutAction"));

    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveAction, &QAction::triggered, this, [this]() { saveProject(false); });
    connect(saveAsAction, &QAction::triggered, this, [this]() { saveProject(true); });
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    connect(addGroupAction, &QAction::triggered, this, &MainWindow::addGroup);
    connect(importGroupMenuAction, &QAction::triggered, this, &MainWindow::importGroup);
    connect(addPortAction, &QAction::triggered, this, &MainWindow::addPort);
    connect(managePortsAction, &QAction::triggered, this, [this]() {
        if (m_portListView)
        {
            m_portListView->focusPortPanel();
        }
    });
    connect(commMonitorAction, &QAction::triggered, this, &MainWindow::openCommMonitor);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(
            this,
            QStringLiteral("关于 Modbus 配置工具"),
            QStringLiteral(
                "<b>Modbus 配置工具</b><br/>"
                "多端口连接 · 分组画布 · 寄存器实时监控<br/><br/>"
                "用于管理 TCP/RTU 连接端口、寄存器分组与实时采集配置。"));
    });

    rebuildRecentMenu();
}

void MainWindow::buildWorkspace()
{
    m_workspaceSplitter = new QSplitter(Qt::Horizontal, this);
    m_workspaceSplitter->setObjectName(QStringLiteral("workspaceSplitter"));
    m_workspaceSplitter->setChildrenCollapsible(false);

    m_portListView = new ConnectionPortListView(m_workspaceSplitter);
    m_portListView->setMinimumWidth(260);
    m_portListView->setMaximumWidth(380);

    m_canvasView = new GroupCanvasView(m_workspaceSplitter);
    auto *canvasScrollArea = new QScrollArea(m_workspaceSplitter);
    canvasScrollArea->setObjectName(QStringLiteral("groupCanvasScrollArea"));
    canvasScrollArea->setWidgetResizable(true);
    canvasScrollArea->setFrameShape(QFrame::NoFrame);
    canvasScrollArea->setWidget(m_canvasView);

    m_logView = new EventLogView(m_workspaceSplitter);
    m_logView->setMinimumWidth(260);
    m_logView->setMaximumWidth(420);

    m_workspaceSplitter->addWidget(m_portListView);
    m_workspaceSplitter->addWidget(canvasScrollArea);
    m_workspaceSplitter->addWidget(m_logView);
    m_workspaceSplitter->setStretchFactor(0, 0);
    m_workspaceSplitter->setStretchFactor(1, 1);
    m_workspaceSplitter->setStretchFactor(2, 0);
    m_workspaceSplitter->setSizes(QList<int>() << 280 << 1020 << 300);

    setCentralWidget(m_workspaceSplitter);

    m_statusView = new StatusBarView(this);
    statusBar()->addPermanentWidget(m_statusView, 1);
}

void MainWindow::connectActions()
{
    // ViewModel signals
    connect(m_viewModel, &MainWindowViewModel::documentChanged,
            this, &MainWindow::scheduleRefresh);
    connect(m_viewModel, &MainWindowViewModel::dirtyChanged,
            this, [this](bool) { refreshStatus(); });
    connect(m_viewModel, &MainWindowViewModel::recentFilesChanged, this, &MainWindow::rebuildRecentMenu);

    connect(m_viewModel, &MainWindowViewModel::runtimeStateChanged, this,
            [this](const QString &portId, RuntimeState state)
    {
        m_portStates.insert(portId, state);
        m_portListView->updatePortState(portId, state);
        m_canvasView->updatePortStates(m_portStates);
        m_logView->appendMessage(QStringLiteral("RUNTIME"), QStringLiteral("MODBUS"),
                                 QStringLiteral("端口 %1: %2").arg(portId, runtimeStateToString(state)));
        refreshStatus();
    });

    connect(m_viewModel, &MainWindowViewModel::runtimeError, this,
            [this](const QString &portId, const QString &message, const QString &detail)
    {
        m_logView->appendMessage(QStringLiteral("ERROR"), QStringLiteral("MODBUS"),
                                 QStringLiteral("端口 %1: %2 - %3").arg(portId, message, detail));
        QMessageBox::critical(this, QStringLiteral("运行时错误"),
                              QStringLiteral("端口 %1\n%2\n%3").arg(portId, message, detail));
    });

    connect(m_viewModel, &MainWindowViewModel::runtimeDiagnostics, this,
            [this](const QString &portId, const QString &message)
    {
        m_logView->appendMessage(QStringLiteral("INFO"), QStringLiteral("MODBUS"),
                                 QStringLiteral("端口 %1: %2").arg(portId, message));
    });

    connect(m_viewModel, &MainWindowViewModel::commFrameCaptured,
            this, &MainWindow::onCommFrameCaptured);

    // Port list view signals
    connect(m_portListView, &ConnectionPortListView::addPortRequested, this, &MainWindow::addPort);
    connect(m_portListView, &ConnectionPortListView::editPortRequested, this, &MainWindow::editPort);
    connect(m_portListView, &ConnectionPortListView::removePortRequested, this, &MainWindow::removePort);
    connect(m_portListView, &ConnectionPortListView::startPortRequested,
            this, &MainWindow::startPort);
    connect(m_portListView, &ConnectionPortListView::stopPortRequested,
            this, &MainWindow::stopPort);
    connect(m_portListView, &ConnectionPortListView::portSelected, this,
            [this](const QString &portId)
    {
        setMonitoredPort(portId);
    });

    // Canvas view signals
    connect(m_canvasView, &GroupCanvasView::groupMoved, this,
            [this](const QString &groupId, int x, int y)
    {
        const OperationResult result = m_viewModel->moveGroup(groupId, x, y);
        if (!result.success)
        {
            m_logView->appendMessage(QStringLiteral("WARNING"), QStringLiteral("APP"), result.message);
            scheduleRefresh();
        }
    });

    connect(m_canvasView, &GroupCanvasView::groupSelected, this,
            [this](const QString &groupId)
    {
        m_selectedGroupId = groupId;
        m_canvasView->setSelectedGroup(groupId);
    });

    connect(m_canvasView, &GroupCanvasView::groupEnabledChangeRequested, this,
            [this](const QString &groupId, bool enabled)
    {
        m_selectedGroupId = groupId;
        showResult(m_viewModel->setGroupEnabled(groupId, enabled),
                   enabled ? QStringLiteral("分组已启用") : QStringLiteral("分组已停用"));
    });

    connect(m_canvasView, &GroupCanvasView::groupPortChangeRequested, this,
            [this](const QString &groupId, const QString &portId)
    {
        m_selectedGroupId = groupId;
        const OperationResult result = m_viewModel->setGroupPort(groupId, portId);
        showResult(result,
                   portId.isEmpty() ? QStringLiteral("分组已解除端口绑定")
                                    : QStringLiteral("分组端口已更新"));
        if (!result.success)
        {
            scheduleRefresh();
        }
    });

    connect(m_canvasView, &GroupCanvasView::groupDoubleClicked, this, &MainWindow::showGroupRealtime);

    connect(m_canvasView, &GroupCanvasView::groupContextMenuRequested, this,
            [this](const QString &groupId, const QPoint &globalPos)
    {
        m_selectedGroupId = groupId;
        QMenu menu(this);
        menu.addAction(QStringLiteral("寄存器配置"), this, [this, groupId]() { showGroupConfig(groupId); });
        menu.addAction(QStringLiteral("查看实时数值"), this, [this, groupId]() { showGroupRealtime(groupId); });
        menu.addSeparator();
        menu.addAction(QStringLiteral("导入 CSV"), this, [this, groupId]() { importGroupCsv(groupId); });
        menu.addAction(QStringLiteral("导出 CSV"), this, [this, groupId]() { exportGroupCsv(groupId); });
        menu.addSeparator();
        menu.addAction(QStringLiteral("编辑分组"), this, [this, groupId]() { editGroup(groupId); });
        menu.addAction(QStringLiteral("删除分组"), this, [this, groupId]() { removeGroup(groupId); });
        menu.exec(globalPos);
    });

    connect(m_canvasView, &GroupCanvasView::canvasClicked, this, [this]()
    {
        m_selectedGroupId.clear();
        m_canvasView->setSelectedGroup(QString());
    });
    connect(m_viewModel, &MainWindowViewModel::runtimeValueChanged, this,
            [this](const QString &pointId)
    {
        m_canvasView->updateRuntimeValue(m_viewModel->document(), pointId);
    });
}

void MainWindow::scheduleRefresh()
{
    if (m_refreshScheduled)
    {
        return;
    }
    m_refreshScheduled = true;
    QTimer::singleShot(0, this, [this]()
    {
        m_refreshScheduled = false;
        refreshDocument();
    });
}

void MainWindow::refreshDocument()
{
    const ProjectDocument &document = m_viewModel->document();
    for (const ConnectionPort &port : document.ports)
    {
        m_portStates.insert(port.id, m_viewModel->portState(port.id));
    }
    m_portListView->setModel(document.ports, document.groups, m_portStates);
    m_canvasView->setModel(document, m_portStates);
    updateGroupCount(document.groups.size());
    updatePortCount(document.ports.size());
    syncCommMonitorPorts();
    if (!m_selectedGroupId.isEmpty())
    {
        m_canvasView->setSelectedGroup(m_selectedGroupId);
    }
    refreshStatus();
}

void MainWindow::refreshStatus()
{
    const ProjectDocument &document = m_viewModel->document();
    m_statusView->updateStatus(document, m_viewModel->isDirty(), m_portStates);
    setWindowTitle(QStringLiteral("%1%2 - Modbus 配置工具")
                   .arg(document.project.name, m_viewModel->isDirty() ? QStringLiteral(" *") : QString()));
}

void MainWindow::updateGroupCount(int count)
{
    if (m_groupCountBadge)
    {
        m_groupCountBadge->setText(QStringLiteral("%1 分组").arg(count));
    }
}

void MainWindow::updatePortCount(int count)
{
    if (m_portCountBadge)
    {
        m_portCountBadge->setText(QStringLiteral("%1 端口").arg(count));
    }
}

bool MainWindow::confirmDiscardChanges()
{
    if (!m_viewModel->isDirty()) { return true; }
    const QMessageBox::StandardButton answer = QMessageBox::warning(this, QStringLiteral("未保存修改"), QStringLiteral("当前工程存在未保存修改。"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) { return saveProject(false); }
    return answer == QMessageBox::Discard;
}

void MainWindow::newProject() { if (confirmDiscardChanges()) { m_viewModel->newProject(); } }

void MainWindow::tryOpenStartupProject()
{
    auto openAndAnnounce = [this](const QString &path, const QString &sourceTag) -> bool {
        const OperationResult result = m_viewModel->openProject(path);
        if (result.success)
        {
            if (m_logView)
            {
                m_logView->appendMessage(
                    QStringLiteral("INFO"),
                    QStringLiteral("APP"),
                    QStringLiteral("已自动打开%1：%2").arg(sourceTag, path));
            }
            refreshStatus();
            rebuildRecentMenu();
            return true;
        }

        if (m_logView)
        {
            m_logView->appendMessage(
                QStringLiteral("WARN"),
                QStringLiteral("APP"),
                QStringLiteral("自动打开%1失败：%2（%3）")
                    .arg(sourceTag, result.message, path));
        }
        return false;
    };

    // 1) 优先打开最近工程（按最近使用顺序，跳过已不存在的路径）
    const QStringList recentPaths = m_viewModel->recentFiles();
    for (const QString &recentPath : recentPaths)
    {
        if (!QFileInfo::exists(recentPath))
        {
            m_viewModel->removeRecentFile(recentPath);
            continue;
        }
        if (openAndAnnounce(recentPath, QStringLiteral("最近工程")))
        {
            return;
        }
        // 打不开的最近项从列表移除，继续尝试下一项
        m_viewModel->removeRecentFile(recentPath);
    }

    // 2) 没有可用最近工程时，回退运行目录默认工程
    const QStringList searchDirs = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };
    const QStringList preferredNames = {
        QStringLiteral("ModbusConfigTool.mctproj"),
        QStringLiteral("project.mctproj"),
        QStringLiteral("default.mctproj")
    };

    QString chosenPath;
    QSet<QString> seenDirs;
    for (const QString &dirPath : searchDirs)
    {
        const QString absoluteDir = QDir(dirPath).absolutePath();
        if (seenDirs.contains(absoluteDir))
        {
            continue;
        }
        seenDirs.insert(absoluteDir);

        for (const QString &name : preferredNames)
        {
            const QString candidate = QDir(absoluteDir).filePath(name);
            if (QFileInfo::exists(candidate))
            {
                chosenPath = candidate;
                break;
            }
        }
        if (!chosenPath.isEmpty())
        {
            break;
        }

        const QFileInfoList projects = QDir(absoluteDir).entryInfoList(
            QStringList{QStringLiteral("*.mctproj")},
            QDir::Files | QDir::Readable,
            QDir::Time);
        if (!projects.isEmpty())
        {
            chosenPath = projects.first().absoluteFilePath();
            break;
        }
    }

    if (chosenPath.isEmpty())
    {
        // 3) 仍没有则保持默认空工程
        return;
    }

    openAndAnnounce(chosenPath, QStringLiteral("默认工程"));
}
void MainWindow::openProject()
{
    if (!confirmDiscardChanges()) { return; }
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开工程"), QString(), QStringLiteral("Modbus 工程 (*.mctproj)"));
    if (!path.isEmpty()) { showResult(m_viewModel->openProject(path), QStringLiteral("工程已打开")); }
}

bool MainWindow::saveProject(bool saveAs)
{
    QString path = m_viewModel->filePath();
    if (saveAs || path.isEmpty()) { path = QFileDialog::getSaveFileName(this, QStringLiteral("保存工程"), path, QStringLiteral("Modbus 工程 (*.mctproj)")); }
    if (path.isEmpty()) { return false; }
    if (!path.endsWith(QStringLiteral(".mctproj"), Qt::CaseInsensitive)) { path += QStringLiteral(".mctproj"); }
    const OperationResult result = m_viewModel->saveProject(path); showResult(result, QStringLiteral("工程已保存")); return result.success;
}

void MainWindow::addGroup()
{
    RegisterGroup group;
    group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    group.color = QStringLiteral("#f54e00");
    group.enabled = true;
    group.canvasX = 40;
    group.canvasY = 40;

    GroupEditorDialog dialog(group, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        showResult(m_viewModel->addGroup(dialog.group()), QStringLiteral("分组已新增"));
    }
}

QString MainWindow::uniqueGroupName(const QString &preferredName, const QString &ignoreGroupId) const
{
    QString base = preferredName.trimmed();
    if (base.isEmpty())
    {
        base = QStringLiteral("导入分组");
    }

    const ProjectDocument &doc = m_viewModel->document();
    auto nameExists = [&](const QString &name) {
        for (const RegisterGroup &group : doc.groups)
        {
            if (!ignoreGroupId.isEmpty() && group.id == ignoreGroupId)
            {
                continue;
            }
            if (group.name == name)
            {
                return true;
            }
        }
        return false;
    };

    if (!nameExists(base))
    {
        return base;
    }

    int suffix = 2;
    QString candidate = QStringLiteral("%1_%2").arg(base).arg(suffix);
    while (nameExists(candidate))
    {
        ++suffix;
        candidate = QStringLiteral("%1_%2").arg(base).arg(suffix);
    }
    return candidate;
}

void MainWindow::importGroup()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入分组 CSV"),
        QString(),
        QStringLiteral("CSV 文件 (*.csv);;所有文件 (*.*)"));
    if (path.isEmpty())
    {
        return;
    }

    static const QStringList palette = {
        QStringLiteral("#f54e00"), QStringLiteral("#1f8a65"), QStringLiteral("#2f6feb"),
        QStringLiteral("#c08532"), QStringLiteral("#8b5cf6"), QStringLiteral("#cf2d56")
    };

    const QFileInfo fileInfo(path);
    RegisterGroup group;
    group.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // 分组名称直接取导入文件名（不含扩展名），重名自动加后缀。
    group.name = uniqueGroupName(fileInfo.completeBaseName());
    group.color = palette.at(m_viewModel->document().groups.size() % palette.size());
    group.enabled = true;
    group.isDefault = false;
    group.canvasX = 0;
    group.canvasY = 0;
    group.description = QStringLiteral("由 CSV 导入：%1").arg(fileInfo.fileName());

    const OperationResult result = m_viewModel->importGroupFromCsv(path, group);
    const QString successMessage = result.message.isEmpty()
        ? QStringLiteral("分组已导入")
        : result.message;
    showResult(result, successMessage);
    if (result.success)
    {
        m_selectedGroupId = group.id;
        scheduleRefresh();
    }
}

void MainWindow::addPort()
{
    ConnectionPort port = m_viewModel->makeDefaultPort();
    ConnectionConfigDialog dialog(port, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        showResult(m_viewModel->addPort(dialog.port()), QStringLiteral("端口已新增"));
    }
}

void MainWindow::editPort(const QString &portId)
{
    const ProjectDocument &doc = m_viewModel->document();
    for (const ConnectionPort &port : doc.ports)
    {
        if (port.id == portId)
        {
            ConnectionConfigDialog dialog(port, this);
            if (dialog.exec() == QDialog::Accepted)
            {
                showResult(m_viewModel->updatePort(dialog.port()), QStringLiteral("端口已更新"));
            }
            return;
        }
    }
}

void MainWindow::removePort(const QString &portId)
{
    int bindings = 0;
    for (const RegisterGroup &group : m_viewModel->document().groups)
    {
        if (group.portId == portId)
        {
            ++bindings;
        }
    }
    const QString message = bindings > 0
        ? QStringLiteral("该端口已绑定 %1 个分组。删除后这些分组将解除绑定，确定继续？")
              .arg(bindings)
        : QStringLiteral("确定删除该端口？");
    if (QMessageBox::question(this, QStringLiteral("删除端口"),
                              message)
        == QMessageBox::Yes)
    {
        showResult(m_viewModel->removePort(portId), QStringLiteral("端口已删除"));
    }
}

void MainWindow::startPort(const QString &portId)
{
    m_viewModel->startPort(portId);
}

void MainWindow::stopPort(const QString &portId)
{
    m_viewModel->stopPort(portId);
}

void MainWindow::showGroupRealtime(const QString &groupId)
{
    auto *panel = new GroupRealtimePanel(groupId, m_viewModel->document(), this);
    panel->setAttribute(Qt::WA_DeleteOnClose);
    connect(panel, &GroupRealtimePanel::configureRegistersRequested, this, &MainWindow::showGroupConfig);
    connect(panel, &GroupRealtimePanel::valueWriteRequested, this,
            [this](const QString &pointId, const RegisterValue &value)
    {
        QString protocolKey;
        QString pointName;
        for (const RegisterPoint &point : m_viewModel->document().registers)
        {
            if (point.id == pointId)
            {
                protocolKey = point.protocolKey;
                pointName = point.name;
                break;
            }
        }

        m_viewModel->writePoint(pointId, value);

        if (m_logView)
        {
            const QString keyText = protocolKey.isEmpty()
                ? QStringLiteral("（空）")
                : protocolKey;
            m_logView->appendMessage(
                QStringLiteral("INFO"),
                QStringLiteral("RUNTIME"),
                QStringLiteral("手动写入实时值：协议键=%1，名称=%2，值=%3")
                    .arg(keyText, pointName, value.toDisplayString()));
        }
    });
    connect(panel, &GroupRealtimePanel::bulkValuesWriteRequested, this,
            [this](const QList<QPair<QString, RegisterValue>> &values)
    {
        for (const auto &item : values)
        {
            m_viewModel->writePoint(item.first, item.second);
        }
        if (m_logView)
        {
            m_logView->appendMessage(
                QStringLiteral("INFO"),
                QStringLiteral("RUNTIME"),
                QStringLiteral("随机写入实时值：共 %1 个点位（均在 min/max 范围内）")
                    .arg(values.size()));
        }
    });
    connect(m_viewModel, &MainWindowViewModel::runtimeValueChanged, panel,
            [panel, this](const QString &)
    {
        panel->updateValues(m_viewModel->document());
    });
    panel->show();
}

void MainWindow::showGroupConfig(const QString &groupId)
{
    auto *dialog = new GroupRegisterConfigDialog(groupId, m_viewModel->document(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &GroupRegisterConfigDialog::addRegisterRequested, this, &MainWindow::addRegisterToGroup);
    connect(dialog, &GroupRegisterConfigDialog::editRegisterRequested, this, &MainWindow::editRegister);
    connect(dialog, &GroupRegisterConfigDialog::removeRegistersRequested, this, &MainWindow::removeRegisters);
    connect(dialog, &GroupRegisterConfigDialog::importCsvRequested, this, &MainWindow::importGroupCsv);
    connect(dialog, &GroupRegisterConfigDialog::exportCsvRequested, this, &MainWindow::exportGroupCsv);
    connect(m_viewModel, &MainWindowViewModel::documentChanged, dialog, [this, dialog]()
    {
        dialog->setDocument(m_viewModel->document());
    });
    dialog->show();
}

void MainWindow::editGroup(const QString &groupId)
{
    const ProjectDocument &doc = m_viewModel->document();
    for (const RegisterGroup &group : doc.groups)
    {
        if (group.id == groupId)
        {
            GroupEditorDialog dialog(group, this);
            if (dialog.exec() == QDialog::Accepted)
            {
                showResult(m_viewModel->updateGroup(dialog.group()), QStringLiteral("分组已更新"));
            }
            return;
        }
    }
}

void MainWindow::removeGroup(const QString &groupId)
{
    const RegisterGroup *target = nullptr;
    for (const RegisterGroup &group : m_viewModel->document().groups)
    {
        if (group.id == groupId)
        {
            target = &group;
            break;
        }
    }
    if (!target)
    {
        QMessageBox::warning(this, QStringLiteral("删除分组"), QStringLiteral("分组不存在或已被删除"));
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("删除分组"),
        QStringLiteral("确定删除分组「%1」吗？\n\n分组内寄存器将一并删除。")
            .arg(target->name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    showResult(m_viewModel->removeGroup(groupId, true), QStringLiteral("分组已删除"));
}

void MainWindow::importGroupCsv(const QString &groupId)
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("导入 CSV"),
                                                      QString(), QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    QMessageBox modeBox(this);
    modeBox.setWindowTitle(QStringLiteral("导入模式"));
    modeBox.setIcon(QMessageBox::Question);
    modeBox.setText(QStringLiteral("是否替换分组中的现有寄存器？\n\n"
                                   "「是」= 清空后导入\n"
                                   "「否」= 追加到现有寄存器"));
    modeBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    modeBox.setButtonText(QMessageBox::Yes, QStringLiteral("是"));
    modeBox.setButtonText(QMessageBox::No, QStringLiteral("否"));
    const bool replaceGroup = modeBox.exec() == QMessageBox::Yes;

    // 导入到已有分组时，分组名称同步为 CSV 文件名。
    const QString groupName = uniqueGroupName(QFileInfo(path).completeBaseName(), groupId);
    showResult(m_viewModel->importCsvIntoGroup(groupId, path, replaceGroup, groupName),
               QStringLiteral("CSV 已导入"));
}

void MainWindow::exportGroupCsv(const QString &groupId)
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出 CSV"),
                                                      QString(), QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    showResult(m_viewModel->exportGroupCsv(groupId, path), QStringLiteral("CSV 已导出"));
}

void MainWindow::addRegisterToGroup(const QString &groupId)
{
    RegisterPoint point = ProjectFactory::createRegister(groupId, m_viewModel->nextAddress(groupId));
    RegisterEditorDialog dialog(point, m_viewModel->document().groups, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        showResult(m_viewModel->addRegister(dialog.point()), QStringLiteral("寄存器已新增"));
    }
}

void MainWindow::editRegister(const QString &registerId)
{
    const ProjectDocument &doc = m_viewModel->document();
    for (const RegisterPoint &point : doc.registers)
    {
        if (point.id == registerId)
        {
            RegisterEditorDialog dialog(point, doc.groups, this);
            connect(&dialog, &RegisterEditorDialog::manualWriteRequested, this,
                    [this, registerId](const RegisterValue &value)
            {
                m_viewModel->writePoint(registerId, value);
            });
            if (dialog.exec() == QDialog::Accepted)
            {
                showResult(m_viewModel->updateRegister(dialog.point()), QStringLiteral("寄存器已更新"));
            }
            return;
        }
    }
}

void MainWindow::removeRegisters(const QStringList &registerIds)
{
    if (registerIds.isEmpty()) return;

    if (QMessageBox::question(this, QStringLiteral("删除寄存器"),
                              QStringLiteral("确定删除选中的 %1 条寄存器？").arg(registerIds.size()))
        == QMessageBox::Yes)
    {
        showResult(m_viewModel->removeRegisters(registerIds), QStringLiteral("寄存器已删除"));
    }
}

void MainWindow::showResult(const OperationResult &result, const QString &successMessage)
{
    if (result.success)
    {
        const QString message = result.message.isEmpty() ? successMessage : result.message;
        m_statusView->showMessage(message);
        m_logView->appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"),
                                 result.detail.isEmpty()
                                     ? message
                                     : QStringLiteral("%1（%2）").arg(message, result.detail));
    }
    else
    {
        const QString text = result.detail.isEmpty()
                                 ? result.message
                                 : QStringLiteral("%1\n%2").arg(result.message, result.detail);
        QMessageBox::warning(this, QStringLiteral("操作失败"), text);
        m_logView->appendMessage(QStringLiteral("WARNING"), QStringLiteral("APP"), result.message);
    }
}
void MainWindow::rebuildRecentMenu()
{
    if (!m_recentMenu) { return; }
    m_recentMenu->clear();
    const QStringList paths = m_viewModel->recentFiles();
    if (paths.isEmpty())
    {
        QAction *empty = m_recentMenu->addAction(QStringLiteral("暂无最近工程"));
        empty->setEnabled(false);
        return;
    }
    for (const QString &path : paths)
    {
        QAction *action = m_recentMenu->addAction(path);
        connect(action, &QAction::triggered, this, [this, path]()
        {
            if (confirmDiscardChanges())
            {
                const OperationResult result = m_viewModel->openProject(path);
                showResult(result, QStringLiteral("工程已打开"));
                if (!result.success
                    && QMessageBox::question(this, QStringLiteral("移除最近工程"),
                                             QStringLiteral("是否从最近工程列表移除该路径？"))
                       == QMessageBox::Yes)
                {
                    m_viewModel->removeRecentFile(path);
                }
            }
        });
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmDiscardChanges())
    {
        event->ignore();
        return;
    }
    m_viewModel->stopAllPorts();
    event->accept();
}

void MainWindow::openCommMonitor()
{
    if (!m_commMonitor)
    {
        m_commMonitor = new CommMonitorDialog(this);
        m_commMonitor->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_commMonitor, &QObject::destroyed, this, [this]() {
            m_commMonitor = nullptr;
        });
        connect(m_commMonitor->view(), &CommMonitorView::monitorPortChanged, this,
                [this](const QString &portId)
        {
            setMonitoredPort(portId, false);
        });
        if (m_monitoredPortId.isEmpty() && m_portListView)
        {
            m_monitoredPortId = m_portListView->selectedPortId();
            m_monitoredPortName = portNameById(m_monitoredPortId);
        }
        syncCommMonitorPorts();
        updateCommMonitorPort();
    }
    m_commMonitor->show();
    m_commMonitor->raise();
    m_commMonitor->activateWindow();
}

void MainWindow::setMonitoredPort(const QString &portId)
{
    setMonitoredPort(portId, true);
}

void MainWindow::setMonitoredPort(const QString &portId, bool syncCombo)
{
    const bool changed = (m_monitoredPortId != portId);
    m_monitoredPortId = portId;
    m_monitoredPortName = portNameById(portId);
    if (!m_commMonitor)
    {
        return;
    }
    if (changed)
    {
        m_commMonitor->view()->clearFrames();
    }
    if (syncCombo)
    {
        m_commMonitor->view()->setSelectedPortId(portId);
    }
    updateCommMonitorPort();
}

void MainWindow::syncCommMonitorPorts()
{
    if (!m_commMonitor || !m_viewModel)
    {
        return;
    }

    QVector<QPair<QString, QString>> ports;
    for (const ConnectionPort &port : m_viewModel->document().ports)
    {
        ports.append(qMakePair(port.id, port.name));
    }
    m_commMonitor->view()->setPorts(ports, m_monitoredPortId);
    m_monitoredPortId = m_commMonitor->view()->selectedPortId();
    m_monitoredPortName = portNameById(m_monitoredPortId);
    updateCommMonitorPort();
}

void MainWindow::updateCommMonitorPort()
{
    if (!m_commMonitor)
    {
        return;
    }

    const QString title = m_monitoredPortName.isEmpty()
        ? QStringLiteral("通信监控 - 未选择端口")
        : QStringLiteral("通信监控 - %1").arg(m_monitoredPortName);
    m_commMonitor->setWindowTitle(title);
    m_commMonitor->view()->setHint(m_monitoredPortId.isEmpty()
        ? QStringLiteral("请选择要监控的连接端口")
        : QStringLiteral("仅显示所选端口的收发报文，可在上方下拉切换"));
}

void MainWindow::onCommFrameCaptured(const QString &portId, const CommFrame &frame)
{
    if (!m_commMonitor || !m_commMonitor->isVisible())
    {
        return;
    }
    if (portId != m_monitoredPortId)
    {
        return;
    }
    if (m_commMonitor->view()->isPaused())
    {
        return;
    }
    m_commMonitor->view()->appendFrame(frame);
}

QString MainWindow::portNameById(const QString &portId) const
{
    if (portId.isEmpty() || !m_viewModel)
    {
        return QString();
    }
    for (const ConnectionPort &port : m_viewModel->document().ports)
    {
        if (port.id == portId)
        {
            return port.name;
        }
    }
    return QString();
}
