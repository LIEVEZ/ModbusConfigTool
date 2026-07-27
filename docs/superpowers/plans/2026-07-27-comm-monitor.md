# 通信监控窗口 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为当前选中的连接端口提供独立浮动通信监控窗口，实时显示 Modbus 收/发 PDU 的精简排障信息。

**Architecture:** 在 `BlockModbusTcpServer` / `BlockModbusRtuSlave` 的 `processRequest` 收发两侧构造 `CommFrame`，经 `ModbusRuntimeWorker` → `RuntimeService` → `MainWindowViewModel` 跨线程转发；`MainWindow` 按当前选中 `portId` 过滤后追加到非模态 `CommMonitorDialog`。View 不直接访问 `QModbusServer`。

**Tech Stack:** Qt 5.14.2、C++17、Qt Widgets、Qt Serial Bus、qmake、MinGW 7.3.0 64-bit

**Spec:** `docs/superpowers/specs/2026-07-27-comm-monitor-design.md`

---

## 文件结构

| 文件 | 职责 |
|---|---|
| Create: `ModbusConfigTool/Source/Domain/Models/comm_frame.h` | `CommDirection`、`CommFrame`、HEX/解析辅助声明 |
| Create: `ModbusConfigTool/Source/Domain/Models/comm_frame.cpp` | PDU→CommFrame 解析与 HEX 格式化 |
| Modify: `ModbusConfigTool/Source/Infrastructure/Modbus/modbus_runtime_worker.h` | `frameCaptured(CommFrame)` 信号 |
| Modify: `ModbusConfigTool/Source/Infrastructure/Modbus/modbus_runtime_worker.cpp` | processRequest 埋点 |
| Modify: `ModbusConfigTool/Source/Application/Runtime/runtime_service.h/.cpp` | 转发 `frameCaptured(portId, frame)` |
| Modify: `ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.h/.cpp` | 转发 `commFrameCaptured` |
| Create: `ModbusConfigTool/Source/Views/Monitor/comm_monitor_view.h/.cpp` | 表格+工具条 |
| Create: `ModbusConfigTool/Source/Views/Monitor/comm_monitor_dialog.h/.cpp` | 非模态单例壳 |
| Modify: `ModbusConfigTool/Source/Views/Main/main_window.h/.cpp` | 菜单入口、选中端口联动、过滤 |
| Modify: `ModbusConfigTool/Source/Resources/Styles/base.qss` 或 `controls.qss` | 监控窗口基础样式 |
| Create: `ModbusConfigTool/Tests/Unit/test_comm_frame.cpp`（若 Tests 工程易接入）或跳过，改为编译+手动验收 | 解析单测 |

> 工程 `SOURCES/HEADERS` 使用 `$$files(..., true)` 自动收录，无需改 `.pro`（Tests 除外）。

---

### Task 1: CommFrame 模型与解析

**Files:**
- Create: `ModbusConfigTool/Source/Domain/Models/comm_frame.h`
- Create: `ModbusConfigTool/Source/Domain/Models/comm_frame.cpp`

- [ ] **Step 1: 新增头文件**

```cpp
#ifndef COMM_FRAME_H
#define COMM_FRAME_H

#include <QByteArray>
#include <QDateTime>
#include <QMetaType>
#include <QModbusPdu>
#include <QString>

enum class CommDirection
{
    Rx,
    Tx
};

struct CommFrame
{
    CommDirection direction = CommDirection::Rx;
    QString functionCodeText;
    int address = -1;
    int quantity = -1;
    bool success = true;
    int exceptionCode = -1;
    QByteArray pduBytes;
    QDateTime timestamp = QDateTime::currentDateTime();
};

namespace CommFrameFactory
{
QString formatHex(const QByteArray &bytes);
QByteArray pduToBytes(const QModbusPdu &pdu);
CommFrame fromRequest(const QModbusPdu &request);
CommFrame fromResponse(const QModbusPdu &response, const CommFrame &requestContext);
}

Q_DECLARE_METATYPE(CommFrame)

#endif
```

- [ ] **Step 2: 实现解析**

