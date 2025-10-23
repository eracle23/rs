<#
.SYNOPSIS
  One-click bootstrap of Windows build prerequisites using Chocolatey.

.DESCRIPTION
  Installs Ninja, sccache, and NSIS via Chocolatey by default.
  Optionally installs CMake and Git. Can auto-install Chocolatey and auto-elevate.

.EXAMPLE
  pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey

.EXAMPLE
  pwsh Tools/Bootstrap-Prereqs.ps1 -All

#>
param(
  [switch]$Ninja = $true,
  [switch]$Sccache = $true,
  [switch]$NSIS = $true,
  [switch]$CMake = $false,
  [switch]$Git = $false,
  [switch]$VSBuildTools = $false,
  [switch]$WindowsSDK = $false,
  [switch]$All = $false,

  [string]$NsisVersion,
  [string]$CMakeVersion,

  [switch]$InstallChocolatey,
  [switch]$AutoElevate
)

$ErrorActionPreference = 'Stop'

function Write-Header($text) {
  Write-Host "`n== $text ==" -ForegroundColor Cyan
}

function Test-Admin {
  $currentIdentity = [Security.Principal.WindowsIdentity]::GetCurrent()
  $principal = New-Object Security.Principal.WindowsPrincipal($currentIdentity)
  return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Ensure-Admin {
  if (-not (Test-Admin)) {
    if ($AutoElevate) {
      Write-Host "Elevating to Administrator..." -ForegroundColor Yellow
      $psCmd = Get-Command pwsh -ErrorAction SilentlyContinue
      if ($psCmd) { $ps = $psCmd.Source } else { $ps = (Get-Command powershell -ErrorAction Stop).Source }
      $argsList = @('-NoProfile','-ExecutionPolicy','Bypass','-File',("`"{0}`"" -f $MyInvocation.MyCommand.Path))
  if ($Ninja) { $argsList += '-Ninja' }
  if ($Sccache) { $argsList += '-Sccache' }
  if ($NSIS) { $argsList += '-NSIS' }
  if ($CMake) { $argsList += '-CMake' }
  if ($Git) { $argsList += '-Git' }
  if ($VSBuildTools) { $argsList += '-VSBuildTools' }
  if ($WindowsSDK) { $argsList += '-WindowsSDK' }
  if ($All) { $argsList += '-All' }
      if ($InstallChocolatey) { $argsList += '-InstallChocolatey' }
      if ($NsisVersion) { $argsList += @('-NsisVersion',$NsisVersion) }
      if ($CMakeVersion) { $argsList += @('-CMakeVersion',$CMakeVersion) }
      Start-Process -FilePath $ps -ArgumentList $argsList -Verb RunAs
      exit 0
    } else {
      throw "Administrator privileges required. Re-run with elevated PowerShell or pass -AutoElevate."
    }
  }
}

function Ensure-Choco {
  if (Get-Command choco -ErrorAction SilentlyContinue) { return }
  if (-not $InstallChocolatey) {
    throw "Chocolatey not found. Re-run with -InstallChocolatey to install it automatically, or install manually from https://chocolatey.org/install"
  }
  Write-Header "Installing Chocolatey"
  Set-ExecutionPolicy Bypass -Scope Process -Force
  [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12
  Invoke-Expression ((Invoke-WebRequest -UseBasicParsing 'https://community.chocolatey.org/install.ps1').Content)
  $env:ChocolateyInstall = [string]$env:ChocolateyInstall
  if (-not $env:ChocolateyInstall) { $env:ChocolateyInstall = "$env:ProgramData\chocolatey" }
  $chocoBin = Join-Path $env:ChocolateyInstall 'bin'
  if ($env:PATH -notlike "*${chocoBin}*") { $env:PATH = "$chocoBin;$env:PATH" }
}

function Install-ChocoPackage {
  param(
    [Parameter(Mandatory=$true)][string]$Name,
    [string]$Version
  )
  $args = @('upgrade', $Name, '-y', '--no-progress', '--allow-downgrade')
  if ($Version) { $args += @('--version', $Version) }
  Write-Host "choco $($args -join ' ')" -ForegroundColor DarkGray
  choco @args
}

if ($All) {
  $CMake = $true
  $Git = $true
}

Write-Header "Bootstrap: prerequisites via Chocolatey"
Ensure-Admin
Ensure-Choco

Write-Header "Installing core tools"
if ($Ninja)   { Install-ChocoPackage -Name 'ninja' }
if ($Sccache) { Install-ChocoPackage -Name 'sccache' }
if ($NSIS)    { Install-ChocoPackage -Name 'nsis' -Version $NsisVersion }

Write-Header "Optional tools"
if ($CMake)   { Install-ChocoPackage -Name 'cmake' -Version $CMakeVersion }
if ($Git)     { Install-ChocoPackage -Name 'git' }
if ($VSBuildTools) {
  Write-Header "Installing Visual Studio 2022 Build Tools (C++ + Windows 10 SDK)"
  $pp = "'--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows10SDK.19041 --includeRecommended --includeOptional --passive --norestart'"
  choco upgrade visualstudio2022buildtools -y --no-progress --package-parameters $pp
}
if ($WindowsSDK) {
  Write-Header "Installing Windows 10 SDK (explicit)"
  choco upgrade windows-sdk-10.0 -y --no-progress
}

Write-Header "Summary"
$ninjaCmd   = Get-Command ninja -ErrorAction SilentlyContinue
$sccacheCmd = Get-Command sccache -ErrorAction SilentlyContinue
$nsisCmd    = Get-Command makensis -ErrorAction SilentlyContinue
$cmakeCmd   = Get-Command cmake -ErrorAction SilentlyContinue
$gitCmd     = Get-Command git -ErrorAction SilentlyContinue
Write-Host ("Ninja:    {0}" -f ($(if ($ninjaCmd) {$ninjaCmd.Source} else {'<not found>'})))
Write-Host ("sccache:  {0}" -f ($(if ($sccacheCmd) {$sccacheCmd.Source} else {'<not found>'})))
Write-Host ("NSIS:     {0}" -f ($(if ($nsisCmd) {$nsisCmd.Source} else {'<not found>'})))
Write-Host ("CMake:    {0}" -f ($(if ($cmakeCmd) {$cmakeCmd.Source} else {'<not found>'})))
Write-Host ("Git:      {0}" -f ($(if ($gitCmd) {$gitCmd.Source} else {'<not found>'})))

Write-Host "`nDone. Open a new terminal if tools aren't visible in PATH." -ForegroundColor Green
