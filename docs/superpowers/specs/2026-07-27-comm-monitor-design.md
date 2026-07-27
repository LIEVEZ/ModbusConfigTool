# 通信监控窗口设计规格

## 1. 目标

为 ModbusConfigTool 增加独立浮动的**通信监控窗口**，用于在联调排障时查看
当前选中端口上真实的 Modbus 请求/响应交互过程（方向、功能码、地址、数量、
结果与完整 HEX），解决“已连接但不知道有没有交互/回了什么”的问题。

本功能第一期只服务人工排障，不替代事件日志，不做协议深度解码或抓包级分析。

## 2. 已确认需求

| 项 | 选择 |
|---|---|
| 窗口形态 | 独立非模态浮动窗口 |
| 信息密度 | 精简排障版 |
| 抓取范围 | 仅当前左侧选中的连接端口 |
| 打开方式 | 菜单（必选），工具栏按钮可选 |
| 历史策略 | 窗口内最多保留 1000 条，超出丢弃最旧 |

## 3. 非目标（第一期不做）

- 多端口同时抓取与混合显示
- 窗口关闭后仍后台持续抓取
- MBAP 字段逐项拆解、寄存器值业务解码
- 导出 PCAP/CSV/文本
- Wireshark 级底层 TCP 重组可视化
- 修改现有事件日志的职责（事件日志继续只记系统/运行状态）

## 4. 界面设计

### 4.1 入口

- 菜单：`连接配置` → `通信监控`
- 对象名：`commMonitorAction`
- 可选：工具栏按钮与快捷键 `Ctrl+M`（若实现快捷键，需在关于/提示中可见）

### 4.2 窗口

- 类型：`QDialog` 非模态（`Qt::Window` 标志，可最小化/最大化/拖到副屏）
- 对象名：`commMonitorDialog`
- 标题：`通信监控 - <端口显示名>`；无选中端口时为 `通信监控 - 未选择端口`
- 单例行为：重复触发入口时若窗口已存在则 `raise()` / `activateWindow()`，不新建多个实例

### 4.3 顶部工具条

| 控件 | 行为 |
|---|---|
| 端口标签 | 只读，显示当前监控端口名；随主窗口端口选择变化 |
| 暂停/继续 | 暂停后仍可看已有记录，但不再追加；继续后恢复 |
| 清除 | 清空表格 |
| 自动滚动 | 复选框，默认勾选；勾选时新行追加后滚到底部 |

### 4.4 表格列

| 列 | 内容 | 说明 |
|---|---|---|
| 时间 | `HH:mm:ss.zzz` | 本地接收/发送处理时刻 |
| 方向 | `收` / `发` | 收=主站请求；发=本工具响应 |
| 功能码 | 如 `03` `04` `06` `10`，异常时显示原功能码 | 两位十六进制或十进制需统一为两位 HEX 文本 |
| 地址 | 起始寄存器地址 | 写单寄存器也填该地址；无法解析时显示 `-` |
| 数量 | 寄存器数量 | 写单寄存器为 `1`；无法解析时显示 `-` |
| 结果 | `OK` 或 `异常XX` | XX 为两位异常码，如 `异常02` |
| HEX | 完整 PDU 十六进制，字节空格分隔 | 请求行为请求 PDU；响应行为响应 PDU（含异常帧） |

一次成功交互通常连续两行：先 `收` 后 `发`。

### 4.5 空态与提示

- 未选中端口：表格空，顶部提示“请先在左侧选择要监控的连接端口”
- 已选端口但未运行：仍可打开窗口；无帧时提示“端口未运行或尚无报文”
- 切换选中端口：清空当前表格（避免混端口），标题与端口标签更新，继续只显示新端口帧

## 5. 架构与数据流

保持现有 MVVM 边界：View 不直接碰 `QModbusServer`。

```text
BlockModbusTcpServer / BlockModbusRtuSlave
        |  processRequest 前后构造 CommFrame
        v
ModbusRuntimeWorker::frameCaptured(portId? / via RuntimeService bind)
        v
RuntimeService::frameCaptured(portId, frame)
        v
MainWindowViewModel::commFrameCaptured(portId, frame)
        v
MainWindow 过滤 portId == m_selectedPortId 且窗口未暂停
        v
CommMonitorView::appendFrame(frame)
```

说明：

- `portId` 在 `RuntimeService` 连接 worker 时闭包捕获，不要求 worker 知道 portId 字符串也可行；
  推荐由 `RuntimeService` 在转发时附带 portId，worker 只发帧内容。
- 仅**展示层**按当前选中端口过滤；底层运行中的端口仍可照常通信，只是监控窗口不显示其他端口。

## 6. 数据模型

新增轻量结构（放 Domain 或 Application 均可，优先 `Domain/Models` 或 `Infrastructure/Modbus` 旁的共享头，避免 View 依赖 Infrastructure 细节）：

```text
enum class CommDirection { Rx, Tx };

struct CommFrame {
    CommDirection direction;
    QString functionCodeText;   // "03" / "04" ...
    int address = -1;           // -1 表示不可用
    int quantity = -1;          // -1 表示不可用
    bool success = true;
    int exceptionCode = -1;     // 成功为 -1
    QByteArray pduHexSource;    // 原始 PDU 字节，用于生成 HEX 列
    QDateTime timestamp;
};
```