```cpp
#include "comm_frame.h"

#include <QModbusExceptionResponse>

namespace
{
QString fcText(QModbusPdu::FunctionCode code)
{
    return QStringLiteral("%1").arg(quint8(code) & 0x7F, 2, 16, QLatin1Char('0')).toUpper();
}

void fillAddressQuantity(const QModbusPdu &pdu, int *address, int *quantity)
{
    *address = -1;
    *quantity = -1;
    const auto fc = pdu.functionCode();
    if (fc == QModbusPdu::ReadHoldingRegisters
        || fc == QModbusPdu::ReadInputRegisters
        || fc == QModbusPdu::WriteMultipleRegisters)
    {
        if (pdu.dataSize() >= 4)
        {
            quint16 addr = 0;
            quint16 qty = 0;
            pdu.decodeData(&addr, &qty);
            *address = int(addr);
            *quantity = int(qty);
        }
    }
    else if (fc == QModbusPdu::WriteSingleRegister)
    {
        if (pdu.dataSize() >= 4)
        {
            quint16 addr = 0;
            quint16 value = 0;
            pdu.decodeData(&addr, &value);
            Q_UNUSED(value);
            *address = int(addr);
            *quantity = 1;
        }
    }
}
}

QString CommFrameFactory::formatHex(const QByteArray &bytes)
{
    QStringList parts;
    parts.reserve(bytes.size());
    for (unsigned char byteValue : bytes)
    {
        parts.append(QStringLiteral("%1").arg(byteValue, 2, 16, QLatin1Char('0')).toUpper());
    }
    return parts.join(QLatin1Char(' '));
}

QByteArray CommFrameFactory::pduToBytes(const QModbusPdu &pdu)
{
    QByteArray bytes;
    bytes.append(char(quint8(pdu.functionCode()) | (pdu.isException() ? 0x80 : 0x00)));
    // 注意：QModbusPdu::functionCode() 已去掉异常位；异常帧需用 isException 还原
    if (pdu.isException())
    {
        bytes[0] = char(quint8(pdu.functionCode()) | 0x80);
    }
    else
    {
        bytes[0] = char(quint8(pdu.functionCode()));
    }
    bytes.append(pdu.data());
    return bytes;
}

CommFrame CommFrameFactory::fromRequest(const QModbusPdu &request)
{
    CommFrame frame;
    frame.direction = CommDirection::Rx;
    frame.timestamp = QDateTime::currentDateTime();
    frame.functionCodeText = fcText(request.functionCode());
    fillAddressQuantity(request, &frame.address, &frame.quantity);
    frame.success = true;
    frame.exceptionCode = -1;
    frame.pduBytes = pduToBytes(request);
    return frame;
}

CommFrame CommFrameFactory::fromResponse(const QModbusPdu &response, const CommFrame &requestContext)
{
    CommFrame frame;
    frame.direction = CommDirection::Tx;
    frame.timestamp = QDateTime::currentDateTime();
    frame.functionCodeText = fcText(response.functionCode());
    frame.pduBytes = pduToBytes(response);
    if (response.isException())
    {
        frame.success = false;
        frame.exceptionCode = int(response.exceptionCode());
        frame.address = requestContext.address;
        frame.quantity = requestContext.quantity;
    }
    else
    {
        frame.success = true;
        frame.exceptionCode = -1;
        frame.address = requestContext.address;
        frame.quantity = requestContext.quantity;
        // 写响应可从自身解析
        if (response.functionCode() == QModbusPdu::WriteSingleRegister
            || response.functionCode() == QModbusPdu::WriteMultipleRegisters)
        {
            fillAddressQuantity(response, &frame.address, &frame.quantity);
        }
    }
    return frame;
}
```

实现时注意：`pduToBytes` 对异常帧 function code 字节必须带 `0x80`。用 `QModbusPdu` 无法直接读原始 code 时，按 `isException()` 合成。

- [ ] **Step 3: 快速静态检查**

确认头文件可被 worker / view 同时 include，无 Widgets 依赖。

- [ ] **Step 4: Commit**

```powershell
git add ModbusConfigTool/Source/Domain/Models/comm_frame.h ModbusConfigTool/Source/Domain/Models/comm_frame.cpp
git commit -m "feat(modbus): 新增 CommFrame 报文模型与 PDU 解析"
```

---

### Task 2: Runtime 埋点与信号转发

