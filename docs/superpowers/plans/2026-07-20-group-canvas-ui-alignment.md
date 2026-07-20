# Group Canvas UI Alignment Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 完整对齐 `docs/ui/group-drag-prototype.html` 的“连接端口 + 分组自由画布”主界面，同时保留现有多端口业务结构。

**Architecture:** 保留 ViewModel、应用服务和领域模型，集中重构主窗口组合、端口列表、滚动画布和分组卡片。端口运行状态仍由 `RuntimeService` 驱动，卡片只发出用户意图，所有持久化修改通过 `MainWindowViewModel` 完成。

**Tech Stack:** Qt 5.14.2、C++17、Qt Widgets、Qt Test、QSS、qmake、MinGW 7.3.0 64-bit

---

## 文件结构

- 修改 `ModbusConfigTool/Tests/Integration/test_main_window.cpp`：覆盖主窗口三栏、工具栏和核心对象名。
- 创建 `ModbusConfigTool/Tests/Unit/test_group_card_widget.cpp`：覆盖卡片控件、状态和信号。
- 创建 `ModbusConfigTool/Tests/Unit/test_connection_port_list_view.cpp`：覆盖端口绑定数和运行状态。
- 修改 `ModbusConfigTool/Tests/Tests.pro`：注册新增测试源文件。
- 修改 `ModbusConfigTool/Source/Views/Main/main_window.{h,cpp}`：工具栏、分栏、信号编排和状态缓存。
- 修改 `ModbusConfigTool/Source/Views/Connection/connection_port_list_view.{h,cpp}`：端口卡片、绑定数和启停状态。
- 修改 `ModbusConfigTool/Source/Views/Groups/group_canvas_view.{h,cpp}`：滚动画布内容范围、卡片复用和交互转发。
- 修改 `ModbusConfigTool/Source/Views/Groups/group_card_widget.{h,cpp}`：组合式卡片、药丸、端口下拉、灰显和 tooltip。
- 修改 `ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.{h,cpp}`：提供按端口查询运行状态。
- 修改 `ModbusConfigTool/Source/Resources/Styles/base.qss`、`controls.qss`：收口主界面静态样式。

## Task 1：建立主窗口结构测试

**Files:**
- Modify: `ModbusConfigTool/Tests/Integration/test_main_window.cpp`

- [ ] **Step 1: 将旧固定仪表盘断言替换为自由画布结构断言**

```cpp
#include "Views/Connection/connection_port_list_view.h"
#include "Views/Groups/group_canvas_view.h"
#include "Views/Logging/event_log_view.h"
#include "Views/Main/main_window.h"
#include "test_registry.h"

#include <QLabel>
#include <QSplitter>
#include <QTest>
#include <QToolBar>

class MainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void containsGroupCanvasWorkspace()
    {
        MainWindow window;
        window.show();
        QTest::qWait(20);

        auto *splitter = window.findChild<QSplitter *>(QStringLiteral("workspaceSplitter"));
        QVERIFY(splitter);
        QCOMPARE(splitter->count(), 3);
        QVERIFY(window.findChild<ConnectionPortListView *>());
        QVERIFY(window.findChild<GroupCanvasView *>());
        QVERIFY(window.findChild<EventLogView *>());
        QVERIFY(window.findChild<QToolBar *>(QStringLiteral("workspaceToolBar")));
        QVERIFY(window.findChild<QLabel *>(QStringLiteral("groupCountBadge")));
        QVERIFY(window.width() >= 1200);
    }
};
```

- [ ] **Step 2: 运行测试并确认旧界面实现无法满足新断言**

Run:

```powershell
Set-Location ModbusConfigTool\Tests
qmake Tests.pro CONFIG+=release
mingw32-make -j4
.\bin\ModbusConfigToolTests.exe containsGroupCanvasWorkspace
```

Expected: FAIL，找不到 `workspaceSplitter` 或 `workspaceToolBar`。

- [ ] **Step 3: 提交测试基线**

```powershell
git add ModbusConfigTool/Tests/Integration/test_main_window.cpp
git commit -m "test(ui): 更新自由画布主窗口结构断言"
```

## Task 2：实现原型三栏与顶部工具栏

**Files:**
- Modify: `ModbusConfigTool/Source/Views/Main/main_window.h`
- Modify: `ModbusConfigTool/Source/Views/Main/main_window.cpp`
- Modify: `ModbusConfigTool/Source/Resources/Styles/base.qss`

