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
#include "Views/Main/status_bar_view.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QUuid>

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

    m_groupCountBadge = new QLabel(m_workspaceToolBar);
    m_groupCountBadge->setObjectName(QStringLiteral("groupCountBadge"));
    m_groupCountBadge->setFixedHeight(32);
    m_groupCountBadge->setAlignment(Qt::AlignCenter);
    m_workspaceToolBar->addWidget(m_groupCountBadge);

    QAction *addGroupAction = m_workspaceToolBar->addAction(QStringLiteral("＋ 新增分组"));
    addGroupAction->setObjectName(QStringLiteral("addGroupToolAction"));
    if (auto *addGroupButton = qobject_cast<QToolButton *>(
            m_workspaceToolBar->widgetForAction(addGroupAction)))
    {
        addGroupButton->setObjectName(QStringLiteral("addGroupToolButton"));
        addGroupButton->setFixedHeight(32);
    }
    connect(addGroupAction, &QAction::triggered, this, &MainWindow::addGroup);
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

    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveAction, &QAction::triggered, this, [this]() { saveProject(false); });
    connect(saveAsAction, &QAction::triggered, this, [this]() { saveProject(true); });
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    connect(addGroupAction, &QAction::triggered, this, &MainWindow::addGroup);

    rebuildRecentMenu();
}

void MainWindow::buildWorkspace()
{
    m_workspaceSplitter = new QSplitter(Qt::Horizontal, this);
    m_workspaceSplitter->setObjectName(QStringLiteral("workspaceSplitter"));
    m_workspaceSplitter->setChildrenCollapsible(false);

    m_portListView = new ConnectionPortListView(m_workspaceSplitter);
    m_portListView->setMinimumWidth(220);
    m_portListView->setMaximumWidth(360);

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
    m_workspaceSplitter->setSizes(QList<int>() << 250 << 1050 << 300);

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
        m_logView->appendMessage(QStringLiteral("RUNTIME"), QStringLiteral("MODBUS"),
                                 QStringLiteral("端口 %1: %2").arg(portId, runtimeStateToString(state)));
    });

    connect(m_viewModel, &MainWindowViewModel::runtimeError, this,
            [this](const QString &portId, const QString &message, const QString &detail)
    {
        m_logView->appendMessage(QStringLiteral("ERROR"), QStringLiteral("MODBUS"),
                                 QStringLiteral("端口 %1: %2 - %3").arg(portId, message, detail));
        QMessageBox::critical(this, QStringLiteral("运行时错误"),
                              QStringLiteral("端口 %1\n%2\n%3").arg(portId, message, detail));
    });

    // Port list view signals
    connect(m_portListView, &ConnectionPortListView::addPortRequested, this, &MainWindow::addPort);
    connect(m_portListView, &ConnectionPortListView::editPortRequested, this, &MainWindow::editPort);
    connect(m_portListView, &ConnectionPortListView::removePortRequested, this, &MainWindow::removePort);
    connect(m_portListView, &ConnectionPortListView::startPortRequested,
            this, &MainWindow::startPort);
    connect(m_portListView, &ConnectionPortListView::stopPortRequested,
            this, &MainWindow::stopPort);

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
        QAction *delAction = menu.addAction(QStringLiteral("删除分组"), this, [this, groupId]() { removeGroup(groupId); });
        for (const RegisterGroup &group : m_viewModel->document().groups)
        {
            if (group.id == groupId)
            {
                delAction->setEnabled(!group.isDefault);
                break;
            }
        }
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
    m_canvasView->setModel(document);
    updateGroupCount(document.groups.size());
    if (!m_selectedGroupId.isEmpty())
    {
        m_canvasView->setSelectedGroup(m_selectedGroupId);
    }
    refreshStatus();
}

void MainWindow::refreshStatus()
{
    const ProjectDocument &document = m_viewModel->document();
    m_statusView->updateStatus(document, m_viewModel->isDirty(), RuntimeState::Idle);
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

bool MainWindow::confirmDiscardChanges()
{
    if (!m_viewModel->isDirty()) { return true; }
    const QMessageBox::StandardButton answer = QMessageBox::warning(this, QStringLiteral("未保存修改"), QStringLiteral("当前工程存在未保存修改。"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) { return saveProject(false); }
    return answer == QMessageBox::Discard;
}

void MainWindow::newProject() { if (confirmDiscardChanges()) { m_viewModel->newProject(); } }

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
    connect(m_viewModel, &MainWindowViewModel::runtimeValueChanged, panel, [panel, this](const QString &)
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
    if (QMessageBox::question(this, QStringLiteral("删除分组"),
                              QStringLiteral("删除分组并将其中寄存器移动到默认分组？"))
        == QMessageBox::Yes)
    {
        showResult(m_viewModel->removeGroup(groupId, false), QStringLiteral("分组已删除"));
    }
}

void MainWindow::importGroupCsv(const QString &groupId)
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("导入 CSV"),
                                                      QString(), QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    const bool replaceGroup = QMessageBox::question(this, QStringLiteral("导入模式"),
                                                    QStringLiteral("是否替换分组中的现有寄存器？\n\n"
                                                                   "「是」= 清空后导入\n"
                                                                   "「否」= 追加到现有寄存器"))
                              == QMessageBox::Yes;

    showResult(m_viewModel->importCsvIntoGroup(groupId, path, replaceGroup), QStringLiteral("CSV 已导入"));
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
    if (result.success) { m_statusView->showMessage(successMessage); m_logView->appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), successMessage); }
    else { QMessageBox::warning(this, QStringLiteral("操作失败"), result.message); m_logView->appendMessage(QStringLiteral("WARNING"), QStringLiteral("APP"), result.message); }
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