**Files:**
- Modify: `ModbusConfigTool/Source/Infrastructure/Modbus/modbus_runtime_worker.h`
- Modify: `ModbusConfigTool/Source/Infrastructure/Modbus/modbus_runtime_worker.cpp`
- Modify: `ModbusConfigTool/Source/Application/Runtime/runtime_service.h`
- Modify: `ModbusConfigTool/Source/Application/Runtime/runtime_service.cpp`
- Modify: `ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.h`
- Modify: `ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.cpp`

- [ ] **Step 1: Worker 增加信号**

在 `modbus_runtime_worker.h`：

```cpp
#include "Domain/Models/comm_frame.h"
// signals:
void frameCaptured(const CommFrame &frame);
```

- [ ] **Step 2: 两个 Server 子类 processRequest 埋点**

伪代码（TCP/RTU 相同）：

```cpp
QModbusResponse processRequest(const QModbusPdu &request) override
{
    const CommFrame rx = CommFrameFactory::fromRequest(request);
    emit m_owner->frameCaptured(rx); // 或 store 旁持有 worker 指针/回调

    const QModbusPdu::FunctionCode functionCode = request.functionCode();
    const QModbusResponse response = m_store->processRequest(request);

    const CommFrame tx = CommFrameFactory::fromResponse(response, rx);
    emit m_owner->frameCaptured(tx);

    // 既有 dataWritten 逻辑...
    return response;
}
```

因 `BlockModbusTcpServer` 是 worker.cpp 内匿名命名空间类，推荐构造时传入 `ModbusRuntimeWorker *owner`，在子类里 `emit` 用不了 owner 的信号，应：

```cpp
if (m_owner) {
    QMetaObject::invokeMethod(m_owner, [owner = m_owner, frame = rx]() {
        emit owner->frameCaptured(frame);
    }, Qt::DirectConnection);
}
```

更干净做法：给 Server 子类一个 `std::function<void(const CommFrame&)> m_onFrame`，在 worker `start()` 里绑定：

```cpp
auto *tcp = new BlockModbusTcpServer(&m_store, this);
tcp->setFrameHandler([this](const CommFrame &frame) { emit frameCaptured(frame); });
```

- [ ] **Step 3: RuntimeService 注册 metatype 并转发**

```cpp
// runtime_service.h signals:
void frameCaptured(const QString &portId, const CommFrame &frame);

// ctor:
qRegisterMetaType<CommFrame>();

// startPort connect:
connect(runtime.worker, &ModbusRuntimeWorker::frameCaptured, this,
        [this, portId](const CommFrame &frame) {
            emit frameCaptured(portId, frame);
        });
```

注意：`startPort` 里 worker 可能已存在（二次启动），**只在创建 worker 时 connect 一次**，避免重复连接。当前代码已是 `if (!runtime.thread)` 内 connect，保持该模式。

- [ ] **Step 4: ViewModel 转发**

```cpp
// main_window_view_model.h signals:
void commFrameCaptured(const QString &portId, const CommFrame &frame);

// ctor:
connect(m_runtimeService, &RuntimeService::frameCaptured,
        this, &MainWindowViewModel::commFrameCaptured);
```

- [ ] **Step 5: Commit**

```powershell
git add ModbusConfigTool/Source/Infrastructure/Modbus/modbus_runtime_worker.* `
  ModbusConfigTool/Source/Application/Runtime/runtime_service.* `
  ModbusConfigTool/Source/ViewModels/Main/main_window_view_model.*
git commit -m "feat(runtime): 上报 Modbus 收发 CommFrame"
```

---

### Task 3: CommMonitorView / Dialog UI

**Files:**
- Create: `ModbusConfigTool/Source/Views/Monitor/comm_monitor_view.h`
- Create: `ModbusConfigTool/Source/Views/Monitor/comm_monitor_view.cpp`
- Create: `ModbusConfigTool/Source/Views/Monitor/comm_monitor_dialog.h`
- Create: `ModbusConfigTool/Source/Views/Monitor/comm_monitor_dialog.cpp`

- [ ] **Step 1: View 接口**

