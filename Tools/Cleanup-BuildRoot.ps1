param(
  [string[]]$Roots,
  [string[]]$DriveLetters = @('Q','R'),
  [switch]$Aggressive,
  [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

function Stop-AppLockers {
  param([string[]]$Names)
  foreach ($name in $Names) {
    $procs = Get-Process -Name $name -ErrorAction SilentlyContinue
    foreach ($p in $procs) {
      Write-Host ("Stopping process {0} (PID {1})" -f $p.ProcessName, $p.Id) -ForegroundColor Yellow
      try { $null = $p.CloseMainWindow() } catch {}
      Start-Sleep -Milliseconds 200
      try { if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } } catch {}
    }
  }
}

function Unmap-ShortDrives {
  param([string[]]$Letters)
  foreach($l in $Letters){
    $l = $l.TrimEnd(':')
    if (-not $l) { continue }
    try { Write-Host ("Unmapping {0}:" -f $l) -ForegroundColor Yellow; cmd /c ("subst {0}: /D" -f $l) | Out-Null } catch {}
  }
}

function Reset-Integrity($Path){
  try {
    if (-not (Test-Path $Path)) { return }
    icacls $Path /setintegritylevel "(OI)(CI)M" | Out-Null
  } catch {}
}

Write-Host 'Terminating build-related processes...' -ForegroundColor Cyan
Stop-AppLockers -Names @('Alice','AliceApp-real','SlicerApp-real','CTKAppLauncher','cmake','ninja','python','git','ctest')

Write-Host 'Unmapping short drives...' -ForegroundColor Cyan
Unmap-ShortDrives -Letters $DriveLetters

foreach($root in $Roots){
  if (-not $root) { continue }
  if (-not (Test-Path $root)) { continue }
  $stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
  $dest = "$root.bak_$stamp"
  Write-Host ("Renaming {0} -> {1}" -f $root,$dest) -ForegroundColor Yellow
  try {
    if (-not $DryRun) {
      Reset-Integrity -Path $root
      Rename-Item -LiteralPath $root -NewName (Split-Path $dest -Leaf) -ErrorAction Stop
    }
  } catch {
    Write-Warning ("Rename failed: {0}" -f $_.Exception.Message)
    if ($Aggressive -and -not $DryRun) {
      Write-Host 'Attempting force removal (may require elevation) ...' -ForegroundColor Yellow
      try { Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction Stop }
      catch { Write-Warning ("Force remove failed: {0}" -f $_.Exception.Message) }
    }
  }
}

Write-Host 'Done.' -ForegroundColor Green

