# ModbusConfigTool 构建指南

## 环境要求

- Windows 10/11
- Qt 5.14.2 (mingw73_64)
- MinGW 7.3.0 64-bit
- qmake

## 本机工具路径

```
qmake:         D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin\qmake.exe
mingw32-make:  D:\Qt\Qt5.14.2\Tools\mingw730_64\bin\mingw32-make.exe
g++:           D:\Qt\Qt5.14.2\Tools\mingw730_64\bin\g++.exe
windeployqt:   D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin\windeployqt.exe
```

## 快速开始

### 方法一：使用环境配置脚本（推荐）

```cmd
setup_env.bat
cd ModbusConfigTool
qmake ModbusConfigTool.pro
mingw32-make
```

### 方法二：手动设置 PATH

```powershell
$env:PATH = "D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;$env:PATH"
cd ModbusConfigTool
qmake ModbusConfigTool.pro
mingw32-make
```

## 构建步骤详解

### 1. 配置环境

确保 MinGW 和 Qt 的 bin 目录在 PATH 中：

```cmd
set PATH=D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;%PATH%
```

验证工具可用：

```cmd
qmake --version
g++ --version
```

### 2. 生成 Makefile

```cmd
cd ModbusConfigTool
qmake ModbusConfigTool.pro
```

### 3. 编译

```cmd
mingw32-make
```

Release 版本：

```cmd
mingw32-make release
```

清理：

```cmd
mingw32-make clean
```

### 4. 运行

可执行文件位于：

```
ModbusConfigTool/build/bin/ModbusConfigTool.exe
```

## 运行测试

```cmd
cd Tests
qmake Tests.pro
mingw32-make
build/bin/ModbusConfigToolTests.exe
```

## 发布部署

使用 windeployqt 收集依赖：

```cmd
windeployqt --release --no-translations ModbusConfigTool/build/bin/ModbusConfigTool.exe
```

## 常见问题

### 问题：qmake 报错 "Cannot run compiler 'g++'"

**原因**：MinGW bin 目录不在 PATH 中

**解决**：运行 `setup_env.bat` 或手动添加到 PATH：
```cmd
set PATH=D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;%PATH%
```

### 问题：Git 警告 "LF will be replaced by CRLF"

**原因**：项目配置为 Windows 行尾符 (CRLF)，但某些文件使用 Unix 行尾符 (LF)

**解决**：已通过 `git add --renormalize` 统一处理

## 项目结构

```
ModbusConfigTool/
├── Source/
│   ├── App/              # 应用入口
│   ├── Domain/           # 领域模型
│   ├── Application/      # 应用服务
│   ├── Infrastructure/   # 基础设施
│   ├── ViewModels/       # 视图模型
│   ├── Views/            # UI 视图
│   └── Resources/        # 资源文件（QSS、图标）
├── Tests/
│   ├── Unit/             # 单元测试
│   └── Integration/      # 集成测试
└── build/                # 编译输出（bin/obj/moc/rcc）
```

## 更多信息

- [功能规格](docs/qt5-replica/README.md)
- [复刻追踪矩阵](docs/qt5-replica/09-replication-matrix.md)