```cpp
class CommMonitorView : public QWidget
{
    Q_OBJECT
public:
    explicit CommMonitorView(QWidget *parent = nullptr);
    void setPortName(const QString &portName);
    void setHint(const QString &text);
    void appendFrame(const CommFrame &frame);
    void clearFrames();
    bool isPaused() const;

private slots:
    void onPauseToggled(bool paused);
    void onClearClicked();

private:
    static const int kMaxRows = 1000;
    QLabel *m_portLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QCheckBox *m_autoScroll = nullptr;
    QTableWidget *m_table = nullptr;
    bool m_paused = false;
};
```

- [ ] **Step 2: 表格列与 append 逻辑**

列：时间、方向、功能码、地址、数量、结果、HEX  
`appendFrame`：

```cpp
if (m_paused) return;
const int row = m_table->rowCount();
m_table->insertRow(row);
// setItem ...
while (m_table->rowCount() > kMaxRows) {
    m_table->removeRow(0);
}
if (m_autoScroll->isChecked()) {
    m_table->scrollToBottom();
}
```

显示映射：
- 方向：`Rx→收`，`Tx→发`
- 地址/数量：`<0` 显示 `-`
- 结果：请求行显示 `-`；响应成功 `OK`；失败 `异常%1`（两位 HEX）
- HEX：`CommFrameFactory::formatHex(frame.pduBytes)`

请求行结果列：规格写“推荐 `-`”。

- [ ] **Step 3: Dialog 壳**

```cpp
class CommMonitorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CommMonitorDialog(QWidget *parent = nullptr);
    CommMonitorView *view() const { return m_view; }

private:
    CommMonitorView *m_view = nullptr;
};
```

构造：

```cpp
setObjectName(QStringLiteral("commMonitorDialog"));
setWindowTitle(QStringLiteral("通信监控 - 未选择端口"));
setWindowFlags(windowFlags() | Qt::Window);
setModal(false);
resize(960, 420);
auto *layout = new QVBoxLayout(this);
m_view = new CommMonitorView(this);
layout->addWidget(m_view);
```

- [ ] **Step 4: Commit**

```powershell
git add ModbusConfigTool/Source/Views/Monitor/
git commit -m "feat(ui): 新增通信监控视图与浮动窗口"
```

---

### Task 4: MainWindow 接入

**Files:**
- Modify: `ModbusConfigTool/Source/Views/Main/main_window.h`
- Modify: `ModbusConfigTool/Source/Views/Main/main_window.cpp`

- [ ] **Step 1: 成员与方法**

```cpp
class CommMonitorDialog;
// ...
void openCommMonitor();
void updateCommMonitorPort();
void onCommFrameCaptured(const QString &portId, const CommFrame &frame);
// ...
CommMonitorDialog *m_commMonitor = nullptr;
// m_selectedPortId 若已有则复用；若选中状态只在 port list，增加缓存
QString m_monitoredPortId;
QString m_monitoredPortName;
```

- [ ] **Step 2: 菜单**

在 `buildMenus()` 的 `连接配置` 菜单：

```cpp
QAction *commMonitorAction = connection->addAction(QStringLiteral("通信监控"));
commMonitorAction->setObjectName(QStringLiteral("commMonitorAction"));
commMonitorAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+M")));
connect(commMonitorAction, &QAction::triggered, this, &MainWindow::openCommMonitor);
```

- [ ] **Step 3: openCommMonitor 单例**

```cpp
void MainWindow::openCommMonitor()
{
    if (!m_commMonitor) {
        m_commMonitor = new CommMonitorDialog(this);
        m_commMonitor->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_commMonitor, &QObject::destroyed, this, [this]() {
            m_commMonitor = nullptr;
        });
        updateCommMonitorPort();
    }
    m_commMonitor->show();
    m_commMonitor->raise();
    m_commMonitor->activateWindow();
}
```

- [ ] **Step 4: 端口选中联动**

在现有 `portSelected` / 刷新选中逻辑处：

```cpp
m_monitoredPortId = portId;
m_monitoredPortName = /* 从 document.ports 找 name */;
if (m_commMonitor) {
    m_commMonitor->view()->clearFrames();
    updateCommMonitorPort();
}
```

`updateCommMonitorPort`：

