[CmdletBinding()]
param(
    [string]$BuildDir = "$PSScriptRoot/../build",
    [string]$Config = "Release",
    [string]$Target = "Slicer",
    [int]$MaxParallel = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$resolvedBuildDir = Resolve-Path -Path $BuildDir -ErrorAction Stop

$cmakeArgs = @(
    "--build", $resolvedBuildDir.Path,
    "--config", $Config,
    "--target", $Target,
    "--",
    "/m:$MaxParallel"
)

Write-Host "Running: cmake $($cmakeArgs -join ' ')" -ForegroundColor Cyan

& cmake @cmakeArgs