- [ ] **Step 1: 在主窗口头文件增加工具栏和状态成员**

```cpp
class QLabel;
class QSplitter;
class QToolBar;

private:
    void buildToolBar();
    void updateGroupCount(int count);

    QToolBar *m_workspaceToolBar = nullptr;
    QLabel *m_groupCountBadge = nullptr;
    QSplitter *m_workspaceSplitter = nullptr;
    QHash<QString, RuntimeState> m_portStates;
```

同时包含 `Application/Runtime/runtime_service.h` 和 `<QHash>`，确保 `RuntimeState` 类型完整。

- [ ] **Step 2: 构建工具栏和三栏 splitter**

在构造函数中按以下顺序调用：

```cpp
buildWorkspace();
buildMenus();
buildToolBar();
connectActions();
```

`buildToolBar()` 创建：

```cpp
m_workspaceToolBar = addToolBar(QStringLiteral("工作区"));
m_workspaceToolBar->setObjectName(QStringLiteral("workspaceToolBar"));
m_workspaceToolBar->setMovable(false);
m_workspaceToolBar->setFloatable(false);

auto *title = new QLabel(QStringLiteral("分组画布"), m_workspaceToolBar);
title->setObjectName(QStringLiteral("workspaceTitle"));
m_workspaceToolBar->addWidget(title);

auto *spacer = new QWidget(m_workspaceToolBar);
spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
m_workspaceToolBar->addWidget(spacer);

m_groupCountBadge = new QLabel(m_workspaceToolBar);
m_groupCountBadge->setObjectName(QStringLiteral("groupCountBadge"));
m_workspaceToolBar->addWidget(m_groupCountBadge);

QAction *addGroupAction = m_workspaceToolBar->addAction(QStringLiteral("＋ 新增分组"));
addGroupAction->setObjectName(QStringLiteral("addGroupToolAction"));
connect(addGroupAction, &QAction::triggered, this, &MainWindow::addGroup);
```

`buildWorkspace()` 使用 `QSplitter(Qt::Horizontal)`，对象名为 `workspaceSplitter`，依次加入端口栏、画布和日志栏，并设置：

```cpp
m_workspaceSplitter->setChildrenCollapsible(false);
m_workspaceSplitter->setStretchFactor(0, 0);
m_workspaceSplitter->setStretchFactor(1, 1);
m_workspaceSplitter->setStretchFactor(2, 0);
m_workspaceSplitter->setSizes({250, 1050, 300});
```

- [ ] **Step 3: 文档刷新时同步数量和三栏数据**

```cpp
updateGroupCount(document.groups.size());
m_portListView->setModel(document.ports, document.groups, m_portStates);
m_canvasView->setModel(document);
```

`updateGroupCount()` 使用 `QStringLiteral("%1 分组").arg(count)`。

- [ ] **Step 4: 将主窗口静态样式收口到 QSS**

在 `base.qss` 增加：

```css
QToolBar#workspaceToolBar {
    min-height: 48px;
    padding: 0 14px;
    spacing: 10px;
    background: #f7f7f4;
    border: 0;
    border-bottom: 1px solid rgba(38, 37, 30, 24);
}
QLabel#workspaceTitle {
    font: 600 17px "Bahnschrift", "Segoe UI";
    background: transparent;
}
QLabel#groupCountBadge {
    padding: 4px 9px;
    border-radius: 10px;
    color: #f54e00;
    background: #f3dfd4;
}
QSplitter#workspaceSplitter::handle {
    width: 1px;
    background: rgba(38, 37, 30, 24);
}
```

- [ ] **Step 5: 运行主窗口测试**

Run: `ModbusConfigTool\Tests\bin\ModbusConfigToolTests.exe containsGroupCanvasWorkspace`

Expected: PASS。

- [ ] **Step 6: 提交主窗口结构**

```powershell
git add ModbusConfigTool/Source/Views/Main/main_window.* ModbusConfigTool/Source/Resources/Styles/base.qss
git commit -m "feat(ui): 对齐自由画布主窗口结构"
```

## Task 3：完善端口卡片状态和绑定数量

