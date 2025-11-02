param(
  [ValidateSet('win-ninja-dev','win-ninja-rel','vs17-dev')]
  [string]$Preset = 'vs17-dev',

  [string]$QtDir,

  [switch]$Package,

  [int]$Jobs = 0,

  [switch]$ForceConfigure,

  # Configure only: run CMake configure step and exit without building
  [switch]$ConfigureOnly,

  [string[]]$ExtraCMakeArgs,

  [switch]$UseSharedSlicer,

  [switch]$OutOfTree,

  [switch]$AutoShortDriveSlicer = $true,

  [string]$ShortDriveLetter = 'R',

  [switch]$KeepShortDriveMapping,

  # Optional: Override build root directory (defaults to ..\\..\\RS-build via presets)
  # Example: -BuildRoot C:/RS-build2 (actual binary dir becomes C:/RS-build2/win-ninja-dev or rel)
  [string]$BuildRoot,

  # Retry failed builds (helps with flaky network fetch of externals)
  [int]$BuildRetries = 5,

  # Enable Windows long path support (requires reboot to fully take effect)
  [switch]$EnableLongPaths
)

$ErrorActionPreference = 'Stop'

# Path to Ninja wrapper that demotes duplicate-rule errors to warnings.
# Use forward slashes so it can be safely passed to CMake.
$global:NinjaWrapPath = 'C:/Tools/Ninja-1.11.1/ninja-wrap.bat'