展示层将 `pduHexSource` 格式化为大写/小写一致的空格分隔 HEX（建议大写）。

### 6.1 解析规则

在 `processRequest` 路径：

1. **请求帧（收）**
   - 记录完整 request PDU 字节
   - 从功能码与数据区解析地址/数量（支持 `03/04/06/10`；其他功能码 HEX 仍记录，地址/数量可为 `-`）
   - 结果列对请求帧固定显示 `-` 或空（推荐 `-`），因请求本身无成功失败

2. **响应帧（发）**
   - 记录完整 response PDU 字节
   - 若 `isException()`：结果=`异常XX`，功能码取原始功能码（不含 0x80 显示时去掉异常位，与请求对应）
   - 否则结果=`OK`，地址/数量尽量与请求对齐回填，便于两行对照

3. **时间戳**
   - 使用 `QDateTime::currentDateTime()` 在构造帧时采样

## 7. 运行时埋点位置

文件：`Source/Infrastructure/Modbus/modbus_runtime_worker.cpp`

`BlockModbusTcpServer` / `BlockModbusRtuSlave` 的 `processRequest`：

1. 进入时基于 `request` 构造 Rx `CommFrame` 并 `emit frameCaptured(frame)`
2. 调用 `m_store->processRequest(request)` 得到 `response`
3. 基于 `response`（及请求上下文）构造 Tx `CommFrame` 并 `emit frameCaptured(frame)`
4. 再处理既有 `dataWritten` 逻辑
5. `return response`

`ModbusRuntimeWorker` 增加信号：

```text
void frameCaptured(const CommFrame &frame);
```

`RuntimeService` 在创建 worker 时：

```text
connect(worker, &ModbusRuntimeWorker::frameCaptured, this,
        [this, portId](const CommFrame &frame) {
            emit frameCaptured(portId, frame);
        });
```

需 `qRegisterMetaType<CommFrame>()`，保证跨线程队列连接安全。

## 8. 界面类职责

| 类 | 职责 |
|---|---|
| `CommMonitorView` | 表格、工具条、暂停/清除/自动滚动、append/clear、setPortName |
| `CommMonitorDialog` | 非模态窗口壳、关闭时通知 MainWindow 清空指针 |
| `MainWindow` | 创建/激活窗口、把选中端口变化与 frame 信号接到 View |

文件建议位置：

```text
Source/Views/Monitor/comm_monitor_view.h/.cpp
Source/Views/Monitor/comm_monitor_dialog.h/.cpp
Source/Domain/Models/comm_frame.h   (或 Source/Infrastructure/Modbus/comm_frame.h)
```

样式：复用现有 `base.qss` / `controls.qss` 表格与按钮风格，对象名便于 QSS 定制：

- `commMonitorDialog`
- `commMonitorToolBar`
- `commMonitorTable`
- `commMonitorPauseButton`
- `commMonitorClearButton`

## 9. 与现有模块关系

- **事件日志**：继续记录端口启停、映射诊断、错误；不倾倒每帧 HEX。
- **端口列表选中**：`ConnectionPortListView::portSelected` / MainWindow 现有选中状态是过滤源。
- **分块寄存器存储**：不改变 `ModbusRegisterStore` 业务语义，只在外层 `processRequest` 增加观测。

## 10. 验收标准

1. 启动某端口并选中它，打开通信监控，用 NetAssist/主站发 `03/04` 读请求，窗口出现成对的收/发行，HEX 与结果正确。
2. 异常地址读返回 `异常02` 时，发行结果列为 `异常02`，HEX 含异常 PDU。
3. 切换到另一端口后表格清空，只显示新端口帧。
4. 暂停后不再追加；继续后恢复。
5. 清除后表格为空。
6. 超过 1000 条时最旧记录被移除，界面不明显卡顿。
7. 关闭监控窗口不影响端口继续运行；再次打开为新会话（空表，重新开始）。
8. `qmake` + `mingw32-make` 编译通过；相关新文件不超过工程 800 行/文件约束。

## 11. 实现顺序建议

1. 定义 `CommFrame` + metatype
2. worker `processRequest` 埋点与 `frameCaptured` 信号
3. RuntimeService / ViewModel 转发
4. `CommMonitorView` + `CommMonitorDialog`
5. MainWindow 入口、选中端口联动、过滤与单例窗口
6. 样式与手动联调验收

## 12. 风险与对策

| 风险 | 对策 |
|---|---|
| 高频轮询导致 UI 卡顿 | 限额 1000；仅选中端口显示；后续必要时再做批量刷表 |
| 跨线程信号丢类型 | `Q_DECLARE_METATYPE` + `qRegisterMetaType<CommFrame>()` |
| 解析失败导致无监控 | HEX 必填；解析失败时地址/数量为 `-`，不丢整帧 |
| 与事件日志职责混淆 | 文档与 UI 文案明确：监控=报文，日志=系统事件 |