**Files:**
- Create: `ModbusConfigTool/Tests/Unit/test_connection_port_list_view.cpp`
- Modify: `ModbusConfigTool/Tests/Tests.pro`
- Modify: `ModbusConfigTool/Source/Views/Connection/connection_port_list_view.h`
- Modify: `ModbusConfigTool/Source/Views/Connection/connection_port_list_view.cpp`
- Modify: `ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.h`
- Modify: `ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.cpp`
- Modify: `ModbusConfigTool/Source/Views/Main/main_window.cpp`

- [ ] **Step 1: 新增端口栏失败测试**

测试构造一个端口和两个绑定分组，调用：

```cpp
QHash<QString, RuntimeState> states;
states.insert(port.id, RuntimeState::Running);
view.setModel({port}, {groupOne, groupTwo}, states);

QCOMPARE(view.findChild<QLabel *>(QStringLiteral("bindingCount_") + port.id)->text(),
         QStringLiteral("绑定 2 分组"));
QCOMPARE(view.findChild<QPushButton *>(QStringLiteral("togglePort_") + port.id)->text(),
         QStringLiteral("断开"));
```

再使用 `QSignalSpy` 点击按钮，验证 Running 发出 `stopPortRequested(port.id)`，Idle 发出 `startPortRequested(port.id)`。

- [ ] **Step 2: 将测试加入 Tests.pro 并运行确认失败**

在 `SOURCES` 加入：

```qmake
$$PWD/Unit/test_connection_port_list_view.cpp \
```

Expected: 编译失败或断言失败，因为当前接口没有 groups/states，且没有停止信号。

- [ ] **Step 3: 调整端口栏接口**

```cpp
void setModel(const QList<ConnectionPort> &ports,
              const QList<RegisterGroup> &groups,
              const QHash<QString, RuntimeState> &states);

signals:
    void startPortRequested(const QString &portId);
    void stopPortRequested(const QString &portId);
```

为卡片关键控件设置对象名：`portCard_<id>`、`portState_<id>`、`bindingCount_<id>`、`togglePort_<id>`、`editPort_<id>`、`deletePort_<id>`。

- [ ] **Step 4: 按运行状态渲染按钮**

使用统一映射：

```cpp
const bool running = state == RuntimeState::Running;
const bool transitioning = state == RuntimeState::Starting
                         || state == RuntimeState::Stopping;
toggleButton->setText(running ? QStringLiteral("断开") : QStringLiteral("连接"));
toggleButton->setEnabled(!transitioning);
```

点击时基于卡片保存的 `RuntimeState` 发出 start 或 stop 信号，不根据按钮文本判断。

- [ ] **Step 5: 补齐 ViewModel 状态查询并连接停止信号**

```cpp
RuntimeState MainWindowViewModel::portState(const QString &portId) const
{
    return m_runtimeService->portState(portId);
}
```

在 `MainWindow` 中缓存 `runtimeStateChanged`，刷新对应端口状态；连接 `stopPortRequested` 到 `stopPort()`。

- [ ] **Step 6: 运行端口栏测试**

Run: `ModbusConfigTool\Tests\bin\ModbusConfigToolTests.exe ConnectionPortListViewTest`

Expected: PASS。

- [ ] **Step 7: 提交端口栏实现**

```powershell
git add ModbusConfigTool/Tests/Unit/test_connection_port_list_view.cpp ModbusConfigTool/Tests/Tests.pro ModbusConfigTool/Source/Views/Connection ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.* ModbusConfigTool/Source/Views/Main/main_window.cpp
git commit -m "feat(ui): 完善端口卡片状态与绑定统计"
```

## Task 4：将分组卡片重构为可交互组合控件

**Files:**
- Create: `ModbusConfigTool/Tests/Unit/test_group_card_widget.cpp`
- Modify: `ModbusConfigTool/Tests/Tests.pro`
- Modify: `ModbusConfigTool/Source/Views/Groups/group_card_widget.h`
- Modify: `ModbusConfigTool/Source/Views/Groups/group_card_widget.cpp`
- Modify: `ModbusConfigTool/Source/Resources/Styles/controls.qss`

- [ ] **Step 1: 新增卡片结构和信号测试**

构造一个停用、已绑定端口的分组，验证：

