# ModbusConfigTool

Qt 5.14.2、C++17、Qt Widgets、Qt Serial Bus 实现的 Modbus TCP Server / RTU Slave
配置与数据模拟工具。

## 工程结构

- `Source/Domain`：领域模型、值对象和校验。
- `Source/Application`：工程、寄存器和运行时用例。
- `Source/Infrastructure`：JSON、CSV、Modbus 和策略实现。
- `Source/ViewModels`：界面状态与应用命令入口。
- `Source/Views`：按工作区区域拆分的 Qt Widgets View。
- `Tests`：Qt Test 单元和集成测试。

所有 `.h`、`.cpp`、`.ui` 和 `.qss` 文件不得超过 800 行。

## Release 构建

```powershell
$env:PATH='D:\Qt\Qt5.14.2\Tools\mingw730_64\bin;D:\Qt\Qt5.14.2\5.14.2\mingw73_64\bin;'+$env:PATH
New-Item -ItemType Directory -Force .\build-release | Out-Null
Set-Location .\build-release
qmake ..\ModbusConfigTool.pro CONFIG+=release
mingw32-make -j4
```

## 测试

```powershell
New-Item -ItemType Directory -Force .\build-tests | Out-Null
Set-Location .\build-tests
qmake ..\Tests\Tests.pro CONFIG+=release
mingw32-make -j4
..\Tests\bin\ModbusConfigToolTests.exe
```

## 打包

完成 Release 构建后运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package.ps1
```

真实 RTU 串口联调需要可用设备或成对虚拟串口，属于硬件验收步骤。
