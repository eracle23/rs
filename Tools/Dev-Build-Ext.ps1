<#
.SYNOPSIS
  开发构建（包含扩展的 SuperBuild 驱动）。

.DESCRIPTION
  - 统一执行：准备共享 Slicer（可选）→ 强制重新配置（传递扩展清单）→ 构建顶层 SuperBuild 目标（Slicer / package）。
  - 与 Tools/Dev-Build.ps1 的区别：本脚本默认构建顶层 SuperBuild（含扩展），而不是仅在 Slicer-build 下编译 RadianceApp。

.EXAMPLES
  # 1) 首次使用：准备共享 Slicer + 开发构建（包含扩展）
  pwsh Tools/Dev-Build-Ext.ps1 -Action build -UseSharedSlicer -SetupShared -PersistEnv -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 -Jobs 0

  # 2) 仅重新配置（让新扩展生效，不编译）
  pwsh Tools/Dev-Build-Ext.ps1 -Action configure -UseSharedSlicer -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5

  # 3) 发布并打包（包含扩展）
  pwsh Tools/Dev-Build-Ext.ps1 -Action package -UseSharedSlicer -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5

  # 4) 离线/本地模式：跳过抓取，使用本地扩展源（分号分隔，路径用正斜杠）
  $root = (Resolve-Path .).Path -replace '\\','/'
  pwsh Tools/Dev-Build-Ext.ps1 -Action build -UseSharedSlicer -SkipFetchExt -LocalExtDirs "$root/Externals/SlicerTotalSegmentator;$root/Externals/SlicerNNUnet;$root/Externals/SlicerSegmentEditorExtraEffects;$root/Externals/SlicerAirwaySegmentation;$root/Externals/SlicerDcm2nii" -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 -Jobs 0

#>
param(
  [ValidateSet('configure','build','package')]
  [string]$Action = 'build',

  [ValidateSet('win-ninja-dev','win-ninja-rel','vs17-dev')]
  [string]$Preset = 'vs17-dev',

  # Qt5 CMake 根目录（如 C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5）
  [string]$QtCMakeDir,

  # 是否使用共享 Slicer（强烈推荐）
  [switch]$UseSharedSlicer = $true,

  # 共享 Slicer 源码/构建目录
  [string]$SharedSrcDir = 'C:\\W\\Slicer',
  [string]$SharedBinDir = 'C:\\W\\Slicer-build',

  # 准备共享 Slicer（调用 Tools/Setup-SharedSlicer.ps1）
  [switch]$SetupShared,
  [switch]$PersistEnv,

  # 并行度（0=自动）
  [int]$Jobs = 0,

  # 透传给 CMake 的附加参数（数组，每项一个入参）
  [string[]]$ExtraCMakeArgs,

  # 强制重新配置（默认开启，确保扩展清单生效）
  [switch]$ForceConfigure = $true,

  # 关闭仍在运行的应用（避免 LNK1168）
  [switch]$KillRunningApp,

  # 跳过抓取扩展（与本地扩展目录配合）
  [switch]$SkipFetchExt,
  # 本地扩展路径清单（分号分隔，使用正斜杠）
  [string]$LocalExtDirs
  ,
  # 短盘符回退（解决 Windows 路径/响应文件过长问题）
  [switch]$AutoShortDriveSlicer = $true,
  [string]$ShortDriveLetter = 'Q',
  [switch]$KeepShortDriveMapping,

  # New: 自定义构建根目录（默认使用 ..\\..\\RS-build）。
  # 例如：-BuildRoot C:/RS-build2 将使用 C:/RS-build2/win-ninja-dev 或 C:/RS-build2/win-ninja-rel
  [string]$BuildRoot,

  # 构建重试次数（网络波动时有用）
  [int]$BuildRetries = 5
  ,
  # 下载稳定化（Git/SSH/并发）
  [switch]$StabilizeDownloads = $true,
  [switch]$SSHOver443
)