```cpp
QVERIFY(card.findChild<QPushButton *>(QStringLiteral("groupEnabledToggle")));
QVERIFY(card.findChild<QComboBox *>(QStringLiteral("groupPortCombo")));
QCOMPARE(card.property("enabled").toBool(), false);
QCOMPARE(card.findChild<QComboBox *>(QStringLiteral("groupPortCombo"))->currentData().toString(),
         QStringLiteral("port-1"));
```

使用 `QSignalSpy` 验证点击药丸发出 `enabledChangeRequested(groupId, true)`，选择端口发出 `portChangeRequested(groupId, portId)`。

- [ ] **Step 2: 注册测试并确认失败**

在 `Tests.pro` 加入 `Unit/test_group_card_widget.cpp`，构建并运行。

Expected: FAIL，当前卡片只有 QPainter 绘制内容。

- [ ] **Step 3: 修改构造函数输入和信号**

```cpp
explicit GroupCardWidget(const RegisterGroup &group,
                         int registerCount,
                         const QList<ConnectionPort> &ports,
                         const QList<RegisterPoint> &points,
                         QWidget *parent = nullptr);

signals:
    void enabledChangeRequested(const QString &groupId, bool enabled);
    void portChangeRequested(const QString &groupId, const QString &portId);
```

- [ ] **Step 4: 用布局控件实现卡片内容**

卡片固定宽度约 210 px，最小高度约 154 px。创建 `groupEnabledToggle`、`groupPortCombo`、名称、描述和数量标签。端口下拉首项为：

```cpp
m_portCombo->addItem(QStringLiteral("未绑定端口"), QString());
```

随后按工程端口列表增加 `端口名（TCP/RTU）`。

- [ ] **Step 5: 隔离子控件与拖动事件**

在卡片事件处理中使用：

```cpp
if (childAt(event->pos()) == m_enabledButton
    || m_enabledButton->isAncestorOf(childAt(event->pos()))
    || childAt(event->pos()) == m_portCombo
    || m_portCombo->isAncestorOf(childAt(event->pos())))
{
    event->accept();
    return;
}
```

只有卡片主体按下并超过 `QApplication::startDragDistance()` 后发出拖动信号。

- [ ] **Step 6: 实现停用灰显和悬停摘要**

设置动态属性 `enabled`、`selected`，停用时通过 QSS 灰显。tooltip 文本包含名称、描述、数量和前三条点位：

```cpp
QStringList values;
for (const RegisterPoint &point : points.mid(0, 3))
{
    values.append(QStringLiteral("%1：%2").arg(point.name, point.currentValue.toDisplayString()));
}
setToolTip(summary + QStringLiteral("\n") + values.join(QStringLiteral("\n")));
```

显示值统一调用 `point.currentValue.toDisplayString(point.precision)`，与现有运行值表保持一致。

- [ ] **Step 7: 增加 QSS**

在 `controls.qss` 增加 `QWidget#groupCard`、`QPushButton#groupEnabledToggle`、`QComboBox#groupPortCombo` 的默认、选中、停用和 hover 状态。动态属性变化后调用 `style()->unpolish(this)` / `polish(this)`。

- [ ] **Step 8: 运行卡片测试**

Run: `ModbusConfigTool\Tests\bin\ModbusConfigToolTests.exe GroupCardWidgetTest`

Expected: PASS。

- [ ] **Step 9: 提交分组卡片实现**

```powershell
git add ModbusConfigTool/Tests/Unit/test_group_card_widget.cpp ModbusConfigTool/Tests/Tests.pro ModbusConfigTool/Source/Views/Groups/group_card_widget.* ModbusConfigTool/Source/Resources/Styles/controls.qss
git commit -m "feat(ui): 实现可交互分组卡片"
```

## Task 5：实现可滚动画布与卡片操作闭环

**Files:**
- Modify: `ModbusConfigTool/Source/Views/Groups/group_canvas_view.h`
- Modify: `ModbusConfigTool/Source/Views/Groups/group_canvas_view.cpp`
- Modify: `ModbusConfigTool/Source/Views/Main/main_window.cpp`
- Modify: `ModbusConfigTool/Tests/Integration/test_main_window.cpp`

- [ ] **Step 1: 增加画布范围和卡片信号集成断言**

在主窗口测试中验证 `GroupCanvasView` 位于 `QScrollArea` 内，滚动区域对象名为 `groupCanvasScrollArea`，并验证有分组时画布最小尺寸覆盖最远卡片坐标加卡片尺寸和 80 px 边距。

