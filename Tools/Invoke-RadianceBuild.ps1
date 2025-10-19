# Builds the RadianceSuite SlicerCAT application.
param(
    [string]$BuildDir = "C:\D\RadianceSuite-build",
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Config = "Release",
    [int]$Jobs = 8,
    [string]$Generator = "Visual Studio 17 2022",
    [string]$Architecture = "x64",
    [string]$QtDir = $env:Qt5_DIR,
    [switch]$ConfigureOnly,
    [switch]$BuildOnly,
    [switch]$InnerOnly
)

$ErrorActionPreference = "Stop"

if (-not $QtDir) {
    throw "Qt5_DIR not detected. Pass -QtDir or set the Qt5_DIR environment variable."
}
if (-not (Test-Path $QtDir)) {
    throw "Qt directory not found at '$QtDir'."
}

$resolvedQtDir = (Resolve-Path $QtDir).ProviderPath -replace '\\', '/'
$sourceDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).ProviderPath

if ($Jobs -lt 1) {
    throw "-Jobs must be >= 1."
}

$cachePath = Join-Path $BuildDir "CMakeCache.txt"
$needsConfigure = $true

if (Test-Path $BuildDir) {
    $needsConfigure = -not (Test-Path $cachePath)
} else {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

if ($BuildOnly -and $needsConfigure) {
    throw "Build directory '$BuildDir' has not been configured yet. Omit -BuildOnly for the first run."
}

function Invoke-CMake {
    param(
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Args
    )
    Write-Host ("cmake {0}" -f ($Args -join ' ')) -ForegroundColor Cyan
    & cmake @Args
    if ($LASTEXITCODE -ne 0) {
        throw "cmake exited with code $LASTEXITCODE."
    }
}

$shouldConfigure = $ConfigureOnly -or -not $BuildOnly -or $needsConfigure

if ($shouldConfigure) {
    Invoke-CMake "-S" $sourceDir "-B" $BuildDir "-G" $Generator "-A" $Architecture "-DQt5_DIR=$resolvedQtDir" "-DSlicer_SKIP_ROOT_DIR_MAX_LENGTH_CHECK:BOOL=TRUE"

    if (-not (Test-Path $cachePath)) {
        throw "CMake did not generate '$cachePath'. Check the configuration output above for errors."
    }

    if ($ConfigureOnly) {
        Write-Host "Configure step completed for $BuildDir." -ForegroundColor Green
        return
    }
}

if ($InnerOnly) {
    $innerDir = Join-Path $BuildDir "slicersources-build"
    if (-not (Test-Path $innerDir)) {
        throw "Inner build directory '$innerDir' not found. Run configuration first."
    }
    Invoke-CMake "--build" $innerDir "--config" $Config "--target" "RadianceApp" "--" "/m:$Jobs"
    Write-Host "RadianceApp rebuilt in $innerDir." -ForegroundColor Green
} else {
    Invoke-CMake "--build" $BuildDir "--config" $Config "--" "/m:$Jobs"
    Write-Host "RadianceSuite built in $BuildDir." -ForegroundColor Green
}
