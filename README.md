# ModbusConfigTool

Qt 5.14.2、C++、Qt Widgets、MinGW 7.3.0 64-bit 和 qmake 版本的 Modbus 配置工具新工程。

当前仓库已迁入完整复刻规格和实施计划，尚未创建 Qt5 源码工程。

## 文档入口

- [Qt5 完整复刻规格](docs/qt5-replica/README.md)
- [Qt5 实施计划](docs/superpowers/plans/2026-07-17-qt5-modbus-config-tool.md)
- [当前 Python 项目功能盘点](docs/qt5-migration-feature-inventory.md)
- [主界面复制稿](docs/ui/current-main-window.html)

## 目标技术栈

- Windows
- Qt 5.14.2
- C++17
- Qt Widgets
- Qt Serial Bus
- Qt Serial Port
- MinGW 7.3.0 64-bit
- qmake

## 本机工具路径

```text
qmake: D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin\qmake.exe
mingw32-make: D:\Qt\Qt5.14.2\Tools\mingw730_64\bin\mingw32-make.exe
g++: D:\Qt\Qt5.14.2\Tools\mingw730_64\bin\g++.exe
windeployqt: D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin\windeployqt.exe
```

Qt Serial Bus 和 Qt Serial Port 已安装并验证可用。