function Test-Command {
  param([string]$Name)
  return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

function Get-NinjaVersion {
  try {
    $ver = ninja --version 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $ver) { return $null }
    return $ver.Trim()
  } catch { return $null }
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

# Proactively import VS developer environment so cl/link/SDK are available in plain PowerShell
Import-VSDevEnvironment

# Ensure preferred CMake version is used if available (prefer 3.30/3.29, then 3.27)
function Get-CMakeVersion([string]$Exe){ try { & $Exe --version | Select-Object -First 1 } catch { return $null } }
function Ensure-PreferredCMake {
  $cur = (Get-Command cmake -ErrorAction SilentlyContinue)
  $curLine = if($cur){ Get-CMakeVersion -Exe $cur.Source } else { $null }
  $pref = @('^cmake\s+version\s+3\.30\.', '^cmake\s+version\s+3\.29\.', '^cmake\s+version\s+3\.27\.')
  foreach($p in $pref){ if ($curLine -and ($curLine -match $p)) { $global:PreferredCMakeDir = Split-Path $cur.Source -Parent; Write-Host "Using CMake: $($cur.Source) [$($curLine)]" -ForegroundColor Yellow; return } }
  $candidates = @('C:\\Program Files\\CMake\\bin\\cmake.exe','C:\\ProgramData\\chocolatey\\lib\\cmake\\tools\\**\\bin\\cmake.exe','C:\\Strawberry\\c\\bin\\cmake.exe')
  foreach($cand in $candidates){
    $paths = Get-ChildItem -Path $cand -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName
    foreach($p in $paths){
      $line = Get-CMakeVersion -Exe $p
      if ($line -and ($pref | Where-Object { $line -match $_ })){
        $dir = Split-Path $p -Parent
        $env:PATH = "$dir;" + $env:PATH
        $global:PreferredCMakeDir = $dir
        Write-Host "Preferring CMake: $p [$line]" -ForegroundColor Yellow
        return
      }
    }
  }
  if ($cur){ Write-Host "Using CMake: $($cur.Source) [$curLine]" -ForegroundColor Yellow }
}
Ensure-PreferredCMake

# Prefer pinned Ninja 1.11.1 early in PATH so try_compile picks it up
try {
  $n11 = 'C:\\Tools\\Ninja-1.11.1'
  if (Test-Path $n11) {
    $paths = @($env:PATH -split ';')
    if (-not ($paths | Where-Object { $_ -and ($_ -ieq $n11) })) {
      $env:PATH = "$n11;" + $env:PATH
    }
  }
} catch {}

function Ensure-WindowsSdkLibInclude {
  try {
    $sdkRoot = 'C:\Program Files (x86)\Windows Kits\10'
    if (-not (Test-Path $sdkRoot)) { return $false }
    $libRoot = Join-Path $sdkRoot 'Lib'
    $incRoot = Join-Path $sdkRoot 'Include'
    if (-not (Test-Path $libRoot) -or -not (Test-Path $incRoot)) { return $false }
    $ver = (Get-ChildItem $libRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).Name
    if (-not $ver) { return $false }
    $umLib  = Join-Path $libRoot  (Join-Path $ver 'um\x64')
    $ucrtLib= Join-Path $libRoot  (Join-Path $ver 'ucrt\x64')
    $umInc  = Join-Path $incRoot  (Join-Path $ver 'um')
    $ucrtInc= Join-Path $incRoot  (Join-Path $ver 'ucrt')
    $sharedInc = Join-Path $incRoot (Join-Path $ver 'shared')
    $winrtInc  = Join-Path $incRoot (Join-Path $ver 'winrt')
    $added = $false
    if (Test-Path $umLib -and ($env:LIB -notmatch [regex]::Escape($umLib))) { $env:LIB = "$umLib;" + $env:LIB; $added = $true }
    if (Test-Path $ucrtLib -and ($env:LIB -notmatch [regex]::Escape($ucrtLib))) { $env:LIB = "$ucrtLib;" + $env:LIB; $added = $true }
    foreach ($inc in @($umInc,$ucrtInc,$sharedInc,$winrtInc)) {
      if ($inc -and (Test-Path $inc) -and ($env:INCLUDE -notmatch [regex]::Escape($inc))) {
        $env:INCLUDE = "$inc;" + $env:INCLUDE; $added = $true
      }
    }
    if ($added) { Write-Host "Ensured Windows SDK LIB/INCLUDE paths (v$ver)." -ForegroundColor Yellow }
    return $added
  } catch { return $false }
}

function Ensure-MsvcVcLibInclude {
  try {
    $msvcRoot = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC'
    if (-not (Test-Path $msvcRoot)) { return $false }
    $latest = Get-ChildItem $msvcRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
    if (-not $latest) { return $false }
    $vcInc = Join-Path $latest.FullName 'include'
    $vcLib = Join-Path $latest.FullName 'lib\\x64'
    $added = $false
    if ((Test-Path $vcInc) -and ($env:INCLUDE -notmatch [regex]::Escape($vcInc))) { $env:INCLUDE = "$vcInc;" + $env:INCLUDE; $added = $true }
    if ((Test-Path $vcLib) -and ($env:LIB -notmatch [regex]::Escape($vcLib))) { $env:LIB = "$vcLib;" + $env:LIB; $added = $true }
    if ($added) { Write-Host "Ensured MSVC VC include/lib paths." -ForegroundColor Yellow }
    return $added
  } catch { return $false }
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
    $nwrap = if (Test-Path ($global:NinjaWrapPath -replace '\\','/')) { $global:NinjaWrapPath } else { $null }
    if ($nwrap) {
      if ($Jobs -gt 0) { & $nwrap -j $Jobs } else { & $nwrap }
    } else {
      if ($Jobs -gt 0) { ninja -j $Jobs } else { ninja }
    }
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

function Ensure-TeemQnanhibitPatched {
  param([string]$SlicerBinDir)
  if (-not $SlicerBinDir) { return $false }
  $teemSrc = Join-Path $SlicerBinDir 'teem-prefix\src\teem'
  $target = Join-Path $teemSrc 'CMake\TestQnanhibit.cmake'
  $patch = Join-Path $PSScriptRoot 'Patches\Teem\TestQnanhibit.cmake'
  if (-not (Test-Path $target)) { return $false }
  try {
    $content = Get-Content -Path $target -Raw -ErrorAction Stop
    if ($content -match 'Assume QNaNHiBit==1' -or $content -match 'probe skipped') {
      return $false
    }
    if (-not (Test-Path $patch)) { return $false }
    $bak = $target + '.bak'
    Copy-Item -Path $target -Destination $bak -Force -ErrorAction SilentlyContinue | Out-Null
    Copy-Item -Path $patch -Destination $target -Force
    Write-Host "Patched Teem QNaN probe: $target" -ForegroundColor Yellow
    return $true
  } catch {
    return $false
  }
}

function Clean-TeemInitialCaches {
  param([string]$SlicerBinDir)
  if (-not $SlicerBinDir) { return $false }
  $tmpDir = Join-Path $SlicerBinDir 'teem-prefix\tmp'
  if (-not (Test-Path $tmpDir)) { return $false }
  $changed = $false
  Get-ChildItem -Path $tmpDir -Filter 'teem-cache-*.cmake' -ErrorAction SilentlyContinue |
    ForEach-Object {
      try { Remove-Item -Path $_.FullName -Force; $changed = $true; Write-Host "Removed stale Teem cache: $($_.FullName)" -ForegroundColor Yellow } catch {}
    }
  return $changed
}

function Ensure-SharedSlicerConfigured {
  param(
    [string]$SlicerSrcDir,
    [string]$SlicerBinDir
  )
  if (-not $SlicerSrcDir -or -not $SlicerBinDir) { return $false }
  if (-not (Test-Path $SlicerSrcDir)) { return $false }
  if (-not (Test-Path $SlicerBinDir)) { New-Item -ItemType Directory -Force -Path $SlicerBinDir | Out-Null }
  $need = $false
  $bn = Join-Path $SlicerBinDir 'build.ninja'
  $rn = Join-Path $SlicerBinDir 'CMakeFiles/rules.ninja'
  if (-not (Test-Path $bn) -or -not (Test-Path $rn)) { $need = $true }
  if (-not $need) { return $false }
  Write-Host "Configuring shared Slicer in $SlicerBinDir ..." -ForegroundColor Yellow
  $args = @('-GNinja','-S', $SlicerSrcDir, '-B', $SlicerBinDir,
    '-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON',
    '-DCMAKE_OBJECT_PATH_MAX=128',
    '-DMSVC_DEBUG_INFORMATION_FORMAT=ProgramDatabase',
    # Stabilize HDF5/VTK checks on Windows
    '-DVTK_MODULE_ENABLE_VTK_IOHDF5=NO',
    '-DHDF5_ENABLE_LDOUBLE=OFF',
    '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
    '-DCMAKE_C_FLAGS=/D_CRT_DECLARE_NONSTDC_NAMES=1',
    '-DCMAKE_CXX_FLAGS=/D_CRT_DECLARE_NONSTDC_NAMES=1')
  if (Test-Path ($global:NinjaWrapPath -replace '\\','/')) {
    $args += ('-DCMAKE_MAKE_PROGRAM={0}' -f $global:NinjaWrapPath)
  }
  cmake @args | Write-Host
  return $true
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
      $hasPolicyMin = $false
      $hasMakeProg = $false
      $hasTryCfg = $false
      foreach ($ln in $lines) {
        if ($ln -match '^\s*set\(\s*CMAKE_NINJA_FORCE_RESPONSE_FILE\s*"?ON"?\s*\)') { $hasForceRsp = $true }
        if ($ln -match '^\s*set\(\s*CMAKE_OBJECT_PATH_MAX\s*"?\d+"?\s*\)') { $hasObjMax = $true }
        if ($ln -match '^\s*set\(\s*CMAKE_POLICY_VERSION_MINIMUM\s*"?') { $hasPolicyMin = $true }
        if ($ln -match '^\s*set\(\s*CMAKE_MAKE_PROGRAM\s*"?([^\"]+)"?\s*\)') {
          $hasMakeProg = $true
          $cur = $matches[1] -replace "\\","/"
          $target = $global:NinjaWrapPath
          if ($target -and ($cur -ne $target)) {
            $ln = ('set(CMAKE_MAKE_PROGRAM "{0}")' -f $target)
            $localChanged = $true
          }
        }
        if ($ln -match '^\s*set\(\s*CMAKE_TRY_COMPILE_CONFIGURATION\s*"?([^\"]+)"?\s*\)') { $hasTryCfg = $true }
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
      if (-not $hasPolicyMin) {
        $out += 'set(CMAKE_POLICY_VERSION_MINIMUM "3.5")'
        $localChanged = $true
      }
      # Ensure all ExternalProject initial caches use the Ninja wrapper
      if (-not $hasMakeProg -and (Test-Path ($global:NinjaWrapPath -replace '\\','/'))) {
        $out += ('set(CMAKE_MAKE_PROGRAM "{0}")' -f $global:NinjaWrapPath)
        $localChanged = $true
      }
      # Set safer try_compile configuration (Debug) to avoid COPY_FILE path issues on Ninja
      if (-not $hasTryCfg) {
        $out += 'set(CMAKE_TRY_COMPILE_CONFIGURATION "Debug")'
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

# Parse -D style CMake definitions from ExtraCMakeArgs into a hashtable
function Get-ExtraDefsMap {
  param([string[]]$Args)
  $map = @{}
  if (-not $Args) { return $map }
  foreach($a in $Args){
    if ($a -match '^-D([^=]+)=(.*)$'){
      $k=$matches[1]; $v=$matches[2]
      # strip surrounding quotes if present
      if ($v -match '^"(.*)"$'){ $v=$matches[1] }
      $map[$k]=$v
    }
  }
  return $map
}

# Ensure selected -D defs exist in ExternalProject initial cache files under *-prefix/tmp
function Apply-DefsToInitialCaches {
  param(
    [string]$RootDir,
    [hashtable]$Defs
  )
  if (-not $RootDir -or -not (Test-Path $RootDir) -or -not $Defs -or $Defs.Count -eq 0) { return $false }
  $changed=$false
  $files = Get-ChildItem -Path $RootDir -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\-prefix\\tmp\\' -and $_.Name -like '*cache-*.cmake' }
  foreach($f in $files){
    try{
      $lines = Get-Content -Path $f.FullName -ErrorAction Stop
      $out=@()
      $localChanged=$false
      foreach($line in $lines){ $out += $line }
      foreach($key in $Defs.Keys){
        $val=$Defs[$key]
        $pattern = '^\s*set\(\s*' + [regex]::Escape($key) + '\s*"?'
        $existing = $out | Where-Object { $_ -match $pattern }
        # Write as CACHE entry with FORCE to ensure it takes effect when used via -C initial cache
        $defLine = ('set({0} "{1}" CACHE STRING "Initial cache" FORCE)' -f $key, ($val -replace '\\','/'))
        if ($existing){
          # Replace first occurrence, keep rest
          for($i=0;$i -lt $out.Count;$i++){
            if ($out[$i] -match $pattern){ $out[$i]=$defLine; $localChanged=$true; break }
          }
        } else {
          $out += $defLine
          $localChanged=$true
        }
      }
      if ($localChanged){ Set-Content -Path $f.FullName -Value $out -Encoding UTF8; Write-Host "Applied defs to: $($f.FullName)" -ForegroundColor Yellow; $changed=$true }
    } catch {}
  }
  return $changed
}

# Ensure inner Slicer-build uses Ninja response files to avoid long command lines on Windows
function Ensure-SlicerBuildResponseFiles {
  param([string]$RootBuildDir)
  try {
    if (-not $RootBuildDir) { return $false }
    $slicerSrc = Join-Path $RootBuildDir 'slicersources-src'
    $slicerBin = Join-Path $RootBuildDir 'Slicer-build'
    if (-not (Test-Path $slicerSrc)) { return $false }
    if (-not (Test-Path $slicerBin)) { try { New-Item -ItemType Directory -Force -Path $slicerBin | Out-Null } catch {} }
    # If Slicer-build already has a generated build.ninja, avoid re-configuring here to prevent
    # duplicate custom targets (e.g., 'multiple rules generate CompileSlicerPythonFiles').
    $buildNinja = Join-Path $slicerBin 'build.ninja'
    if (Test-Path $buildNinja) {
      Write-Host "Slicer-build already configured (build.ninja present); skipping reconfigure to avoid duplicate rules." -ForegroundColor Yellow
      return $false
    }
    $cache = Join-Path $slicerBin 'CMakeCache.txt'
    $need = $true
    if (Test-Path $cache) {
      $txt = Get-Content -Path $cache -Raw -ErrorAction SilentlyContinue
      if ($txt -and $txt -match 'CMAKE_NINJA_FORCE_RESPONSE_FILE:BOOL=ON') { $need = $false }
    }
    if (-not $need) { return $false }
    Write-Host "Enabling Ninja response files for inner Slicer-build..." -ForegroundColor Yellow
    # Use a very short output path prefix for rsp/dep files to avoid D8022 path issues
    $shortPrefix = $slicerBin
    # If Slicer-build is mapped to a drive (e.g., R:/), prefer that
    $drives = Get-PSDrive -PSProvider FileSystem | Select-Object -ExpandProperty Root
    foreach($d in $drives){ if ($d -match '^[A-Z]:\\$' -and (Resolve-Path $d).Path -eq (Resolve-Path $slicerBin).Path){ $shortPrefix = ($d + 'o') ; break } }
    try { if (-not (Test-Path $shortPrefix)) { New-Item -ItemType Directory -Force -Path $shortPrefix | Out-Null } } catch {}

    # Force VC/Windows SDK LIB/INCLUDE for inner cmake try-compile to avoid LNK1104 (kernel32.lib)
    try {
      $msvcRoot = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC'
      $latest = Get-ChildItem $msvcRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
      if ($latest) {
        $vcInc = Join-Path $latest.FullName 'include'
        $vcLib = Join-Path $latest.FullName 'lib\\x64'
        if ($vcInc -and (Test-Path $vcInc) -and ($env:INCLUDE -notmatch [regex]::Escape($vcInc))) { $env:INCLUDE = "$vcInc;" + $env:INCLUDE }
        if ($vcLib -and (Test-Path $vcLib) -and ($env:LIB -notmatch [regex]::Escape($vcLib))) { $env:LIB = "$vcLib;" + $env:LIB }
      }
      $sdkRoot = 'C:\\Program Files (x86)\\Windows Kits\\10'
      $libRoot = Join-Path $sdkRoot 'Lib'
      $incRoot = Join-Path $sdkRoot 'Include'
      $ver = (Get-ChildItem $libRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).Name
      if ($ver) {
        $umLib  = Join-Path $libRoot  (Join-Path $ver 'um\\x64')
        $ucrtLib= Join-Path $libRoot  (Join-Path $ver 'ucrt\\x64')
        $umInc  = Join-Path $incRoot  (Join-Path $ver 'um')
        $ucrtInc= Join-Path $incRoot  (Join-Path $ver 'ucrt')
        $sharedInc = Join-Path $incRoot (Join-Path $ver 'shared')
        $winrtInc  = Join-Path $incRoot (Join-Path $ver 'winrt')
        $cppwinrtInc = Join-Path $incRoot (Join-Path $ver 'cppwinrt')
        foreach ($inc in @($umInc,$ucrtInc,$sharedInc,$winrtInc,$cppwinrtInc)) {
          if ($inc -and (Test-Path $inc) -and ($env:INCLUDE -notmatch [regex]::Escape($inc))) { $env:INCLUDE = "$inc;" + $env:INCLUDE }
        }
        foreach ($lib in @($umLib,$ucrtLib)) {
          if ($lib -and (Test-Path $lib) -and ($env:LIB -notmatch [regex]::Escape($lib))) { $env:LIB = "$lib;" + $env:LIB }
        }
      }
      $libPreview = (($env:LIB -split ';') | Where-Object { $_ } | Select-Object -First 3) -join ';'
      $incPreview = (($env:INCLUDE -split ';') | Where-Object { $_ } | Select-Object -First 3) -join ';'
      Write-Host "Inner cmake ENV prepared. LIB(head)=$libPreview" -ForegroundColor Yellow
      Write-Host "Inner cmake ENV prepared. INCLUDE(head)=$incPreview" -ForegroundColor Yellow
    } catch {}
    $args = @('-G','Ninja','-S',$slicerSrc,'-B',$slicerBin,
      '-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON',
      '-DCMAKE_OBJECT_PATH_MAX=128',
      '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=ProgramDatabase',
      '-DCMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS=ON',
      '-DCMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS=ON',
      '-DCMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES=ON',
      '-DCMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES=ON')
    if ($shortPrefix) { $args += ('-DCMAKE_NINJA_OUTPUT_PATH_PREFIX={0}' -f ($shortPrefix -replace "\\","/")) }
    if (Test-Path ($global:NinjaWrapPath -replace '\\','/')) {
      $args += ('-DCMAKE_MAKE_PROGRAM={0}' -f $global:NinjaWrapPath)
    }
    cmake @args | Write-Host
    return $true
  } catch { return $false }
}

# Remove known duplicate phony alias lines that can cause
# "multiple rules generate .../CompileSlicerPythonFiles" in Ninja
function Fix-NinjaDuplicateAliases {
  param([string]$SlicerBuildDir)
  try {
    if (-not $SlicerBuildDir) { return $false }
    $ninja = Join-Path $SlicerBuildDir 'build.ninja'
    if (-not (Test-Path $ninja)) { return $false }
    $text = Get-Content -LiteralPath $ninja -Raw -ErrorAction Stop
    $patterns = @(
      '(?m)^build\s+.+?Slicer-build\\CompileSlicerPythonFiles:\s+phony\s+.+?Slicer-build\\CompileSlicerPythonFiles\s*$',
      '(?m)^build\s+.+?Slicer-build\\CompileStdLibAndSitePackagesPythonFiles:\s+phony\s+.+?Slicer-build\\CompileStdLibAndSitePackagesPythonFiles\s*$'
    )
    $changed = $false
    foreach($pat in $patterns){
      if ($text -match $pat) {
        $text = [regex]::Replace($text, $pat, '# removed duplicate alias')
        $changed = $true
      }
    }
    if ($changed){
      Set-Content -LiteralPath $ninja -Value $text -Encoding ASCII -NoNewline
      Write-Host "Patched Ninja file to remove duplicate phony aliases" -ForegroundColor Yellow
      return $true
    }
    return $false
  } catch { return $false }
}

# Remove ExternalProject initial caches that contain a stale pattern (e.g., old build root like RS-build)
function Purge-InitialCachesByPattern {
  param(
    [string]$RootDir,
    [string]$Pattern
  )
  if (-not $RootDir -or -not (Test-Path $RootDir) -or -not $Pattern) { return 0 }
  $files = Get-ChildItem -Path $RootDir -Recurse -File -Include '*cache-*.cmake' -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\-prefix\\tmp\\' }
  $count=0
  foreach($f in $files){
    try {
      $text = Get-Content -Path $f.FullName -Raw -ErrorAction Stop
      if ($text -match [regex]::Escape($Pattern)) {
        Remove-Item -Path $f.FullName -Force -ErrorAction SilentlyContinue
        $count++
      }
    } catch {}
  }
  if ($count -gt 0) { Write-Host "Purged initial caches containing '$Pattern': $count" -ForegroundColor Yellow }
  return $count
}

# Normalize ExtraCMakeArgs also here (if invoked directly), same logic as Dev-Build-Ext
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
[void](Ensure-WindowsSdkLibInclude)

# Ensure VC++ STL include/lib paths available when VS dev env is not imported
[void](Ensure-MsvcVcLibInclude)

# Ensure MSVC link tools preferred over MinGW/Strawberry
$env:PATH = (@($env:PATH -split ';') | Where-Object { $_ -and ($_ -notmatch 'Strawberry\\c\\bin') -and ($_ -notmatch 'MinGW') }) -join ';'
$prefDir = $global:PreferredCMakeDir
if ($prefDir -and (Test-Path $prefDir)) {
  if (-not (@($env:PATH -split ';') | Where-Object { $_ -and ($_ -ieq $prefDir) })) {
    $env:PATH = "$prefDir;" + $env:PATH
  }
}
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

# Ensure MT and RC are available to CMake and add generator fixups
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

# Ninja 专属的 CMake 修正，仅在 Ninja 预设下注入
if ($Preset -like 'win-ninja*') {
  # Always enable Ninja response files and shorten object paths to avoid Windows command line limits
  $cmakeArgFixups += '-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON'
  # Respect CMake's minimum value (>=128)
  $cmakeArgFixups += '-DCMAKE_OBJECT_PATH_MAX=128'
  # Prefer PDB debug info to reduce path length pressure
  $cmakeArgFixups += '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=ProgramDatabase'

  # Ensure all generated build systems use the Ninja wrapper (propagates into ExternalProject builds)
  if (Test-Path ($global:NinjaWrapPath -replace '\\','/')) {
    $cmakeArgFixups += ('-DCMAKE_MAKE_PROGRAM={0}' -f $global:NinjaWrapPath)
  }

  # If sccache is available, use it as compiler launcher
  if (Test-Command sccache) {
    $cmakeArgFixups += '-DCMAKE_C_COMPILER_LAUNCHER=sccache'
    $cmakeArgFixups += '-DCMAKE_CXX_COMPILER_LAUNCHER=sccache'
  }
}

# Prefer TLS verify OFF only if explicitly requested upstream; keep conservative default here
# Expose knob as part of ExtraCMakeArgs, but also allow injection through fixups if desired later

# Optionally enable Windows long path support
if ($EnableLongPaths) {
  try {
    $key = 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem'
    $val = (Get-ItemProperty -Path $key -Name LongPathsEnabled -ErrorAction SilentlyContinue).LongPathsEnabled
    if ($val -ne 1) {
      New-ItemProperty -Path $key -Name LongPathsEnabled -PropertyType DWord -Value 1 -Force | Out-Null
      Write-Host 'Enabled Windows long paths (registry). Reboot is required to fully apply.' -ForegroundColor Yellow
    }
  } catch {}
}

# Warn if Ninja 1.12+ is detected (known longer-path quirks on Windows)
$ninjaVer = Get-NinjaVersion
if ($ninjaVer) {
  if ($ninjaVer -match '^(\d+)\.(\d+)\.(\d+)$') {
    $maj=[int]$matches[1]; $min=[int]$matches[2]
    if ($maj -ge 1 -and $min -ge 12) {
      Write-Warning ("Detected Ninja {0}. For maximal stability on Windows, Ninja 1.11.x is recommended." -f $ninjaVer)
    } else { Write-Host ("Using Ninja {0}" -f $ninjaVer) -ForegroundColor Yellow }
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
  # Ensure shared Slicer has a configured build system if missing
  if ($env:SLICER_SRC_DIR -and $env:SLICER_BIN_DIR) {
    [void](Ensure-SharedSlicerConfigured -SlicerSrcDir $env:SLICER_SRC_DIR -SlicerBinDir $env:SLICER_BIN_DIR)
  }
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
# VS 预设的二进制目录由 CMakePresets 控制；为确保首次集成可用，强制执行一次配置
if ($Preset -like 'vs17*') { $needsConfigure = $true }
if (-not (Test-Path $buildDir)) { $needsConfigure = $true }
elseif (-not (Test-Path (Join-Path $buildDir 'CMakeCache.txt'))) { $needsConfigure = $true }
else {
  # If generator files look incomplete, force re-configure
  $buildNinja = Join-Path $buildDir 'build.ninja'
  $rulesNinja = Join-Path $buildDir 'CMakeFiles/rules.ninja'
  if (-not (Test-Path $buildNinja) -or -not (Test-Path $rulesNinja)) { $needsConfigure = $true }
}

if ($needsConfigure) {
  Write-Host "Configuring with preset '$effectivePreset'..." -ForegroundColor Green
  $cfgArgs = @('--preset', $effectivePreset)
  $extraNorm = Normalize-ExtraArgs -Args $ExtraCMakeArgs
  if ($extraNorm) { $cfgArgs += $extraNorm }
  # Purge any stale linker settings from cache (avoid picking MinGW/Strawberry ld.exe)
  $cfgArgs += @('-U','CMAKE_LINKER','-U','CMAKE_.*_LINKER')
  if ($cmakeArgFixups -and $cmakeArgFixups.Count -gt 0) { $cfgArgs += $cmakeArgFixups }
  if ($BuildRoot) {
    $sub = if ($effectivePreset -like '*rel*') { 'win-ninja-rel' } else { 'win-ninja-dev' }
    $binDir = Join-Path ($BuildRoot -replace "\\","/") $sub
    if (-not (Test-Path $binDir)) { New-Item -ItemType Directory -Force -Path $binDir | Out-Null }
    # Repo root is one level up from Tools
    $srcDir = Resolve-Path (Join-Path $PSScriptRoot '..') | Select-Object -ExpandProperty Path
    $srcDir = ($srcDir -replace "\\","/")
    $cfgArgs += @('-B', $binDir, '-S', $srcDir)

    # Map short drive for inner Slicer-build before configure so response-file prefix can use it
    if ($AutoShortDriveSlicer) {
      try {
        $slicerBin = Join-Path $binDir 'Slicer-build'
        if (-not (Test-Path $slicerBin)) { New-Item -ItemType Directory -Force -Path $slicerBin | Out-Null }
        $drv = $ShortDriveLetter.TrimEnd(':'); if (-not $drv) { $drv = 'R' }
        if (Test-Path ("$($drv):")) {
          # choose an alternate letter if requested one is busy
          foreach($alt in 'R','Q','P','S','T','U') { if (-not (Test-Path ("$($alt):"))) { $drv=$alt; break } }
        }
        Write-Host ("Mapping {0}: to {1} for short-path Slicer configure..." -f $drv,$slicerBin) -ForegroundColor Yellow
        cmd /c ("subst {0}: `"{1}`"" -f $drv,$slicerBin) | Out-Null
      } catch {}
    }
  }
  cmake @cfgArgs | Write-Host
  # Ninja 专属：注入 -D 到各 ExternalProject 初始 cache，VS 生成器不需要
  if ($effectivePreset -like 'win-ninja*') {
    $defs = Get-ExtraDefsMap -Args $extraNorm
    if ($BuildRoot) {
      [void](Apply-DefsToInitialCaches -RootDir $binDir -Defs $defs)
      [void](Purge-InitialCachesByPattern -RootDir $binDir -Pattern 'RS-build')
      [void](Purge-InitialCachesByPattern -RootDir $binDir -Pattern 'RS\\\\-build')
      [void](Purge-InitialCachesByPattern -RootDir $binDir -Pattern 'W/Slicer-build')
      [void](Purge-InitialCachesByPattern -RootDir $binDir -Pattern 'W\\\\Slicer-build')
    } else {
      [void](Apply-DefsToInitialCaches -RootDir $buildDir -Defs $defs)
      [void](Purge-InitialCachesByPattern -RootDir $buildDir -Pattern 'RS-build')
      [void](Purge-InitialCachesByPattern -RootDir $buildDir -Pattern 'RS\\\\-build')
      [void](Purge-InitialCachesByPattern -RootDir $buildDir -Pattern 'W/Slicer-build')
      [void](Purge-InitialCachesByPattern -RootDir $buildDir -Pattern 'W\\\\Slicer-build')
    }
  }
} else {
  Write-Host "Configure step skipped (use -ForceConfigure to reconfigure)."
}

# Honor ConfigureOnly: skip any build/package/fallback logic
if ($ConfigureOnly) {
  Write-Host "ConfigureOnly specified: skipping build steps." -ForegroundColor Yellow
  return
}

# 选择构建 preset（VS 使用 vs17- 系列，其余沿用 Ninja 系列）
if ($effectivePreset -like 'vs17*') {
  $buildPreset = 'vs17-dev-rel'
} else {
  if ($effectivePreset -like '*rel*') {
    if ($effectivePreset -like '*shared*') { $buildPreset = 'build-rel-shared' }
    elseif ($effectivePreset -like '*out*') { $buildPreset = 'build-rel-out' }
    else { $buildPreset = 'build-rel' }
  } else {
    if ($effectivePreset -like '*shared*') { $buildPreset = 'build-dev-shared' }
    elseif ($effectivePreset -like '*out*') { $buildPreset = 'build-dev-out' }
    else { $buildPreset = 'build-dev' }
  }
}

# If BuildRoot is specified, override buildDir to point to custom binary dir
if ($BuildRoot) {
  $sub = if ($effectivePreset -like '*rel*') { 'win-ninja-rel' } else { 'win-ninja-dev' }
  $buildDir = Join-Path ($BuildRoot -replace "\\","/") $sub
}

Write-Host "Building with preset '$buildPreset' (Jobs=$Jobs) ..." -ForegroundColor Green
if ($effectivePreset -like 'win-ninja*') {
  # Proactively ensure Python lib alias if already present
  [void](Ensure-PythonLibAliases -RootBuildDir $buildDir)
  # Sanitize initial caches for ExternalProject (response files, object path, policy minimum)
  # Always sanitize current build dir; also sanitize shared Slicer if in use.
  [void](Sanitize-UpstreamQtCache -SlicerBinDir $buildDir)
  # Also propagate top-level -D defs into EP initial caches before build
  $extraNorm = Normalize-ExtraArgs -Args $ExtraCMakeArgs
  $defs = Get-ExtraDefsMap -Args $extraNorm
  [void](Apply-DefsToInitialCaches -RootDir $buildDir -Defs $defs)
   # Ensure inner Slicer-build uses response files to avoid CreateProcess failures
   [void](Ensure-SlicerBuildResponseFiles -RootBuildDir $buildDir)
  if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) { [void](Sanitize-UpstreamQtCache -SlicerBinDir $env:SLICER_BIN_DIR) }
}
# If shared Slicer already fetched ExternalProject sources, proactively patch Teem probe
if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) {
  $patched = Ensure-TeemQnanhibitPatched -SlicerBinDir $env:SLICER_BIN_DIR
  if ($patched) {
    # remove stale initial caches to force re-configure with patched macro
    [void](Clean-TeemInitialCaches -SlicerBinDir $env:SLICER_BIN_DIR)
  }
}
function Invoke-BuildOnce {
  param([string]$Dir,[string]$Preset,[int]$Jobs)
  $nativeArgs = @('-w','dupbuild=warn')
  if ($Dir) {
    if ($Jobs -gt 0) { cmake --build $Dir -- -j $Jobs @nativeArgs | Write-Host } else { cmake --build $Dir -- @nativeArgs | Write-Host }
  } else {
    if ($Jobs -gt 0) { cmake --build --preset $Preset -- -j $Jobs @nativeArgs | Write-Host } else { cmake --build --preset $Preset -- @nativeArgs | Write-Host }
  }
}

$attempt=0; $success=$false
do {
  $attempt++
  if ($BuildRoot) { Invoke-BuildOnce -Dir $buildDir -Jobs $Jobs } else { Invoke-BuildOnce -Preset $buildPreset -Jobs $Jobs }
  if ($LASTEXITCODE -eq 0) { $success=$true; break }
  if ($attempt -le $BuildRetries) {
    Write-Host ("Build attempt {0}/{1} failed. Retrying..." -f $attempt,$BuildRetries) -ForegroundColor Yellow
    Start-Sleep -Seconds ([Math]::Min(15, 5 * $attempt))
  }
} while($attempt -le $BuildRetries)

if (-not $success -and $LASTEXITCODE -ne 0 -and ($effectivePreset -like 'win-ninja*')) {
  # Attempt remediation for python3.lib and Qt backslash path, then retry once
  $fixed = Ensure-PythonLibAliases -RootBuildDir $buildDir
  if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) { $fixed = (Sanitize-UpstreamQtCache -SlicerBinDir $env:SLICER_BIN_DIR) -or $fixed }
  # If using shared Slicer, prefer building there on short path instead of local Slicer-build
  $slicerBuildDir = $null
  if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) { $slicerBuildDir = $env:SLICER_BIN_DIR } else { $slicerBuildDir = Join-Path $buildDir 'Slicer-build' }

  # Try to sanitize inner Ninja file to remove duplicate phony targets that cause
  # 'multiple rules generate ...' errors (CompileSlicerPythonFiles, etc.)
  try {
    if ($slicerBuildDir -and (Test-Path $slicerBuildDir)) {
      if (Fix-NinjaDuplicateAliases -SlicerBuildDir $slicerBuildDir) { $fixed = $true }
    }
  } catch {}
  # If Teem QNaN probe present, patch it and try building Teem target first
  if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) {
    $didPatch = Ensure-TeemQnanhibitPatched -SlicerBinDir $env:SLICER_BIN_DIR
    if ($didPatch) {
      [void](Clean-TeemInitialCaches -SlicerBinDir $env:SLICER_BIN_DIR)
      Write-Host "Rebuilding 'teem' external after patch..." -ForegroundColor Yellow
      if (Test-Path $env:SLICER_BIN_DIR) { cmake --build $env:SLICER_BIN_DIR --target teem -j 1 | Write-Host }
      if ($LASTEXITCODE -eq 0) { $fixed = $true }
    }
  }
  # If still failing, try short-drive Slicer build to avoid rsp/path issues
  if (-not $fixed -and $AutoShortDriveSlicer -and $slicerBuildDir -and (Test-Path $slicerBuildDir)) {
    $shortOk = Invoke-SlicerShortDriveBuild -SlicerBuildDir $slicerBuildDir -Jobs $Jobs -PreferredLetter $ShortDriveLetter -KeepMapping:$KeepShortDriveMapping
    if ($shortOk) { $fixed = $true }
  }

  # If inner Slicer-build may be in a bad multi-configured state, drop and reconfigure it once
  if (-not $fixed) {
    try {
      $innerDir = if ($UseSharedSlicer -and $env:SLICER_BIN_DIR) { $env:SLICER_BIN_DIR } else { Join-Path $buildDir 'Slicer-build' }
      if (Test-Path $innerDir) {
        Write-Host "Resetting inner Slicer-build (to avoid 'multiple rules generate ...')" -ForegroundColor Yellow
        Remove-Item -LiteralPath $innerDir -Recurse -Force -ErrorAction SilentlyContinue
        New-Item -ItemType Directory -Force -Path $innerDir | Out-Null
        # Re-enable response files right away
        [void](Ensure-SlicerBuildResponseFiles -RootBuildDir $buildDir)
        $fixed = $true
      }
    } catch {}
  }
  if ($fixed) {
    Write-Host "Retrying build after applying remediation..." -ForegroundColor Yellow
    # Attempt to sanitize Ninja file for known duplicate alias before retry
    try { [void](Fix-NinjaDuplicateAliases -SlicerBuildDir (Join-Path $buildDir 'Slicer-build')) } catch {}
    # Always build by absolute binary dir to honor -BuildRoot overrides
    Invoke-BuildOnce -Dir $buildDir -Jobs $Jobs
  }
}

if ($LASTEXITCODE -ne 0) { throw "Build failed." }

if ($Package) {
  Write-Host "Packaging (target 'package') ..." -ForegroundColor Green
  if ($effectivePreset -like 'vs17*') {
    cmake --build --preset $buildPreset --target package | Write-Host
  } else {
    cmake --build $buildDir --target package | Write-Host
  }
}

Write-Host "Done." -ForegroundColor Cyan
# 稳定化下载（Git/SSH/并发）
[switch]$StabilizeDownloads = $true,
[switch]$SSHOver443
)
# 下载稳定化（可选，默认启用）
if ($StabilizeDownloads) {
  try {
    & (Join-Path $PSScriptRoot 'Setup-DownloadStability.ps1') -Apply -UseGitSSHRewrite -GitHttp11 -GitMaxRequests 2 -GitCompression 0 -SSHOver443:$SSHOver443 | Out-Null
  } catch {
    Write-Warning ("下载稳定化步骤未成功: {0}" -f $_.Exception.Message)
  }
}
