param(
  [ValidateSet('configure','build')]
  [string]$Action = 'build',

  [ValidateSet('win-ninja-dev','win-ninja-rel')]
  [string]$Preset = 'win-ninja-dev',

  # Qt5 CMake 目录（例如 C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5）
  [string]$QtCMakeDir,

  # 使用共享 Slicer（推荐）
  [switch]$UseSharedSlicer = $true,

  # 共享 Slicer 源码/构建目录（不存在时可配合 -SetupShared 创建）
  [string]$SharedSrcDir = 'C:\\W\\Slicer',
  [string]$SharedBinDir = 'C:\\W\\Slicer-build',

  # 如需自动创建共享 Slicer 并设置用户级环境变量
  [switch]$SetupShared,
  [switch]$PersistEnv,

  # 仅编译此应用目标
  [string]$BuildTarget = 'RadianceApp',
  [int]$Jobs = 0,

  # 传递给 CMake 的额外缓存参数（分号分隔）
  [string[]]$ExtraCMakeArgs,

  # 强制重新配置
  [switch]$ForceConfigure = $true,

  # 构建前终止可能占用 DLL 的进程（例如 Alice.exe）
  [switch]$KillRunningApp
)

$ErrorActionPreference = 'Stop'

Write-Host "Dev-Build wrapper: $Action (Preset=$Preset)" -ForegroundColor Cyan

# Import VS dev environment so cl/link/SDK are available in this process
function Import-VSDevEnvironment {
  try {
    $vswhere = Join-Path "$Env:ProgramFiles(x86)" 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { return }
    $instPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $instPath) { return }
    $vsdev = Join-Path $instPath 'Common7\Tools\VsDevCmd.bat'
    if (-not (Test-Path $vsdev)) { return }
    Write-Host "Importing VS dev environment from: $vsdev" -ForegroundColor Yellow
    $envDump = cmd /c "call \"$vsdev\" -arch=x64 >nul && set"
    foreach ($line in $envDump) {
      if ($line -match '^(.*?)=(.*)$') {
        $name=$matches[1]; $val=$matches[2]
        try { [System.Environment]::SetEnvironmentVariable($name,$val,'Process') } catch {}
      }
    }
  } catch {}
}
Import-VSDevEnvironment

# 1) 处理 Qt5 路径
if ($QtCMakeDir) {
  if (-not (Test-Path $QtCMakeDir)) { throw "QtCMakeDir 不存在: $QtCMakeDir" }
  $env:QT5_DIR = ($QtCMakeDir -replace "\\","/")
  Write-Host "Using QT5_DIR=$env:QT5_DIR" -ForegroundColor Yellow
} elseif (-not $env:QT5_DIR) {
  Write-Warning "QT5_DIR 未设置。可通过 -QtCMakeDir 指定，或预先设置环境变量 QT5_DIR。"
}

# 2) 处理共享 Slicer 环境
if ($UseSharedSlicer) {
  $needSetup = $false
  if (-not $env:SLICER_SRC_DIR -or -not $env:SLICER_BIN_DIR) { $needSetup = $true }
  if (-not (Test-Path $SharedSrcDir) -or -not (Test-Path $SharedBinDir)) { $needSetup = $true }

  if ($SetupShared -or $needSetup) {
    Write-Host "Setting up shared Slicer at SRC=$SharedSrcDir BIN=$SharedBinDir ..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot 'Setup-SharedSlicer.ps1') -SrcDir $SharedSrcDir -BinDir $SharedBinDir -SetEnv:$PersistEnv
  }

  # 本进程设置，确保后续构建可用
  if (Test-Path $SharedSrcDir) { $env:SLICER_SRC_DIR = ($SharedSrcDir -replace "\\","/") }
  if (Test-Path $SharedBinDir) { $env:SLICER_BIN_DIR = ($SharedBinDir -replace "\\","/") }
  Write-Host "SLICER_SRC_DIR=$env:SLICER_SRC_DIR" -ForegroundColor DarkGray
  Write-Host "SLICER_BIN_DIR=$env:SLICER_BIN_DIR" -ForegroundColor DarkGray
}

# 3) 先仅执行配置（禁止任何回退/短盘符构建）
$invokeParams = @{ Preset = $Preset; UseSharedSlicer = $true; ConfigureOnly = $true; AutoShortDriveSlicer = $false }
if ($ForceConfigure) { $invokeParams.ForceConfigure = $true }
if ($QtCMakeDir) { $invokeParams.QtDir = $QtCMakeDir }
if ($ExtraCMakeArgs) { $invokeParams.ExtraCMakeArgs = $ExtraCMakeArgs }

Write-Host "Configuring via Tools/Invoke-RadianceBuild.ps1 ..." -ForegroundColor Green
& (Join-Path $PSScriptRoot 'Invoke-RadianceBuild.ps1') @invokeParams

if ($Action -eq 'configure') {
  Write-Host "ConfigureOnly 完成。" -ForegroundColor Cyan
  return
}