$ErrorActionPreference = 'Stop'

Write-Host "Dev-Build-Ext: $Action (Preset=$Preset, Shared=$UseSharedSlicer)" -ForegroundColor Cyan
if ($BuildRoot -and ($Preset -notlike 'vs17*')) {
  $normBuildRoot = ($BuildRoot -replace "\\","/")
  Write-Host "Using custom build root: $normBuildRoot" -ForegroundColor Yellow
}

function Test-Command {
  param([string]$Name)
  return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

# 规范化 ExtraCMakeArgs：支持以逗号分隔的单字符串（来自 cmd 的常见调用），并去除包裹引号
function Normalize-ExtraArgs {
  param([string[]]$Args)
  $out=@()
  if (-not $Args) { return $out }
  foreach($a in $Args){
    if ($null -eq $a) { continue }
    $parts = $a -split ","
    foreach($p in $parts){
      $t = $p.Trim()
      if ($t.StartsWith('"') -and $t.EndsWith('"') -and $t.Length -ge 2) { $t = $t.Substring(1, $t.Length-2) }
      if ($t.StartsWith("'") -and $t.EndsWith("'") -and $t.Length -ge 2) { $t = $t.Substring(1, $t.Length-2) }
      if ([string]::IsNullOrWhiteSpace($t)) { continue }
      $out += $t
    }
  }
  return $out
}

# 简单短盘符构建：将给定目录映射到指定盘符并运行 ninja
function Invoke-ShortDriveBuild {
  param(
    [string]$TargetDir,
    [int]$Jobs = 0,
    [string]$PreferredLetter = 'Q',
    [switch]$KeepMapping
  )
  if (-not (Test-Path $TargetDir)) { return $false }
  $drive = $PreferredLetter.TrimEnd(':')
  if (-not $drive) { $drive = 'Q' }
  if (Test-Path ($drive + ':\')) {
    # 尝试其他盘符
    foreach ($alt in @('Q','R','P','S','T','U')) {
      if (-not (Test-Path ($alt + ':\'))) { $drive = $alt; break }
    }
  }
  $mapCmd = "subst ${drive}: `"$TargetDir`""
  Write-Host "Mapping ${drive}: to $TargetDir for short-path ninja build..." -ForegroundColor Yellow
  cmd /c $mapCmd | Out-Null
  $ok = Test-Path ("${drive}:/")
  if (-not $ok) { Write-Warning "Failed to map ${drive}: to $TargetDir"; return $false }
  try {
    Push-Location ("${drive}:/")
    if ($Jobs -gt 0) { ninja -j $Jobs } else { ninja }
    if ($LASTEXITCODE -ne 0) { return $false }
  } finally {
    Pop-Location -ErrorAction SilentlyContinue
    if (-not $KeepMapping) {
      Write-Host "Unmapping ${drive}:" -ForegroundColor Yellow
      cmd /c ("subst ${drive}: /D") | Out-Null
    }
  }
  return $true
}

function Import-VSDevEnvironment {
  try {
    $vswhere = Join-Path "$Env:ProgramFiles(x86)" 'Microsoft Visual Studio\\Installer\\vswhere.exe'
    if (-not (Test-Path $vswhere)) { return }
    $instPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $instPath) { return }
    $vsdev = Join-Path $instPath 'Common7\\Tools\\VsDevCmd.bat'
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

# 1) 导入 VS 开发环境，确保 cl/link/SDK 可用
Import-VSDevEnvironment

# 2) 设置 Qt5 CMake 路径（或使用已有 QT5_DIR）
if ($QtCMakeDir) {
  if (-not (Test-Path $QtCMakeDir)) { throw "QtCMakeDir 不存在: $QtCMakeDir" }
  $env:QT5_DIR = ($QtCMakeDir -replace "\\","/")
  Write-Host "Using QT5_DIR=$env:QT5_DIR" -ForegroundColor Yellow
} elseif (-not $env:QT5_DIR) {
  Write-Warning "QT5_DIR 未设置，建议指定 -QtCMakeDir 或预设环境变量 QT5_DIR"
}

# 3) 共享 Slicer（可选但推荐）
if ($UseSharedSlicer) {
  $needSetup = $false
  if (-not $env:SLICER_SRC_DIR -or -not $env:SLICER_BIN_DIR) { $needSetup = $true }
  if (-not (Test-Path $SharedSrcDir) -or -not (Test-Path $SharedBinDir)) { $needSetup = $true }
  if ($SetupShared -or $needSetup) {
    Write-Host "Setting up shared Slicer at SRC=$SharedSrcDir BIN=$SharedBinDir ..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot 'Setup-SharedSlicer.ps1') -SrcDir $SharedSrcDir -BinDir $SharedBinDir -SetEnv:$PersistEnv
  }
  if (Test-Path $SharedSrcDir) { $env:SLICER_SRC_DIR = ($SharedSrcDir -replace "\\","/") }
  if (Test-Path $SharedBinDir) { $env:SLICER_BIN_DIR = ($SharedBinDir -replace "\\","/") }
  Write-Host "SLICER_SRC_DIR=$env:SLICER_SRC_DIR" -ForegroundColor DarkGray
  Write-Host "SLICER_BIN_DIR=$env:SLICER_BIN_DIR" -ForegroundColor DarkGray
}

# 4) 组织 CMake 额外参数（扩展相关）
$cfgArgs = @()
$extraNorm = Normalize-ExtraArgs -Args $ExtraCMakeArgs
if ($extraNorm) { $cfgArgs += $extraNorm }
if ($SkipFetchExt) { $cfgArgs += '-DRS_SKIP_FETCH_EXT=ON' }
if ($LocalExtDirs) {
  # 统一正斜杠
  $norm = ($LocalExtDirs -replace "\\","/")
  $cfgArgs += ("-DSlicer_EXTENSION_SOURCE_DIRS={0}" -f $norm)
}

# 5) 强制重新配置（将扩展清单传入 SuperBuild）
Write-Host "Configuring via Tools/Invoke-RadianceBuild.ps1 ..." -ForegroundColor Green
if ($BuildRoot -and ($Preset -notlike 'vs17*')) {
  & (Join-Path $PSScriptRoot 'Invoke-RadianceBuild.ps1') -Preset $Preset -UseSharedSlicer:$UseSharedSlicer -ForceConfigure:$ForceConfigure -ConfigureOnly -QtDir $QtCMakeDir -ExtraCMakeArgs $cfgArgs -BuildRoot $normBuildRoot -BuildRetries $BuildRetries -StabilizeDownloads:$StabilizeDownloads -SSHOver443:$SSHOver443
} else {
  & (Join-Path $PSScriptRoot 'Invoke-RadianceBuild.ps1') -Preset $Preset -UseSharedSlicer:$UseSharedSlicer -ForceConfigure:$ForceConfigure -ConfigureOnly -QtDir $QtCMakeDir -ExtraCMakeArgs $cfgArgs -StabilizeDownloads:$StabilizeDownloads -SSHOver443:$SSHOver443
}

if ($Action -eq 'configure') {
  Write-Host "ConfigureOnly 完成。" -ForegroundColor Cyan
  return
}

# 6) 构建/打包顶层 SuperBuild 目标
if ($KillRunningApp) { Stop-AppLockers -Names @('Alice','AliceApp-real','SlicerDesigner') }

if ($Preset -like 'vs17*') {
  $buildPreset = 'vs17-dev-rel'
} elseif ($Preset -like '*rel*') {
  $buildPreset = if ($UseSharedSlicer) { 'build-rel-shared' } else { 'build-rel' }
} else {
  $buildPreset = if ($UseSharedSlicer) { 'build-dev-shared' } else { 'build-dev' }
}

# 当指定了自定义 BuildRoot 时，直接按路径构建/打包并提前返回，避免使用预设绑定到固定 RS-build。
if ($BuildRoot -and ($Preset -notlike 'vs17*')) {
  $binSub = if ($Preset -like '*rel*') { 'win-ninja-rel' } else { 'win-ninja-dev' }
  $rsConfigDir = $normBuildRoot
  $buildDir = Join-Path $rsConfigDir $binSub
  if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Force -Path $buildDir | Out-Null }
  if ($Action -eq 'package') {
    Write-Host "Packaging in $buildDir ..." -ForegroundColor Green
    if ($Jobs -gt 0) { cmake --build $buildDir --target package -- -j $Jobs | Write-Host } else { cmake --build $buildDir --target package | Write-Host }
    if ($LASTEXITCODE -ne 0) { throw "Packaging failed." }
    Write-Host "Package ???" -ForegroundColor Cyan
    return
  }
  # 默认执行 Slicer 目标
  # 通过 Invoke-RadianceBuild 执行构建，复用内部的缓存清理/策略修复逻辑
  & (Join-Path $PSScriptRoot 'Invoke-RadianceBuild.ps1') -Preset $Preset -UseSharedSlicer:$UseSharedSlicer -QtDir $QtCMakeDir -Jobs $Jobs -ExtraCMakeArgs $cfgArgs -BuildRoot $normBuildRoot -BuildRetries $BuildRetries -StabilizeDownloads:$StabilizeDownloads -SSHOver443:$SSHOver443
  if ($LASTEXITCODE -ne 0) { throw "Build failed." }
  Write-Host "Done." -ForegroundColor Cyan
  return
}

if ($Action -eq 'package') {
  Write-Host "Packaging with preset '$buildPreset' ..." -ForegroundColor Green
  cmake --build --preset $buildPreset --target package | Write-Host
  if ($LASTEXITCODE -ne 0) { throw "Packaging failed." }
  Write-Host "Package 完成。" -ForegroundColor Cyan
  return
}

# 默认构建 Slicer 顶层目标（会驱动扩展构建）
Write-Host "Building with preset '$buildPreset' (target=Slicer, Jobs=$Jobs) ..." -ForegroundColor Green
if ($Jobs -gt 0) {
  cmake --build --preset $buildPreset --target Slicer -- -j $Jobs | Write-Host
} else {
  cmake --build --preset $buildPreset --target Slicer | Write-Host
}

if ($LASTEXITCODE -ne 0 -and ($Preset -notlike 'vs17*')) {
  # 回退：在 Slicer-build 目录使用短盘符 + ninja 直接构建，缓解 D8022/rsp 超长
  if ($AutoShortDriveSlicer) {
    $rsBuildRoot = Join-Path $PSScriptRoot "..\\..\\RS-build"
    $rsBuildRoot = Resolve-Path $rsBuildRoot | Select-Object -ExpandProperty Path
    $binSub = if ($Preset -like '*rel*') { 'win-ninja-rel' } else { 'win-ninja-dev' }
    $rsConfigDir = Join-Path $rsBuildRoot $binSub
    $slicerSubBuild = Join-Path $rsConfigDir 'Slicer-build'
    $fallbackDir = $slicerSubBuild
    # 若共享 Slicer 已配置可用，优先尝试共享构建目录
    if ($UseSharedSlicer -and $env:SLICER_BIN_DIR -and (Test-Path $env:SLICER_BIN_DIR)) {
      $fallbackDir = $env:SLICER_BIN_DIR
    }
    $ok = Invoke-ShortDriveBuild -TargetDir $fallbackDir -Jobs $Jobs -PreferredLetter $ShortDriveLetter -KeepMapping:$KeepShortDriveMapping
    if ($ok) { Write-Host "Short-drive build succeeded." -ForegroundColor Green } else { throw "Build failed." }
  } else {
    throw "Build failed."
  }
}

Write-Host "Done." -ForegroundColor Cyan
