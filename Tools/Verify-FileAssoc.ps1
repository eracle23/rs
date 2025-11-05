<#
.SYNOPSIS
  验证 Alice 的文件关联注册是否完整（HKCU/HKLM）。

.USAGE
  pwsh Tools/Verify-FileAssoc.ps1
#>

[CmdletBinding()]
param()

function Test-Value {
  param(
    [Parameter(Mandatory)] [string]$Path,
    [Parameter(Mandatory)] [string]$Name
  )
  try {
    $v = Get-ItemProperty -Path $Path -Name $Name -ErrorAction Stop | Select-Object -ExpandProperty $Name
    return @{ Ok=$true; Value=$v }
  } catch {
    return @{ Ok=$false; Value=$null }
  }
}

function Show-Check($label, $ok, $extra) {
  if ($ok) {
    Write-Host "[OK]  $label" -ForegroundColor Green
  } else {
    Write-Host "[NG]  $label" -ForegroundColor Yellow
    if ($extra) { Write-Host "      -> $extra" -ForegroundColor DarkYellow }
  }
}

Write-Host "== 检查 HKCU（当前用户） ==" -ForegroundColor Cyan

$ok1 = (Test-Path "HKCU:\Software\Classes\Applications\Alice.exe\SupportedTypes")
Show-Check 'Applications\Alice.exe\SupportedTypes' $ok1 $null

$owp1 = (Test-Path "HKCU:\Software\Classes\.mrml\OpenWithProgids")
$owp2 = (Test-Path "HKCU:\Software\Classes\.mrb\OpenWithProgids")
Show-Check '.mrml\OpenWithProgids' $owp1 $null
Show-Check '.mrb\OpenWithProgids'  $owp2 $null

$cmd1 = Test-Path "HKCU:\Software\Classes\Alice.MRML\shell\open\command"
$cmd2 = Test-Path "HKCU:\Software\Classes\Alice.MRB\shell\open\command"
Show-Check 'Alice.MRML\shell\open\command' $cmd1 $null
Show-Check 'Alice.MRB\shell\open\command'  $cmd2 $null

$regapp = Test-Value -Path "HKCU:\Software\RegisteredApplications" -Name 'Alice'
Show-Check 'RegisteredApplications\Alice' $regapp.Ok $null

Write-Host "== 检查 HKLM（机器级，如管理员写入） ==" -ForegroundColor Cyan

$ml1 = (Test-Path "HKLM:\Software\Classes\Applications\Alice.exe\SupportedTypes")
Show-Check 'HKLM Applications\Alice.exe\SupportedTypes' $ml1 $null

$ml2 = (Test-Path "HKLM:\Software\RegisteredApplications") -and (Get-ItemProperty HKLM:\Software\RegisteredApplications -Name Alice -ErrorAction SilentlyContinue)
Show-Check 'HKLM RegisteredApplications\Alice' $ml2 $null

Write-Host "\n提示：若以上均为 OK，但设置页仍未出现推荐项，请关闭并重新打开“设置/资源管理器”，或注销后再试。" -ForegroundColor Gray

