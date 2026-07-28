# ModbusConfigTool

基于 **Qt 5.14.2 / C++17 / qmake** 的 Modbus 从站配置与联调工具。  
用于管理寄存器分组、模拟 TCP Server / RTU Slave，并在联调过程中查看实时数值与通信报文。

工程格式：`.mctproj`（JSON）  
运行平台：Windows 10/11

---

## 功能概览

### 工程管理
- 新建 / 打开 / 保存 / 另存为工程（`*.mctproj`）
- 最近工程列表
- 启动时自动打开最近一次工程；若无则创建默认工程
- 窗口标题随工程名/文件名更新（避免长期显示“未命名工程”）

### 连接端口
- 多端口管理：Modbus TCP Server、Modbus RTU Slave
- 端口启用/停用、参数配置
- 分组可绑定到指定端口
- 运行中支持分组启停与端口绑定变更的**热更新映射**（无需整端重连）

### 分组画布
- 自由画布展示分组卡片（拖动定位）
- 支持 **Ctrl 多选** 后批量拖动
- 新增分组、编辑分组、启用/停用分组
- 分组右键：寄存器配置、实时数值、导入/导出 CSV、编辑、删除等
- **多文件连续导入**分组 CSV；分组名默认取导入文件名

### 寄存器配置
- 按分组维护寄存器点位（地址、类型、协议键、策略等）
- CSV 导入/导出：内容按文件原样显示（空值保持为空，不自动加后缀）
- 寄存器名称允许为空
- **地址冲突规则（分组隔离）**
  - 同组内部地址重叠：报错
  - 跨组：仅当**绑定同一非空端口且同时启用**时检查重叠
  - 未绑定端口的分组不参与跨组冲突检查

### 实时数值
- 双击分组卡片打开实时数值面板
- 显示当前值、策略等信息
- 双击“实时值”列可编辑（按协议键返回类型校验）
- 双击“协议键”可复制到剪贴板
- 双击其它列可跳转寄存器配置
- 支持：
  - 随机当前全部值 / 当前筛选值（在范围内）
  - 重置全部 / 重置当前筛选
- 行选中后可进入对应配置界面

### 策略
- 支持线性、随机、正弦等策略（工程内策略引擎驱动）
- 实时数值界面展示策略字段
- 寄存器编辑中可配置策略参数

### 通信监控
- 菜单「通信监控」或快捷键 `Ctrl+M`
- 查看联调过程中的实际收发报文，便于排查异常码、映射等问题
- 可按端口过滤监控内容

### 事件日志
- 左侧/日志区记录 INFO / WARNING / ERROR 等运行信息
- 支持清除日志

---

## 技术栈

| 项目 | 说明 |
|------|------|
| 语言 | C++17 |
| UI | Qt Widgets |
| 协议 | Qt Serial Bus（Modbus）、Qt Serial Port |
| 构建 | qmake + MinGW 7.3.0 64-bit |
| Qt 版本 | 5.14.2 |

### 本机工具路径（可按本机安装调整）

```text
qmake:        D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin\qmake.exe
mingw32-make: D:\Qt\Qt5.14.2\Tools\mingw730_64\bin\mingw32-make.exe
g++:          D:\Qt\Qt5.14.2\Tools\mingw730_64\bin\g++.exe
windeployqt:  D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin\windeployqt.exe
```

---

## 快速开始

### 1. 配置环境（推荐）

```cmd
setup_env.bat
```

或手动将 MinGW / Qt 的 `bin` 加入 `PATH`：

```powershell
$env:PATH = "D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;$env:PATH"
```

### 2. 编译

```cmd
cd ModbusConfigTool
qmake ModbusConfigTool.pro
mingw32-make
```

Release：

```cmd
mingw32-make release
```

清理：

```cmd
mingw32-make clean
```

### 3. 运行

可执行文件：

```text
ModbusConfigTool/build/bin/ModbusConfigTool.exe
```

> 若链接时报 `Permission denied`，请先关闭正在运行的 `ModbusConfigTool.exe` 再编译。

### 4. 部署

```cmd
windeployqt --release --no-translations ModbusConfigTool/build/bin/ModbusConfigTool.exe
```

更完整的构建说明见：[README_BUILD.md](README_BUILD.md)

---

## 目录结构

```text
ModbusConfigTool/
├── ModbusConfigTool/                 # qmake 工程根目录
│   ├── ModbusConfigTool.pro
│   ├── Source/
│   │   ├── App/                      # 应用入口与组装
│   │   ├── Domain/                   # 领域模型、校验、接口
│   │   ├── Application/              # 工程/寄存器/连接/运行时服务
│   │   ├── Infrastructure/           # Modbus 运行时、CSV/JSON 持久化、策略引擎
│   │   ├── ViewModels/               # 主窗口 ViewModel
│   │   ├── Views/                    # 画布、实时值、监控、对话框等 UI
│   │   └── Resources/                # QSS 与资源
│   ├── Tests/                        # 单元/集成测试（构建产物已忽略）
│   └── build/                        # 编译输出（bin/obj/moc/rcc/ui）
├── docs/                             # 规格、计划、UI 原型
├── setup_env.bat                     # 本机构建环境脚本
├── README.md
├── README_BUILD.md
└── LICENSE                           # Apache-2.0
```

架构分层：**Views → ViewModels → Application Services → Domain ← Infrastructure**

---

## 典型联调流程

1. **新建或打开工程**（`.mctproj`）
2. **新增/配置端口**（TCP 或 RTU），并启用
3. **导入分组 CSV** 或手动新增分组与寄存器
4. 将分组**绑定到端口**并**启用**
5. 主站开始轮询后，在**实时数值**中观察/修改点位
6. 用 **通信监控**（`Ctrl+M`）核对实际报文
7. 如切换启用分组出现异常码，确认是否同端口地址重叠；映射会随启停热更新

### CSV 导入约定
- 分组名称 = 导入文件名（不含扩展名，按实现取文件名）
- 单元格内容按原样导入；空单元格显示为空
- 寄存器名称允许为空
- 仅在“同端口 + 双方启用”时做跨组地址冲突校验

---

## 文档

| 文档 | 说明 |
|------|------|
| [README_BUILD.md](README_BUILD.md) | 构建、测试、部署与常见问题 |
| [docs/qt5-replica/README.md](docs/qt5-replica/README.md) | 功能复刻规格总览 |
| [docs/qt5-replica/01-functional-spec.md](docs/qt5-replica/01-functional-spec.md) | 功能规格 |
| [docs/qt5-replica/04-modbus-runtime.md](docs/qt5-replica/04-modbus-runtime.md) | TCP/RTU 运行时与寄存器映射 |
| [docs/superpowers/specs/2026-07-27-comm-monitor-design.md](docs/superpowers/specs/2026-07-27-comm-monitor-design.md) | 通信监控设计 |
| [docs/ui/current-main-window.html](docs/ui/current-main-window.html) | 主界面参考 |

---

## 开发说明

- 构建产物统一输出到 `ModbusConfigTool/build/`，已由 `.gitignore` 忽略
- `Tests/` 相关编译中间文件也已忽略
- 提交规范建议使用 Conventional Commits（中文摘要），例如：  
  `feat: 完善分组联调、地址隔离与运行时热更新`

---

## 许可证

Apache License 2.0，详见 [LICENSE](LICENSE)。
