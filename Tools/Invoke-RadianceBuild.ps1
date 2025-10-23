param(
  [ValidateSet('win-ninja-dev','win-ninja-rel')]
  [string]$Preset = 'win-ninja-dev',

  [string]$QtDir,

  [switch]$Package,

  [int]$Jobs = 0,

  [switch]$ForceConfigure,

  [string[]]$ExtraCMakeArgs,

  [switch]$UseSharedSlicer,

  [switch]$OutOfTree,

  [switch]$AutoShortDriveSlicer = $true,

  [string]$ShortDriveLetter = 'R',

  [switch]$KeepShortDriveMapping
)

$ErrorActionPreference = 'Stop'

function Test-Command {
  param([string]$Name)
  return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
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

function Get-FreeDriveLetter([string[]]$Preferred) {
  $used = Get-PSDrive -PSProvider FileSystem | Select-Object -ExpandProperty Name
  foreach ($l in $Preferred) { if (-not ($used -contains $l.TrimEnd(':'))) { return ($l.TrimEnd(':')) } }
  return $null
}

function Invoke-SlicerShortDriveBuild {
  param(
    [string]$SlicerBuildDir,
    [int]$Jobs = 0,
    [string]$PreferredLetter = 'R',
    [switch]$KeepMapping
  )
  if (-not (Test-Path $SlicerBuildDir)) { return $false }
  $drive = $PreferredLetter.TrimEnd(':')
  if (-not $drive) { $drive = 'R' }
  if (Test-Path ($drive + ':\')) {
    $alt = Get-FreeDriveLetter -Preferred @('R','Q','P','S','T','U')
    if ($alt) { $drive = $alt } else { Write-Warning 'No free drive letter for subst mapping.'; return $false }
  }
  $mapCmd = "subst ${drive}: `"$SlicerBuildDir`""
  Write-Host "Mapping ${drive}: to $SlicerBuildDir for short-path Slicer build..." -ForegroundColor Yellow
  cmd /c $mapCmd | Out-Null
  $ok = Test-Path ("${drive}:/")
  if (-not $ok) { Write-Warning "Failed to map ${drive}: to $SlicerBuildDir"; return $false }
  try {
    Push-Location ("${drive}:/")
    Write-Host "Building Slicer in ${drive}:/ with ninja (Jobs=$Jobs)..." -ForegroundColor Green
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

function Ensure-PythonLibAliases {
  param([string]$RootBuildDir)
  $changed = $false
  if (-not $RootBuildDir) { return $false }
  $candidateDirs = @(
    (Join-Path $RootBuildDir 'python-build\libs'),
    (Join-Path $RootBuildDir 'python-install\libs'),
    (Join-Path $RootBuildDir 'python-build\CMakeBuild\libpython\Release')
  )
  foreach ($dir in $candidateDirs) {
    if (-not (Test-Path $dir)) { continue }
    $alias = Join-Path $dir 'python3.lib'
    if (-not (Test-Path $alias)) {
      $src = Get-ChildItem -Path $dir -Filter 'python3??.lib' -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ne 'python3.lib' } |
        Sort-Object Name -Descending |
        Select-Object -First 1
      if (-not $src) {
        # try more generic pattern (e.g., python312.lib under different layout)
        $src = Get-ChildItem -Path $dir -Filter 'python3*.lib' -ErrorAction SilentlyContinue |
          Where-Object { $_.Name -ne 'python3.lib' } |
          Sort-Object Name -Descending |
          Select-Object -First 1
      }
      if ($src) {
        try {
          Copy-Item -Path $src.FullName -Destination $alias -Force
          Write-Host ("Created python lib alias: {0} -> {1}" -f $alias, $src.Name) -ForegroundColor Yellow
          $changed = $true
        } catch {}
      }
    }
  }
  return $changed
}

function Sanitize-UpstreamQtCache {
  param([string]$SlicerBinDir)
  $changed = $false
  if (-not $SlicerBinDir) { return $false }
  # Scan all ExternalProject initial cache files under *-prefix/tmp
  $files = Get-ChildItem -Path $SlicerBinDir -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\-prefix\\tmp\\' -and $_.Name -like '*cache-*.cmake' }
  foreach ($f in $files) {
    try {
      $lines = Get-Content -Path $f.FullName
      $out = @()
      $localChanged = $false
      $hasForceRsp = $false
      $hasObjMax = $false
      foreach ($ln in $lines) {
        if ($ln -match '^\s*set\(\s*CMAKE_NINJA_FORCE_RESPONSE_FILE\s*"?ON"?\s*\)') { $hasForceRsp = $true }
        if ($ln -match '^\s*set\(\s*CMAKE_OBJECT_PATH_MAX\s*"?\d+"?\s*\)') { $hasObjMax = $true }
        if ($ln -match '^\s*set\(\s*Qt5_DIR\s*"([^"]+)"') {
          $parts = $ln -split '"',3
          if ($parts.Count -ge 3) {
            $fixed = $parts[1] -replace "\\","/"
            if ($fixed -ne $parts[1]) {
              $ln = $parts[0] + '"' + $fixed + '"' + $parts[2]
              $localChanged = $true
            }
          }
        }
        $out += $ln
      }
      if (-not $hasForceRsp) {
        $out += 'set(CMAKE_NINJA_FORCE_RESPONSE_FILE "ON")'
        $localChanged = $true
      }
      if (-not $hasObjMax) {
        $out += 'set(CMAKE_OBJECT_PATH_MAX "128")'
        $localChanged = $true
      }
      if ($localChanged) {
        Set-Content -Path $f.FullName -Value $out -Encoding UTF8
        Write-Host "Sanitized cache: $($f.FullName)" -ForegroundColor Yellow
        $changed = $true
      }
    } catch {}
  }
  return $changed
}

Write-Host "RadianceSuite fast build script (CMake Presets + Ninja + sccache)" -ForegroundColor Cyan

# Hint if VS dev env not loaded when using Ninja + cl
if (-not (Test-Command ninja)) {
  Write-Warning "Ninja not found. Install via Chocolatey: 'choco install ninja' or VS installer."
}

if (-not (Test-Command cmake)) {
  throw "cmake not found in PATH. Install CMake 3.23+ and retry."
}

if ($QtDir) {
  if (-not (Test-Path $QtDir)) { throw "-QtDir not found: $QtDir" }
  $env:QT5_DIR = $QtDir
} elseif (-not $env:QT5_DIR) {
  Write-Warning "QT5_DIR not set. Set -QtDir or $env:QT5_DIR to prebuilt Qt5 cmake path."
}
# Normalize Qt path to forward slashes to avoid CMake escape issues
if ($env:QT5_DIR) {
  $env:QT5_DIR = $env:QT5_DIR -replace "\\","/"
  Write-Host "Using Qt5_DIR = $env:QT5_DIR"
}

if (Test-Command sccache) {
  if (-not $env:SCCACHE_DIR) { $env:SCCACHE_DIR = Join-Path $HOME ".sccache" }
  if (-not $env:SCCACHE_CACHE_SIZE) { $env:SCCACHE_CACHE_SIZE = "20G" }
  Write-Host "sccache enabled. Cache: $env:SCCACHE_DIR (size $env:SCCACHE_CACHE_SIZE)"
} else {
  Write-Host "sccache not found; proceeding without compiler cache." -ForegroundColor Yellow
}

# Ensure MSVC/SDK environment for Ninja + clang-cl
if (-not (Test-Command cl.exe) -or -not $env:INCLUDE -or -not $env:LIB) {
  Import-VSDevEnvironment
}

# Fallback: add MSVC bin x64 to PATH explicitly if missing
if (-not (Test-Command cl.exe)) {
  $msvcRoot = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC'
  if (Test-Path $msvcRoot) {
    $latest = Get-ChildItem $msvcRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
    if ($latest) {
      $vcBin = Join-Path $latest.FullName 'bin\\Hostx64\\x64'
      if (Test-Path $vcBin) {
        $env:PATH = "$vcBin;$env:PATH"
        Write-Host "Added MSVC bin to PATH: $vcBin" -ForegroundColor Yellow
      }
    }
  }
}

# Ensure MSVC link tools preferred over MinGW/Strawberry
$env:PATH = (@($env:PATH -split ';') | Where-Object { $_ -and ($_ -notmatch 'Strawberry\\c\\bin') -and ($_ -notmatch 'MinGW') }) -join ';'
$linkCmd = Get-Command link.exe -ErrorAction SilentlyContinue
if (-not $linkCmd) {
  $vcLink = Get-ChildItem 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC\\*\\bin\\Hostx64\\x64\\link.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($vcLink) { $env:PATH = "$(Split-Path $vcLink.FullName -Parent);$env:PATH"; $linkCmd = Get-Command link.exe -ErrorAction SilentlyContinue }
}
if ($linkCmd) {
  $cmakeArgFixups += ("-DCMAKE_LINKER={0}" -f $linkCmd.Source)
  $env:LD = $linkCmd.Source
}
$ldEnv = [Environment]::GetEnvironmentVariable('LD','Process')
if ($ldEnv -and ($ldEnv -match 'Strawberry' -or $ldEnv -match 'MinGW')) {
  if ($linkCmd) { $env:LD = $linkCmd.Source } else { [Environment]::SetEnvironmentVariable('LD',$null,'Process') }
}
$libCmd = Get-Command lib.exe -ErrorAction SilentlyContinue
if (-not $libCmd) {
  $vcLib = Get-ChildItem 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC\\*\\bin\\Hostx64\\x64\\lib.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($vcLib) { $env:PATH = "$(Split-Path $vcLib.FullName -Parent);$env:PATH"; $libCmd = Get-Command lib.exe -ErrorAction SilentlyContinue }
}
if ($libCmd) {
  $cmakeArgFixups += ("-DCMAKE_AR={0}" -f $libCmd.Source)
}

# Ensure MT and RC are available to CMake
$cmakeArgFixups = @()
if (-not (Test-Command mt.exe)) {
  $mt = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin\*\x64\mt.exe' -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
  if ($mt) {
    $mtDir = Split-Path $mt.FullName -Parent
    $env:PATH = "$mtDir;$env:PATH"
    $cmakeArgFixups += ("-DCMAKE_MT={0}" -f $mt.FullName)
    Write-Host "Using mt.exe: $($mt.FullName)" -ForegroundColor Yellow
  }
}
if (-not (Test-Command rc.exe)) {
  $rc = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin\*\x64\rc.exe' -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
  if ($rc) {
    $rcDir = Split-Path $rc.FullName -Parent
    $env:PATH = "$rcDir;$env:PATH"
    $cmakeArgFixups += ("-DCMAKE_RC_COMPILER={0}" -f $rc.FullName)
    Write-Host "Using rc.exe: $($rc.FullName)" -ForegroundColor Yellow
  }
}

 # Configure if missing or forced
$needsConfigure = $ForceConfigure
$effectivePreset = $Preset
switch ($Preset) {
  'win-ninja-dev' { $buildDir = Join-Path $PSScriptRoot "..\build\win-ninja-dev" }
  'win-ninja-rel' { $buildDir = Join-Path $PSScriptRoot "..\build\win-ninja-rel" }
}

 # Adjust preset for shared Slicer or out-of-tree builds
if ($UseSharedSlicer) {
  if ($effectivePreset -eq 'win-ninja-dev') { $effectivePreset = 'win-ninja-dev-shared' }
  if ($effectivePreset -eq 'win-ninja-rel') { $effectivePreset = 'win-ninja-rel-shared' }
}
if ($OutOfTree) {
  if ($effectivePreset -eq 'win-ninja-dev') { $effectivePreset = 'win-ninja-dev-out' }
  if ($effectivePreset -eq 'win-ninja-rel') { $effectivePreset = 'win-ninja-rel-out' }
}

# If using shared Slicer and env vars are not set, try defaults
if ($UseSharedSlicer) {
  if (-not $env:SLICER_SRC_DIR -or -not $env:SLICER_BIN_DIR) {
    $defaultSrc = 'C:\W\Slicer'
    $defaultBin = 'C:\W\Slicer-build'
    if ((Test-Path $defaultSrc) -and (Test-Path $defaultBin)) {
      $env:SLICER_SRC_DIR = $defaultSrc
      $env:SLICER_BIN_DIR = $defaultBin
      Write-Host "Using default shared Slicer: SRC=$defaultSrc BIN=$defaultBin" -ForegroundColor Yellow
    } else {
      Write-Warning "SLICER_SRC_DIR/BIN_DIR not set and defaults not found. Run 'pwsh Tools/Setup-SharedSlicer.ps1 -SetEnv' first."
    }
  }
  # Normalize to forward slashes for CMake
  if ($env:SLICER_SRC_DIR) { $env:SLICER_SRC_DIR = $env:SLICER_SRC_DIR -replace "\\","/" }
  if ($env:SLICER_BIN_DIR) { $env:SLICER_BIN_DIR = $env:SLICER_BIN_DIR -replace "\\","/" }
}

# Determine final buildDir (defaults are out-of-tree)
switch ($effectivePreset) {
  'win-ninja-dev'        { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\win-ninja-dev" }
  'win-ninja-rel'        { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\win-ninja-rel" }
  'win-ninja-dev-shared' { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\win-ninja-dev" }
  'win-ninja-rel-shared' { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\win-ninja-rel" }
  'win-ninja-dev-out'    { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\win-ninja-dev" }
  'win-ninja-rel-out'    { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\win-ninja-rel" }
  default                { $buildDir = Join-Path $PSScriptRoot "..\\..\\RS-build\\$Preset" }
}
if (-not (Test-Path $buildDir)) { $needsConfigure = $true }
elseif (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) { $needsConfigure = $true }

if ($needsConfigure) {
  Write-Host "Configuring with preset '$effectivePreset'..." -ForegroundColor Green
  $cfgArgs = @('--preset', $effectivePreset)
  if ($ExtraCMakeArgs) { $cfgArgs += $ExtraCMakeArgs }
  # Purge any stale linker settings from cache (avoid picking MinGW/Strawberry ld.exe)
  $cfgArgs += @('-U','CMAKE_LINKER','-U','CMAKE_.*_LINKER')
  if ($cmakeArgFixups -and $cmakeArgFixups.Count -gt 0) { $cfgArgs += $cmakeArgFixups }
  cmake @cfgArgs | Write-Host
} else {
  Write-Host "Configure step skipped (use -ForceConfigure to reconfigure)."
}

# Build using build preset (align with shared/out variants)
if ($effectivePreset -like '*rel*') {
  if ($effectivePreset -like '*shared*') { $buildPreset = 'build-rel-shared' }
  elseif ($effectivePreset -like '*out*') { $buildPreset = 'build-rel-out' }
  else { $buildPreset = 'build-rel' }
} else {
  if ($effectivePreset -like '*shared*') { $buildPreset = 'build-dev-shared' }
  elseif ($effectivePreset -like '*out*') { $buildPreset = 'build-dev-out' }
  else { $buildPreset = 'build-dev' }
}

Write-Host "Building with preset '$buildPreset' (Jobs=$Jobs) ..." -ForegroundColor Green
# Proactively ensure Python lib alias if already present
[void](Ensure-PythonLibAliases -RootBuildDir $buildDir)
# Proactively sanitize upstream VTK cache to avoid backslash escapes in Qt5_DIR
if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) { [void](Sanitize-UpstreamQtCache -SlicerBinDir $env:SLICER_BIN_DIR) }
if ($Jobs -gt 0) {
  cmake --build --preset $buildPreset -- -j $Jobs | Write-Host
} else {
  cmake --build --preset $buildPreset | Write-Host
}

if ($LASTEXITCODE -ne 0) {
  # Attempt remediation for python3.lib and Qt backslash path, then retry once
  $fixed = Ensure-PythonLibAliases -RootBuildDir $buildDir
  if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) { $fixed = (Sanitize-UpstreamQtCache -SlicerBinDir $env:SLICER_BIN_DIR) -or $fixed }
  # If still failing, try short-drive Slicer build to avoid rsp/path issues
  if (-not $fixed -and $AutoShortDriveSlicer) {
    $slicerBuildDir = Join-Path $buildDir 'Slicer-build'
    if (Test-Path $slicerBuildDir) {
      $shortOk = Invoke-SlicerShortDriveBuild -SlicerBuildDir $slicerBuildDir -Jobs $Jobs -PreferredLetter $ShortDriveLetter -KeepMapping:$KeepShortDriveMapping
      if ($shortOk) { $fixed = $true }
    }
  }
  if ($fixed) {
    Write-Host "Retrying build after applying remediation..." -ForegroundColor Yellow
    if ($Jobs -gt 0) {
      cmake --build --preset $buildPreset -- -j $Jobs | Write-Host
    } else {
      cmake --build --preset $buildPreset | Write-Host
    }
  }
}

if ($LASTEXITCODE -ne 0) { throw "Build failed." }

if ($Package) {
  Write-Host "Packaging (target 'package') ..." -ForegroundColor Green
  cmake --build $buildDir --target package | Write-Host
}

Write-Host "Done." -ForegroundColor Cyan
