#include "main_window.h"

#include "Application/Project/project_service.h"
#include "Application/Registers/register_service.h"
#include "Application/Runtime/runtime_service.h"
#include "Domain/Models/project_factory.h"
#include "Infrastructure/Persistence/json_project_repository.h"
#include "Infrastructure/Persistence/csv_register_gateway_impl.h"
#include "Views/Dialogs/connection_config_dialog.h"
#include "Views/Dialogs/batch_edit_dialog.h"
#include "Views/Dialogs/register_editor_dialog.h"
#include "Views/Groups/group_panel_view.h"
#include "Views/Logging/event_log_view.h"
#include "Views/Registers/register_config_view.h"
#include "Views/RuntimeControl/runtime_control_view.h"
#include "Views/RuntimeValues/runtime_value_view.h"
#include "Views/Main/status_bar_view.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_repository = new JsonProjectRepository;
    m_csvGateway = new CsvRegisterGatewayImpl;
    m_projectService = new ProjectService(m_repository, this);
    m_registerService = new RegisterService(m_projectService);
    m_runtimeService = new RuntimeService(this);
    buildWorkspace(); buildMenus(); connectActions(); refreshDocument();
    resize(1440, 900); setMinimumWidth(1100); setWindowTitle(QStringLiteral("Modbus 配置工具"));
}

MainWindow::~MainWindow()
{
    delete m_registerService;
    delete m_csvGateway;
    delete m_repository;
}

void MainWindow::buildMenus()
{
    QMenu *project = menuBar()->addMenu(QStringLiteral("项目"));
    QAction *newAction = project->addAction(QStringLiteral("新建工程")); newAction->setObjectName(QStringLiteral("newProjectAction"));
    QAction *openAction = project->addAction(QStringLiteral("打开工程")); openAction->setObjectName(QStringLiteral("openProjectAction"));
    project->addSeparator();
    QAction *saveAction = project->addAction(QStringLiteral("保存工程")); saveAction->setObjectName(QStringLiteral("saveProjectAction"));
    QAction *saveAsAction = project->addAction(QStringLiteral("工程另存为"));
    project->addSeparator();
    QAction *importCsvAction = project->addAction(QStringLiteral("导入寄存器 CSV"));
    QAction *exportCsvAction = project->addAction(QStringLiteral("导出寄存器 CSV"));
    project->addSeparator(); QAction *closeAction = project->addAction(QStringLiteral("关闭程序"));
    QMenu *organization = menuBar()->addMenu(QStringLiteral("组织"));
    QAction *addGroupAction = organization->addAction(QStringLiteral("新增分组"));
    QAction *removeGroupAction = organization->addAction(QStringLiteral("删除当前分组"));
    QAction *connectionAction = menuBar()->addAction(QStringLiteral("连接配置"));
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);
    connect(saveAction, &QAction::triggered, this, [this]() { saveProject(false); });
    connect(saveAsAction, &QAction::triggered, this, [this]() { saveProject(true); });
    connect(importCsvAction, &QAction::triggered, this, [this]()
    {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("导入 CSV"), QString(), QStringLiteral("CSV 文件 (*.csv)"));
        if (path.isEmpty()) { return; }
        const CsvImportResult imported = m_csvGateway->importFile(path, m_projectService->document());
        if (!imported.result.success) { showResult(imported.result, QString()); return; }
        ProjectDocument &document = m_projectService->editableDocument(); document.groups = imported.groups; document.registers += imported.registers;
        m_projectService->markDirty(); showResult(OperationResult::ok(), QStringLiteral("CSV 已导入"));
    });
    connect(exportCsvAction, &QAction::triggered, this, [this]()
    {
        QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出 CSV"), QString(), QStringLiteral("CSV 文件 (*.csv)"));
        if (path.isEmpty()) { return; } if (!path.endsWith(QStringLiteral(".csv"), Qt::CaseInsensitive)) { path += QStringLiteral(".csv"); }
        showResult(m_csvGateway->exportFile(path, m_projectService->document()), QStringLiteral("CSV 已导出"));
    });
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    connect(addGroupAction, &QAction::triggered, this, &MainWindow::addGroup);
    connect(removeGroupAction, &QAction::triggered, this, &MainWindow::removeGroup);
    connect(connectionAction, &QAction::triggered, m_runtimeControl, &RuntimeControlView::configureRequested);
}

