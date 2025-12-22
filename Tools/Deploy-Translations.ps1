<#
.SYNOPSIS
    Compile and deploy SlicerLanguageTranslations to Slicer install directory

.DESCRIPTION
    This script compiles .ts translation files to .qm format and copies them
    to Slicer's translations directory. Run after build to enable Chinese UI.

.PARAMETER TranslationsDir
    Path to SlicerLanguageTranslations translations directory
    Default: SlicerLanguageTranslations-main/translations under project root

.PARAMETER OutputDir
    Output directory for compiled .qm files (usually bin/translations under install prefix)
    
.PARAMETER QtDir
    Qt installation directory (to find lrelease tool)
    Default: Auto-detect from environment or common paths

.PARAMETER Languages
    List of languages to compile
    Default: zh-CN

.PARAMETER Components
    List of components to compile
    Default: Slicer, CTK, TotalSegmentator, SlicerIGT, MONAIAuto3DSeg, MONAILabel, SlicerVMTK

.PARAMETER BuildDir
    Slicer build directory (to auto-detect install prefix)
    Default: C:/S/vs-dev/Slicer-build

.EXAMPLE
    .\Deploy-Translations.ps1 -OutputDir "C:\S\rs-install\bin\translations"

.EXAMPLE
    .\Deploy-Translations.ps1 -BuildDir "D:\work\RS\rs-build\Slicer-build"
#>

param(
    [string]$TranslationsDir,
    [string]$OutputDir,
    [string]$QtDir,
    [string]$BuildDir = "C:/S/vs-dev/Slicer-build",
    [string[]]$Languages = @("zh-CN"),
    [string[]]$Components = @("Slicer", "CTK", "TotalSegmentator", "SlicerIGT", "MONAIAuto3DSeg", "MONAILabel", "SlicerVMTK")
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir

# Set default translations directory
if (-not $TranslationsDir) {
    $TranslationsDir = Join-Path $ProjectRoot "SlicerLanguageTranslations-main\translations"
}

if (-not (Test-Path $TranslationsDir)) {
    Write-Error "Translations directory not found: $TranslationsDir"
    exit 1
}

# Find lrelease tool
function Find-Lrelease {
    param([string]$QtDir)
    
    $candidates = @()
    
    if ($QtDir) {
        $candidates += Join-Path $QtDir "bin\lrelease.exe"
    }
    
    # From environment variable
    if ($env:Qt5_DIR) {
        $candidates += Join-Path (Split-Path -Parent $env:Qt5_DIR) "..\..\..\bin\lrelease.exe"
    }
    
    # Common Qt installation paths
    $candidates += @(
        "C:\Qt\5.15.2\msvc2019_64\bin\lrelease.exe",
        "C:\Qt\5.15.2\msvc2017_64\bin\lrelease.exe",
        "D:\Qt\5.15.2\msvc2019_64\bin\lrelease.exe",
        "D:\Qt5\5.15.2\msvc2019_64\bin\lrelease.exe"
    )
    
    # From PATH
    $inPath = Get-Command lrelease.exe -ErrorAction SilentlyContinue
    if ($inPath) {
        return $inPath.Source
    }
    
    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }
    
    return $null
}

$lrelease = Find-Lrelease -QtDir $QtDir

if (-not $lrelease) {
    Write-Error "Cannot find lrelease tool. Please ensure Qt5 LinguistTools is installed and use -QtDir parameter."
    exit 1
}

Write-Host "Using lrelease: $lrelease" -ForegroundColor Green

# Auto-detect output directory
if (-not $OutputDir) {
    # Try to get install prefix from CMakeCache
    $cacheFile = Join-Path $BuildDir "CMakeCache.txt"
    if (Test-Path $cacheFile) {
        $match = Select-String -Path $cacheFile -Pattern "CMAKE_INSTALL_PREFIX:PATH=(.+)"
        if ($match) {
            $installPrefix = $match.Matches.Groups[1].Value -replace '/', '\'
            if ($installPrefix) {
                $OutputDir = Join-Path $installPrefix "bin\translations"
                Write-Host "Detected install prefix from CMakeCache: $installPrefix" -ForegroundColor Green
            }
        }
    }
    
    # Fallback to temp directory
    if (-not $OutputDir) {
        $OutputDir = Join-Path $ProjectRoot "build\translations"
        Write-Host "Using temp output directory (use -OutputDir to specify install location)" -ForegroundColor Yellow
    }
}

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

Write-Host "Translations source: $TranslationsDir" -ForegroundColor Cyan
Write-Host "Output directory: $OutputDir" -ForegroundColor Cyan
Write-Host "Languages: $($Languages -join ', ')" -ForegroundColor Cyan
Write-Host "Components: $($Components -join ', ')" -ForegroundColor Cyan
Write-Host ""

$compiled = 0
$failed = 0

foreach ($lang in $Languages) {
    # Try different language code formats
    $langVariants = @($lang, $lang.Replace("-", "_"), $lang.Replace("_", "-"))
    $langVariants = $langVariants | Select-Object -Unique
    
    foreach ($component in $Components) {
        $found = $false
        
        foreach ($langVar in $langVariants) {
            $tsFile = Join-Path $TranslationsDir "$($component)_$($langVar).ts"
            
            if (Test-Path $tsFile) {
                $qmFile = Join-Path $OutputDir "$($component)_$($langVar).qm"
                
                Write-Host "Compiling: $($component)_$($langVar).ts -> .qm" -ForegroundColor Yellow
                
                try {
                    & $lrelease $tsFile -qm $qmFile 2>&1 | Out-Null
                    
                    if (Test-Path $qmFile) {
                        Write-Host "  [OK] Success" -ForegroundColor Green
                        $compiled++
                    } else {
                        Write-Host "  [FAIL] Output file not generated" -ForegroundColor Red
                        $failed++
                    }
                } catch {
                    Write-Host "  [FAIL] Compilation error: $_" -ForegroundColor Red
                    $failed++
                }
                
                $found = $true
                break
            }
        }
        
        if (-not $found) {
            Write-Host "Skipping: $component (no $lang translation found)" -ForegroundColor Gray
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($failed -eq 0) {
    Write-Host "Completed: $compiled succeeded, $failed failed" -ForegroundColor Green
} else {
    Write-Host "Completed: $compiled succeeded, $failed failed" -ForegroundColor Yellow
}
Write-Host "Output directory: $OutputDir" -ForegroundColor Cyan

# List generated files
Write-Host ""
Write-Host "Generated translation files:" -ForegroundColor Cyan
Get-ChildItem $OutputDir -Filter "*.qm" -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Host "  $($_.Name) ($([math]::Round($_.Length/1KB, 1)) KB)" -ForegroundColor White
}

Write-Host ""
if ($compiled -gt 0) {
    Write-Host "[OK] Chinese translations deployed. Restart the app to take effect." -ForegroundColor Green
} else {
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "1. Ensure SlicerLanguageTranslations-main/translations directory exists" -ForegroundColor Yellow
    Write-Host "2. Use -OutputDir to specify the correct install directory" -ForegroundColor Yellow
    Write-Host "Example: .\Deploy-Translations.ps1 -OutputDir 'C:\S\rs-install\bin\translations'" -ForegroundColor Yellow
}
