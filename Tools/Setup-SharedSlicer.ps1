<#
.SYNOPSIS
  Clone Slicer into a shared location and set env vars for reuse.

.DESCRIPTION
  - Clones Slicer to a short path (default C:\W\Slicer) and checks out a fixed commit.
  - Creates a sibling build dir (default C:\W\Slicer-build).
  - Optionally sets user-level env vars SLICER_SRC_DIR and SLICER_BIN_DIR for presets.

.EXAMPLE
  pwsh -ExecutionPolicy Bypass Tools/Setup-SharedSlicer.ps1 -SetEnv

.EXAMPLE
  pwsh Tools/Setup-SharedSlicer.ps1 -SrcDir C:\Dev\Slicer -BinDir C:\Dev\Slicer-build -SetEnv
#>
param(
  [string]$SrcDir = 'C:\W\Slicer',
  [string]$BinDir = 'C:\W\Slicer-build',
  [string]$GitUrl = 'https://github.com/Slicer/Slicer',
  [string]$Commit = 'c805ba768709f2e469e24af379db0881caac9b25',
  [switch]$SetEnv
)

$ErrorActionPreference = 'Stop'

function Ensure-Dir($path) { if (-not (Test-Path $path)) { New-Item -ItemType Directory -Path $path | Out-Null } }

Write-Host "Setting up shared Slicer..." -ForegroundColor Cyan
Ensure-Dir (Split-Path $SrcDir -Parent)

if (-not (Test-Path $SrcDir)) {
  Write-Host "Cloning $GitUrl to $SrcDir" -ForegroundColor Green
  $cloneOk = $true
  try { git clone $GitUrl $SrcDir } catch { $cloneOk = $false }
  if (-not (Test-Path $SrcDir)) { $cloneOk = $false }
  if (-not $cloneOk) {
    Write-Warning "Standard clone failed; attempting shallow fetch of the target commit over HTTP/1.1"
    git init $SrcDir
    git -C $SrcDir remote add origin $GitUrl
    git -C $SrcDir -c http.version=HTTP/1.1 fetch --depth 1 origin $Commit
    git -C $SrcDir checkout FETCH_HEAD
  }
}

Write-Host "Ensuring commit $Commit is checked out" -ForegroundColor Green
$haveCommit = $false
try { git -C $SrcDir rev-parse --verify $Commit | Out-Null; $haveCommit = $true } catch { $haveCommit = $false }
if (-not $haveCommit) {
  git -C $SrcDir -c http.version=HTTP/1.1 fetch --depth 1 origin $Commit
}
git -C $SrcDir checkout $Commit

Ensure-Dir $BinDir

if ($SetEnv) {
  [Environment]::SetEnvironmentVariable('SLICER_SRC_DIR', $SrcDir, 'User')
  [Environment]::SetEnvironmentVariable('SLICER_BIN_DIR', $BinDir, 'User')
  Write-Host "Set user env: SLICER_SRC_DIR=$SrcDir" -ForegroundColor Yellow
  Write-Host "Set user env: SLICER_BIN_DIR=$BinDir" -ForegroundColor Yellow
  Write-Host "Open a new terminal for env changes to take effect." -ForegroundColor Yellow
}

Write-Host "Done." -ForegroundColor Cyan