void MainWindow::buildWorkspace()
{
    auto *central = new QWidget(this); auto *root = new QVBoxLayout(central);
    root->setContentsMargins(14, 14, 14, 8); root->setSpacing(8);
    m_runtimeControl = new RuntimeControlView(central);
    auto *horizontal = new QSplitter(Qt::Horizontal, central);
    m_groupPanel = new GroupPanelView(horizontal);
    auto *vertical = new QSplitter(Qt::Vertical, horizontal);
    m_registerView = new RegisterConfigView(vertical);
    m_runtimeValueView = new RuntimeValueView(vertical);
    horizontal->addWidget(m_groupPanel); horizontal->addWidget(vertical);
    horizontal->setSizes({260, 1100}); vertical->setSizes({420, 270});
    m_logView = new EventLogView(central); m_logView->setMaximumHeight(170);
    root->addWidget(m_runtimeControl); root->addWidget(horizontal, 1); root->addWidget(m_logView);
    setCentralWidget(central);
    m_statusView = new StatusBarView(this); statusBar()->addPermanentWidget(m_statusView, 1);
}

void MainWindow::connectActions()
{
    connect(m_projectService, &ProjectService::documentChanged, this, &MainWindow::refreshDocument);
    connect(m_projectService, &ProjectService::dirtyChanged, this, [this](bool) { refreshDocument(); });
    connect(m_groupPanel, &GroupPanelView::groupSelected, this, [this](const QString &, const QString &name)
    {
        const QString filter = name == QStringLiteral("全部分组") ? QString() : name;
        m_registerView->setGroupFilter(filter); m_runtimeValueView->setGroupFilter(filter);
    });
    connect(m_groupPanel, &GroupPanelView::addRequested, this, &MainWindow::addGroup);
    connect(m_groupPanel, &GroupPanelView::removeRequested, this, &MainWindow::removeGroup);
    connect(m_groupPanel, &GroupPanelView::batchEditRequested, this, [this]()
    {
        const QStringList ids = m_registerView->selectedPointIds();
        if (ids.isEmpty()) { QMessageBox::information(this, QStringLiteral("批量编辑"), QStringLiteral("请先在配置表中选择寄存器。")); return; }
        BatchEditDialog dialog(ids.size(), m_projectService->document().groups, this);
        if (dialog.exec() == QDialog::Accepted) { showResult(m_registerService->applyPatch(ids, dialog.patch()), QStringLiteral("批量编辑已应用")); }
    });
    connect(m_registerView, &RegisterConfigView::addRequested, this, &MainWindow::addRegister);
    connect(m_registerView, &RegisterConfigView::editRequested, this, &MainWindow::editRegister);
    connect(m_registerView, &RegisterConfigView::deleteRequested, this, [this](const QStringList &ids)
    {
        if (ids.isEmpty()) { return; }
        if (QMessageBox::question(this, QStringLiteral("删除寄存器"), QStringLiteral("确定删除选中的 %1 条寄存器？").arg(ids.size())) == QMessageBox::Yes)
        { showResult(m_registerService->removeRegisters(ids), QStringLiteral("寄存器已删除")); }
    });
    connect(m_registerView, &RegisterConfigView::enableRequested, this, [this](const QStringList &ids, bool enabled)
    { showResult(m_registerService->setEnabled(ids, enabled), enabled ? QStringLiteral("寄存器已启用") : QStringLiteral("寄存器已停用")); });
    connect(m_runtimeControl, &RuntimeControlView::configureRequested, this, [this]()
    {
        ConnectionConfigDialog dialog(m_projectService->document().serverProfile, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            m_projectService->editableDocument().serverProfile = dialog.profile(); m_projectService->markDirty();
        }
    });
    connect(m_runtimeControl, &RuntimeControlView::startRequested, this, [this]() { m_runtimeService->start(m_projectService->document()); });
    connect(m_runtimeControl, &RuntimeControlView::stopRequested, m_runtimeService, &RuntimeService::stop);
    connect(m_runtimeService, &RuntimeService::stateChanged, this, [this](RuntimeState state)
    {
        m_runtimeControl->setRuntimeState(state); refreshDocument();
        m_logView->appendMessage(QStringLiteral("RUNTIME"), QStringLiteral("MODBUS"), runtimeStateToString(state));
    });
    connect(m_runtimeService, &RuntimeService::errorOccurred, this, [this](const QString &message, const QString &detail)
    { m_logView->appendMessage(QStringLiteral("ERROR"), QStringLiteral("MODBUS"), message + QStringLiteral(": ") + detail); QMessageBox::critical(this, QStringLiteral("运行时错误"), message); });
}

