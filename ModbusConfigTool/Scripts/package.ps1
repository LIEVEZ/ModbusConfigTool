$projectRoot = Split-Path -Parent $PSScriptRoot
$qtRoot = 'D:\Qt\Qt5.14.2\5.14.2\mingw73_64'
$sourceExe = Join-Path $projectRoot 'bin\ModbusConfigTool.exe'
$destination = Join-Path $projectRoot 'dist\ModbusConfigTool'

if (-not (Test-Path -LiteralPath $sourceExe))
{
    throw "Release executable not found: $sourceExe"
}

New-Item -ItemType Directory -Force -Path $destination | Out-Null
Copy-Item -LiteralPath $sourceExe -Destination $destination -Force

$deployTool = Join-Path $qtRoot 'bin\windeployqt.exe'
$deployArguments = @(
    '--release'
    '--compiler-runtime'
    (Join-Path $destination 'ModbusConfigTool.exe')
)
& $deployTool $deployArguments

if ($LASTEXITCODE -ne 0)
{
    $qtLibraries = @(
        'Qt5Core.dll',
        'Qt5Gui.dll',
        'Qt5Network.dll',
        'Qt5SerialBus.dll',
        'Qt5SerialPort.dll',
        'Qt5Widgets.dll'
    )
    foreach ($library in $qtLibraries)
    {
        Copy-Item -LiteralPath (Join-Path $qtRoot "bin\$library") `
            -Destination $destination -Force
    }

    $compilerRoot = 'D:\Qt\Qt5.14.2\Tools\mingw730_64\bin'
    foreach ($library in @('libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll'))
    {
        Copy-Item -LiteralPath (Join-Path $compilerRoot $library) `
            -Destination $destination -Force
    }

    $platformDirectory = Join-Path $destination 'platforms'
    New-Item -ItemType Directory -Force -Path $platformDirectory | Out-Null
    Copy-Item -LiteralPath (Join-Path $qtRoot 'plugins\platforms\qwindows.dll') `
        -Destination $platformDirectory -Force
}

Write-Output "Package created: $destination"
