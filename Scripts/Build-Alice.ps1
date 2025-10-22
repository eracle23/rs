param(
    [ValidateSet("Debug","Release","RelWithDebInfo","MinSizeRel")]
    [string]$Configuration = "Release",
    [string]$SourceDir = (Resolve-Path -Path (Join-Path $PSScriptRoot "..")).ProviderPath,
    [string]$BuildDir = (Join-Path $SourceDir "build")
)

$ErrorActionPreference = "Stop"

Write-Host "==> Configuring Alice Studio build tree" -ForegroundColor Cyan
cmake -S $SourceDir -B $BuildDir

Write-Host "==> Building Alice Studio ($Configuration)" -ForegroundColor Cyan
cmake --build $BuildDir --config $Configuration