void MainWindow::refreshDocument()
{
    const ProjectDocument &document = m_projectService->document();
    m_groupPanel->setGroups(document.groups, document.registers);
    m_registerView->setDocument(&document); m_runtimeValueView->setDocument(&document);
    m_runtimeControl->setProfile(document.serverProfile); m_runtimeControl->setDirty(m_projectService->isDirty());
    m_statusView->updateStatus(document, m_projectService->isDirty(), m_runtimeService->state());
    setWindowTitle(QStringLiteral("%1%2 - Modbus 配置工具").arg(document.project.name, m_projectService->isDirty() ? QStringLiteral(" *") : QString()));
}

bool MainWindow::confirmDiscardChanges()
{
    if (!m_projectService->isDirty()) { return true; }
    const QMessageBox::StandardButton answer = QMessageBox::warning(this, QStringLiteral("未保存修改"), QStringLiteral("当前工程存在未保存修改。"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) { return saveProject(false); }
    return answer == QMessageBox::Discard;
}

void MainWindow::newProject() { if (confirmDiscardChanges()) { m_projectService->newProject(); } }

void MainWindow::openProject()
{
    if (!confirmDiscardChanges()) { return; }
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("打开工程"), QString(), QStringLiteral("Modbus 工程 (*.mctproj)"));
    if (!path.isEmpty()) { showResult(m_projectService->open(path), QStringLiteral("工程已打开")); }
}

bool MainWindow::saveProject(bool saveAs)
{
    QString path = m_projectService->filePath();
    if (saveAs || path.isEmpty()) { path = QFileDialog::getSaveFileName(this, QStringLiteral("保存工程"), path, QStringLiteral("Modbus 工程 (*.mctproj)")); }
    if (path.isEmpty()) { return false; }
    if (!path.endsWith(QStringLiteral(".mctproj"), Qt::CaseInsensitive)) { path += QStringLiteral(".mctproj"); }
    const OperationResult result = m_projectService->saveAs(path); showResult(result, QStringLiteral("工程已保存")); return result.success;
}

void MainWindow::addGroup()
{
    bool accepted = false; const QString name = QInputDialog::getText(this, QStringLiteral("新增分组"), QStringLiteral("分组名称"), QLineEdit::Normal, QString(), &accepted);
    if (accepted) { showResult(m_registerService->addGroup(name), QStringLiteral("分组已新增")); }
}

void MainWindow::removeGroup()
{
    const QString id = m_groupPanel->selectedGroupId(); if (id.isEmpty()) { return; }
    if (QMessageBox::question(this, QStringLiteral("删除分组"), QStringLiteral("删除分组并将其中寄存器移动到默认分组？")) == QMessageBox::Yes)
    { showResult(m_registerService->removeGroup(id, false), QStringLiteral("分组已删除")); }
}

void MainWindow::addRegister()
{
    QString groupId = m_groupPanel->selectedGroupId(); if (groupId.isEmpty()) { groupId = m_projectService->document().groups.first().id; }
    RegisterPoint point = ProjectFactory::createRegister(groupId, m_registerService->nextAddress(groupId));
    RegisterEditorDialog dialog(point, m_projectService->document().groups, this);
    if (dialog.exec() == QDialog::Accepted) { showResult(m_registerService->addRegister(dialog.point()), QStringLiteral("寄存器已新增")); }
}

void MainWindow::editRegister(const QString &pointId)
{
    if (pointId.isEmpty()) { return; }
    for (const RegisterPoint &point : m_projectService->document().registers)
    {
        if (point.id != pointId) { continue; }
        RegisterEditorDialog dialog(point, m_projectService->document().groups, this);
        if (dialog.exec() == QDialog::Accepted) { showResult(m_registerService->updateRegister(dialog.point()), QStringLiteral("寄存器已更新")); }
        return;
    }
}

void MainWindow::showResult(const OperationResult &result, const QString &successMessage)
{
    if (result.success) { m_statusView->showMessage(successMessage); m_logView->appendMessage(QStringLiteral("INFO"), QStringLiteral("APP"), successMessage); }
    else { QMessageBox::warning(this, QStringLiteral("操作失败"), result.message); m_logView->appendMessage(QStringLiteral("WARNING"), QStringLiteral("APP"), result.message); }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmDiscardChanges()) { event->ignore(); return; }
    m_runtimeService->stop(); event->accept();
}
