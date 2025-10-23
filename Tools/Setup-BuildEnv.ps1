<#
.SYNOPSIS
  One-time setup of QT5_DIR and SLICER_* environment variables.

.DESCRIPTION
  Sets user-level environment variables:
    - QT5_DIR = <path to Qt5 CMake package dir>
    - SLICER_SRC_DIR = <shared Slicer source dir>
    - SLICER_BIN_DIR = <shared Slicer build dir>
  Validates the provided paths and prints a summary.

.EXAMPLE
  pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5 -SlicerSrc C:\W\Slicer -SlicerBin C:\W\Slicer-build

.EXAMPLE
  pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:\Qt\5.15.2\msvc2019_64\lib\cmake\Qt5
#>
param(
  [Parameter(Mandatory=$true)]
  [string]$QtCMakeDir,

  [string]$SlicerSrc = 'C:\W\Slicer',
  [string]$SlicerBin = 'C:\W\Slicer-build'
)

$ErrorActionPreference = 'Stop'

function Assert-Dir([string]$Path, [string]$Name) {
  if (-not (Test-Path $Path)) { throw "Path not found for ${Name}: $Path" }
}

Write-Host "Configuring user env variables for build..." -ForegroundColor Cyan

Assert-Dir $QtCMakeDir 'Qt5_DIR'
# Normalize to forward slashes for CMake
$qtNorm = $QtCMakeDir -replace "\\","/"
[Environment]::SetEnvironmentVariable('QT5_DIR', $qtNorm, 'User')
Write-Host "Set QT5_DIR = $qtNorm" -ForegroundColor Yellow

if (Test-Path $SlicerSrc) {
  [Environment]::SetEnvironmentVariable('SLICER_SRC_DIR', $SlicerSrc, 'User')
  Write-Host "Set SLICER_SRC_DIR = $SlicerSrc" -ForegroundColor Yellow
} else {
  Write-Warning "SLICER_SRC_DIR not set (path missing): $SlicerSrc"
}

if (Test-Path $SlicerBin) {
  [Environment]::SetEnvironmentVariable('SLICER_BIN_DIR', $SlicerBin, 'User')
  Write-Host "Set SLICER_BIN_DIR = $SlicerBin" -ForegroundColor Yellow
} else {
  Write-Warning "SLICER_BIN_DIR not set (path missing): $SlicerBin"
}

Write-Host "Done. Open a new terminal for changes to take effect." -ForegroundColor Green