- [ ] **Step 2: 运行测试确认失败**

Expected: FAIL，当前画布直接作为 splitter 子控件，且固定最小尺寸。

- [ ] **Step 3: 将画布放入 QScrollArea**

在主窗口中创建：

```cpp
auto *canvasScrollArea = new QScrollArea(central);
canvasScrollArea->setObjectName(QStringLiteral("groupCanvasScrollArea"));
canvasScrollArea->setWidgetResizable(true);
canvasScrollArea->setFrameShape(QFrame::NoFrame);
canvasScrollArea->setWidget(m_canvasView);
```

splitter 中间项改为该滚动区域。

- [ ] **Step 4: GroupCanvasView 根据文档扩展内容范围**

为每张卡片计算 `canvasX + cardWidth + 80`、`canvasY + cardHeight + 80` 的最大值，同时不得小于视口建议尺寸。删除调试 `qDebug()` 输出。

- [ ] **Step 5: 转发卡片启用和端口绑定请求**

`GroupCanvasView` 新增：

```cpp
void groupEnabledChangeRequested(const QString &groupId, bool enabled);
void groupPortChangeRequested(const QString &groupId, const QString &portId);
```

`MainWindow` 分别调用 `setGroupEnabled()` 和 `setGroupPort()`，使用 `showResult()` 处理失败；成功后刷新文档和端口绑定数量。

- [ ] **Step 6: 严格对齐右键菜单和双击入口**

保留双击实时数值；右键菜单顺序固定为设计文档顺序。默认分组删除动作禁用或由业务结果阻止。

- [ ] **Step 7: 运行主窗口和卡片测试**

Run: `ModbusConfigTool\Tests\bin\ModbusConfigToolTests.exe`

Expected: 所有 Qt Test 通过。

- [ ] **Step 8: 提交画布闭环**

```powershell
git add ModbusConfigTool/Source/Views/Groups/group_canvas_view.* ModbusConfigTool/Source/Views/Main/main_window.cpp ModbusConfigTool/Tests/Integration/test_main_window.cpp
git commit -m "feat(ui): 完成自由画布交互闭环"
```

## Task 6：原型视觉复核与统一 qmake 验证

**Files:**
- Modify: `ModbusConfigTool/Source/Resources/Styles/base.qss`
- Modify: `ModbusConfigTool/Source/Resources/Styles/controls.qss`
- Modify: `ModbusConfigTool/Source/Views/Connection/connection_port_list_view.cpp`
- Modify: `ModbusConfigTool/Source/Views/Groups/group_card_widget.cpp`

- [ ] **Step 1: 执行静态质量检查**

Run:

```powershell
git diff --check
rg -n "TODO: 统计绑定分组数|TODO: 需要从 viewModel 获取端口状态|qDebug\(\)" ModbusConfigTool/Source/Views
```

Expected: `git diff --check` 无输出；目标 TODO 和临时调试输出不存在。

- [ ] **Step 2: 统一构建测试工程**

Run:

```powershell
$env:PATH='D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;'+$env:PATH
Set-Location ModbusConfigTool\Tests
qmake Tests.pro CONFIG+=release
mingw32-make -j4
.\bin\ModbusConfigToolTests.exe
```

Expected: qmake 和编译成功，全部 Qt Test 为 PASS。

- [ ] **Step 3: 统一构建主工程**

Run:

```powershell
Set-Location ..
qmake ModbusConfigTool.pro CONFIG+=release
mingw32-make -j4
```

Expected: `bin\ModbusConfigTool.exe` 生成，0 个编译错误。

- [ ] **Step 4: 人工对照原型验收**

启动 `bin\ModbusConfigTool.exe`，逐项检查三栏尺寸、顶部工具栏、端口卡片、绑定数量、启停状态、分组药丸、端口下拉、灰显、拖动、tooltip、双击、右键菜单、空状态和窗口缩放。

- [ ] **Step 5: 仅修正验收发现的界面差异并重新执行 Step 1-3**

不得扩展到策略、CSV 格式、持久化版本或 Modbus worker 行为。

- [ ] **Step 6: 提交最终视觉修正**

```powershell
git add ModbusConfigTool/Source/Resources/Styles ModbusConfigTool/Source/Views ModbusConfigTool/Tests
git commit -m "fix(ui): 完成自由画布原型视觉对齐"
```
