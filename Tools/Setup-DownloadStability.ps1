param(
  [switch]$Apply = $true,
  [switch]$UseGitSSHRewrite = $true,
  [switch]$SSHOver443,
  [switch]$GitHttp11 = $true,
  [int]$GitMaxRequests = 2,
  [int]$GitCompression = 0,
  [switch]$ShowOnly
)

$ErrorActionPreference = 'Stop'

function Set-GitConfigIfNeeded {
  param([string]$Key,[string]$Value)
  try {
    $cur = git config --global --get $Key 2>$null
  } catch { $cur = $null }
  if ($cur -ne $Value) {
    Write-Host ("git config --global {0} {1}" -f $Key,$Value) -ForegroundColor Yellow
    if ($Apply) { git config --global $Key $Value | Out-Null }
  } else {
    Write-Host ("git config --global {0} 已是 {1}" -f $Key,$Value) -ForegroundColor DarkGray
  }
}

function Ensure-SshOver443 {
  $sshDir = Join-Path $HOME '.ssh'
  if (-not (Test-Path $sshDir)) { if ($Apply) { New-Item -ItemType Directory -Force -Path $sshDir | Out-Null } }
  $cfg = Join-Path $sshDir 'config'
  $block = @(
    'Host github.com',
    '  HostName ssh.github.com',
    '  Port 443',
    '  User git',
    '  IdentityFile ~/.ssh/id_ed25519',
    '  IdentitiesOnly yes',
    '  ServerAliveInterval 30'
  ) -join "`n"
  $needWrite = $true
  if (Test-Path $cfg) {
    $content = Get-Content -Raw -Path $cfg
    if ($content -match '(?ms)^\s*Host\s+github\.com\b') { $needWrite = $false }
  }
  if ($needWrite) {
    Write-Host "写入 ~/.ssh/config 以通过 443 端口使用 GitHub SSH" -ForegroundColor Yellow
    if ($Apply) { Add-Content -Path $cfg -Value ("`n# Added by Setup-DownloadStability`n{0}`n" -f $block) }
  } else {
    Write-Host "~/.ssh/config 已包含 github.com 配置" -ForegroundColor DarkGray
  }
}

Write-Host '应用下载稳定化配置（Git/SSH/并发）...' -ForegroundColor Cyan

if ($UseGitSSHRewrite) {
  Set-GitConfigIfNeeded -Key 'url.ssh://git@github.com/.insteadof' -Value 'https://github.com/'
}

if ($SSHOver443) {
  Ensure-SshOver443
}

if ($GitHttp11) {
  Set-GitConfigIfNeeded -Key 'http.version' -Value 'HTTP/1.1'
}

if ($GitMaxRequests -gt 0) {
  Set-GitConfigIfNeeded -Key 'http.maxRequests' -Value $GitMaxRequests
}

if ($GitCompression -ge 0) {
  Set-GitConfigIfNeeded -Key 'core.compression' -Value $GitCompression
}

Write-Host '完成。' -ForegroundColor Green

