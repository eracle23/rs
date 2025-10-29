param(
  [Parameter(Mandatory = $true)]
  [string]$SlicerBuild,

  [int]$PyVer = 312,

  [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $SlicerBuild)) {
  throw "Slicer build directory not found: $SlicerBuild"
}

# Destination (the directory the linker searches via /LIBPATH)
$destDir = [IO.Path]::Combine($SlicerBuild, 'python-build', 'CMakeBuild', 'libpython', 'Release')

# Source directories that typically contain python312(.lib)
$srcDirs = @(
  [IO.Path]::Combine($SlicerBuild, 'python-build', 'libs'),
  [IO.Path]::Combine($SlicerBuild, 'python-install', 'libs')
)

$releaseLib  = "python$PyVer.lib"
$releaseAlias = 'python3.lib'
$debugLib    = "python{0}_d.lib" -f $PyVer
$debugAlias  = 'python3_d.lib'

function Ensure-Dir([string]$Dir) {
  if (-not (Test-Path -LiteralPath $Dir)) {
    if ($DryRun) { Write-Host "[DryRun] mkdir $Dir" -ForegroundColor Yellow }
    else { New-Item -ItemType Directory -Force -Path $Dir | Out-Null }
  }
}

function Copy-Alias([string]$SrcFile, [string]$DstFile) {
  if (-not (Test-Path -LiteralPath $SrcFile)) { return $false }
  if (Test-Path -LiteralPath $DstFile) { return $false }
  if ($DryRun) {
    Write-Host "[DryRun] copy `"$SrcFile`" -> `"$DstFile`"" -ForegroundColor Yellow
  } else {
    Copy-Item -Force -LiteralPath $SrcFile -Destination $DstFile
    Write-Host "Created alias: $DstFile" -ForegroundColor Green
  }
  return $true
}

# Ensure destination directory exists
Ensure-Dir $destDir

$made = $false

# 1) Create alias in destination dir from any available source dir
foreach ($sd in $srcDirs) {
  $srcRelease = [IO.Path]::Combine($sd, $releaseLib)
  $dstRelease = [IO.Path]::Combine($destDir, $releaseAlias)
  if (Copy-Alias $srcRelease $dstRelease) { $made = $true; break }
}
foreach ($sd in $srcDirs) {
  $srcDebug = [IO.Path]::Combine($sd, $debugLib)
  $dstDebug = [IO.Path]::Combine($destDir, $debugAlias)
  if (Copy-Alias $srcDebug $dstDebug) { $made = $true; break }
}

# 2) Also create alias in source dirs themselves (some link steps search there, too)
foreach ($sd in $srcDirs) {
  if (Test-Path -LiteralPath $sd) {
    $null = Copy-Alias ([IO.Path]::Combine($sd, $releaseLib)) ([IO.Path]::Combine($sd, $releaseAlias))
    $null = Copy-Alias ([IO.Path]::Combine($sd, $debugLib))   ([IO.Path]::Combine($sd, $debugAlias))
  }
}

if (-not $made) {
  Write-Warning ("No alias created in destination dir: {0}. Ensure {1} exists in: {2}" -f $destDir, $releaseLib, ($srcDirs -join '; '))
  Write-Host   ("Tip: Build Python core first, then re-run this script:`n  cmake --build `"{0}`" -j 0 --target pythoncore" -f $SlicerBuild) -ForegroundColor Yellow
} else {
  Write-Host "Done. Rebuild Python target: `n  cmake --build `"$SlicerBuild`" -j 0 --target python" -ForegroundColor Cyan
}