```cpp
if (!m_commMonitor) return;
const QString title = m_monitoredPortName.isEmpty()
    ? QStringLiteral("通信监控 - 未选择端口")
    : QStringLiteral("通信监控 - %1").arg(m_monitoredPortName);
m_commMonitor->setWindowTitle(title);
m_commMonitor->view()->setPortName(m_monitoredPortName.isEmpty() ? QStringLiteral("未选择") : m_monitoredPortName);
m_commMonitor->view()->setHint(m_monitoredPortId.isEmpty()
    ? QStringLiteral("请先在左侧选择要监控的连接端口")
    : QStringLiteral("仅显示当前选中端口的收发报文"));
```

- [ ] **Step 5: 连接 frame 信号**

在 `connectActions`：

```cpp
connect(m_viewModel, &MainWindowViewModel::commFrameCaptured,
        this, &MainWindow::onCommFrameCaptured);

void MainWindow::onCommFrameCaptured(const QString &portId, const CommFrame &frame)
{
    if (!m_commMonitor || !m_commMonitor->isVisible()) return;
    if (portId != m_monitoredPortId) return;
    if (m_commMonitor->view()->isPaused()) return;
    m_commMonitor->view()->appendFrame(frame);
}
```

规格：窗口打开才“看”；底层可一直 emit，UI 在窗口不存在/不可见时直接 return（省 UI 工作）。关闭后再次打开空表（`WA_DeleteOnClose` 已保证）。

- [ ] **Step 6: Commit**

```powershell
git add ModbusConfigTool/Source/Views/Main/main_window.h ModbusConfigTool/Source/Views/Main/main_window.cpp
git commit -m "feat(ui): 主窗口接入通信监控入口与端口过滤"
```

---

### Task 5: 样式与统一编译验收

**Files:**
- Modify: `ModbusConfigTool/Source/Resources/Styles/base.qss` 或 `controls.qss`

- [ ] **Step 1: 增加最小 QSS**

```css
QDialog#commMonitorDialog {
    background: #f2f1ed;
}
QTableWidget#commMonitorTable {
    background: #ffffff;
    border: 1px solid rgba(38, 37, 30, 22);
    gridline-color: rgba(38, 37, 30, 18);
    font: 12px Consolas, "Cascadia Mono", monospace;
}
QLabel#commMonitorPortLabel {
    font: 600 14px "Segoe UI", "Microsoft YaHei";
}
QLabel#commMonitorHintLabel {
    color: rgba(38, 37, 30, 140);
    font-size: 12px;
}
```

- [ ] **Step 2: 全量编译**

```powershell
$env:PATH = "D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;$env:PATH"
Set-Location ModbusConfigTool
# 若 exe 占用先结束进程
Get-Process ModbusConfigTool -ErrorAction SilentlyContinue | Stop-Process -Force
qmake ModbusConfigTool.pro
mingw32-make -j8
```

Expected: `BUILD_EXIT=0`，生成 `bin/ModbusConfigTool.exe`。

- [ ] **Step 3: 手动验收清单（对照规格 §10）**

1. 选中端口并启动 → 打开通信监控 → 外部主站读寄存器 → 出现收/发行  
2. 非法地址 → 发行结果 `异常02`  
3. 切换端口 → 表格清空  
4. 暂停/继续/清除行为正确  
5. 关窗口不影响通信；再开为空表  
6. Ctrl+M / 菜单可打开，重复打开不产生第二实例  

- [ ] **Step 4: Commit**

```powershell
git add ModbusConfigTool/Source/Resources/Styles/*.qss
git commit -m "style(ui): 通信监控窗口基础样式"
```

---

## Spec 覆盖自检

| 规格项 | 任务 |
|---|---|
| 独立浮动窗口 | Task 3/4 |
| 精简列 | Task 3 |
| 仅当前选中端口 | Task 4 过滤 + 切端口清表 |
| 菜单入口 | Task 4 |
| 暂停/清除/自动滚动 | Task 3 |
| 1000 条上限 | Task 3 `kMaxRows` |
| processRequest 埋点 | Task 2 |
| Runtime/VM 转发 | Task 2 |
| 异常结果展示 | Task 1/3 |
| 关闭不影响运行 | Task 4 DeleteOnClose |
| qmake 编译 | Task 5 |

## 占位符扫描

无 TBD/TODO；类型名统一为 `CommFrame` / `CommFrameFactory` / `frameCaptured` / `commFrameCaptured`。
