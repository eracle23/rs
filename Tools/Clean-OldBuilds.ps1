<#
.SYNOPSIS
  一键清理旧的 Ninja/NMake 构建树与可选安装目录。

.DESCRIPTION
  - 删除可再生的构建目录（不会动源码与工具链）。
  - 可选删除安装聚合目录（rs-install）。
  - 可选取消短盘映射（subst）。
  - 可选移除 Defender 排除（需要管理员）。

.EXAMPLE
  # 默认清理 C:\S 下常见旧目录
  pwsh Tools/Clean-OldBuilds.ps1

.EXAMPLE
  # 额外删除安装目录并取消 S:/R: 映射
  pwsh Tools/Clean-OldBuilds.ps1 -IncludeInstall -DriveLetters S,R

.EXAMPLE
  # 自定义构建根目录
  pwsh Tools/Clean-OldBuilds.ps1 -BuildRoot C:/S2
#>
param(
  [string]$BuildRoot = 'C:/S',
  [switch]$IncludeInstall,
  [string[]]$DriveLetters = @('S','R'),
  [switch]$RemoveDefenderExclusions,
  [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

function Remove-PathSafe {
  param([string]$Path)
  if (-not $Path) { return }
  $p = $Path -replace '\\','/'
  if (-not (Test-Path $p)) { return }
  Write-Host ("Removing: {0}" -f $p) -ForegroundColor Yellow
  if (-not $DryRun) {
    try { Remove-Item -LiteralPath $p -Recurse -Force -ErrorAction Stop }
    catch { Write-Warning ("Failed to remove {0}: {1}" -f $p,$_.Exception.Message) }
  }
}

function Unmap-ShortDrives {
  param([string[]]$Letters)
  foreach ($l in $Letters) {
    $d = $l.TrimEnd(':')
    if (-not $d) { continue }
    Write-Host ("Unmapping {0}:" -f $d) -ForegroundColor Yellow
    if (-not $DryRun) { try { cmd /c ("subst {0}: /D" -f $d) | Out-Null } catch {} }
  }
}

function Remove-DefenderExclusions {
  param([string[]]$Paths)
  try {
    foreach ($p in $Paths) {
      Write-Host ("Removing Defender exclusion: {0}" -f $p) -ForegroundColor Yellow
      if (-not $DryRun) { Remove-MpPreference -ExclusionPath $p -ErrorAction SilentlyContinue }
    }
  } catch { Write-Warning "Remove-MpPreference failed (admin required?)" }
}

$root = ($BuildRoot -replace '\\','/')
$targets = @(
  (Join-Path $root 'win-ninja-dev'),
  (Join-Path $root 'win-ninja-rel'),
  (Join-Path $root 'nmake-dev'),
  (Join-Path $root 'ninja-mc')
)
if ($IncludeInstall) { $targets += (Join-Path $root 'rs-install') }

Write-Host ("Cleaning old build trees under {0}" -f $root) -ForegroundColor Cyan
foreach ($t in $targets) { Remove-PathSafe -Path $t }

if ($DriveLetters -and $DriveLetters.Count -gt 0) { Unmap-ShortDrives -Letters $DriveLetters }

if ($RemoveDefenderExclusions) {
  $ex = @(
    (Join-Path $root 'win-ninja-dev/CMakeFiles'),
    (Join-Path $root 'win-ninja-rel/CMakeFiles'),
    (Join-Path $root 'VTK-build/CMakeFiles')
  )
  Remove-DefenderExclusions -Paths $ex
}

Write-Host 'Done.' -ForegroundColor Green