# 4) 增量编译应用目标
# 优先直接在应用子构建目录 (…/RS-build/<preset>/Slicer-build) 内编译目标，避免顶层不可见目标
if ($Preset -like '*rel*') { $buildPreset = 'build-rel-shared' } else { $buildPreset = 'build-dev-shared' }

# 解析顶层构建目录（与 Invoke 脚本保持一致）
$rsBuildDir = Join-Path $PSScriptRoot "..\\..\\RS-build"
$rsBuildDir = Resolve-Path $rsBuildDir | Select-Object -ExpandProperty Path
$binSub = if ($Preset -like '*rel*') { 'win-ninja-rel' } else { 'win-ninja-dev' }
$rsConfigDir = Join-Path $rsBuildDir $binSub
$appSubBuild = Join-Path $rsConfigDir 'Slicer-build'

# 可选：构建前终止潜在占用 DLL 的进程，避免 LNK1168
function Stop-AppLockers {
  param([string[]]$Names)
  foreach ($name in $Names) {
    $procs = Get-Process -Name $name -ErrorAction SilentlyContinue
    foreach ($p in $procs) {
      Write-Host ("Stopping process {0} (PID {1})" -f $p.ProcessName, $p.Id) -ForegroundColor Yellow
      try { $null = $p.CloseMainWindow() } catch {}
      Start-Sleep -Milliseconds 300
      try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch {}
    }
  }
}
if ($KillRunningApp) {
  Stop-AppLockers -Names @('Alice','AliceApp-real','SlicerDesigner')
}

# 补充：确保 MSVC/WinSDK 的 INCLUDE/LIB 存在于当前进程（避免 cl 找不到 <type_traits>/<stddef.h>）
try {
  $msvcRoot = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC'
  $latest = Get-ChildItem $msvcRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
  if ($latest) {
    $vcInc = Join-Path $latest.FullName 'include'
    $vcLib = Join-Path $latest.FullName 'lib\\x64'
    if (Test-Path $vcInc -and ($env:INCLUDE -notmatch [regex]::Escape($vcInc))) { $env:INCLUDE = "$vcInc;" + $env:INCLUDE }
    if (Test-Path $vcLib -and ($env:LIB -notmatch [regex]::Escape($vcLib))) { $env:LIB = "$vcLib;" + $env:LIB }
  }
  $sdkRoot = 'C:\\Program Files (x86)\\Windows Kits\\10'
  $libRoot = Join-Path $sdkRoot 'Lib'
  $incRoot = Join-Path $sdkRoot 'Include'
  if (Test-Path $libRoot -and Test-Path $incRoot) {
    $ver = (Get-ChildItem $libRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1).Name
    $umLib  = Join-Path $libRoot  (Join-Path $ver 'um\\x64')
    $ucrtLib= Join-Path $libRoot  (Join-Path $ver 'ucrt\\x64')
    $umInc  = Join-Path $incRoot  (Join-Path $ver 'um')
    $ucrtInc= Join-Path $incRoot  (Join-Path $ver 'ucrt')
    $sharedInc = Join-Path $incRoot (Join-Path $ver 'shared')
    $winrtInc  = Join-Path $incRoot (Join-Path $ver 'winrt')
    foreach ($inc in @($umInc,$ucrtInc,$sharedInc,$winrtInc)) {
      if (Test-Path $inc) {
        if ($env:INCLUDE -notmatch [regex]::Escape($inc)) { Write-Host "Adding INCLUDE: $inc" -ForegroundColor DarkGray; $env:INCLUDE = "$inc;" + $env:INCLUDE }
      }
    }
    foreach ($lib in @($umLib,$ucrtLib)) {
      if (Test-Path $lib) {
        if ($env:LIB -notmatch [regex]::Escape($lib)) { Write-Host "Adding LIB: $lib" -ForegroundColor DarkGray; $env:LIB = "$lib;" + $env:LIB }
      }
    }
  }
} catch {}

if (Test-Path (Join-Path $appSubBuild 'build.ninja')) {
  Write-Host ("INCLUDE={0}" -f $env:INCLUDE) -ForegroundColor DarkGray
  Write-Host ("LIB={0}" -f $env:LIB) -ForegroundColor DarkGray
  Write-Host ("Building target '{0}' in '{1}' (Jobs={2}) ..." -f $BuildTarget, $appSubBuild, $Jobs) -ForegroundColor Green
  if ($Jobs -gt 0) {
    cmake --build $appSubBuild --target $BuildTarget -- -j $Jobs | Write-Host
  } else {
    cmake --build $appSubBuild --target $BuildTarget | Write-Host
  }
} else {
  # 回退到预设（顶层）
  Write-Host ("App sub-build not found, falling back to preset '{0}' ..." -f $buildPreset) -ForegroundColor Yellow
  if ($Jobs -gt 0) {
    cmake --build --preset $buildPreset --target $BuildTarget -- -j $Jobs | Write-Host
  } else {
    cmake --build --preset $buildPreset --target $BuildTarget | Write-Host
  }
}

if ($LASTEXITCODE -ne 0) { throw "Build target '$BuildTarget' failed." }

Write-Host "Done." -ForegroundColor Cyan
