@echo off
REM Qt5 Modbus 配置工具 - 构建环境配置脚本
REM 使用方法: setup_env.bat 然后执行构建命令

echo 正在配置 Qt5 构建环境...

set QT_DIR=D:\Qt\Qt5.14.2\5.14.2\mingw73_64
set MINGW_DIR=D:\Qt\Qt5.14.2\Tools\mingw730_64

set PATH=%MINGW_DIR%\bin;%QT_DIR%\bin;%PATH%

echo.
echo 环境配置完成！
echo.
echo Qt 路径: %QT_DIR%
echo MinGW 路径: %MINGW_DIR%
echo.
echo 可用命令:
echo   qmake --version
echo   g++ --version
echo   cd ModbusConfigTool ^&^& qmake ModbusConfigTool.pro
echo   mingw32-make
echo.
