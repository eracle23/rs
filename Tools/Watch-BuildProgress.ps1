<#
.SYNOPSIS
  Monitor RadianceSuite build progress

.DESCRIPTION
  Display build log every 10 seconds until Ctrl+C

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File Tools/Watch-BuildProgress.ps1
#>

param(
    [string]$LogFile = "D:/work/RS/build.log",
    [int]$Interval = 10,
    [int]$TailLines = 20
)

$startTime = Get-Date

while ($true) {
    Clear-Host
    
    $elapsed = (Get-Date) - $startTime
    $elapsedStr = "{0:hh\:mm\:ss}" -f $elapsed
    
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  RadianceSuite Build Monitor" -ForegroundColor Cyan
    Write-Host "  Elapsed: $elapsedStr" -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Cyan
    
    # Check MSBuild process
    $msbuildProcs = Get-Process -Name "MSBuild" -ErrorAction SilentlyContinue
    if ($msbuildProcs) {
        Write-Host "Status: " -NoNewline
        Write-Host "Building..." -ForegroundColor Green
        Write-Host "MSBuild processes: $($msbuildProcs.Count)" -ForegroundColor Gray
    }
    else {
        Write-Host "Status: " -NoNewline
        Write-Host "Build may have completed or not started" -ForegroundColor Yellow
    }
    
    # Show log file size
    if (Test-Path $LogFile) {
        $logSize = (Get-Item $LogFile).Length / 1MB
        Write-Host ("Log size: {0:N2} MB" -f $logSize) -ForegroundColor Gray
    }
    
    Write-Host "----------------------------------------" -ForegroundColor DarkGray
    Write-Host "Latest log (last $TailLines lines):" -ForegroundColor Cyan
    Write-Host ""
    
    if (Test-Path $LogFile) {
        $content = Get-Content $LogFile -Tail $TailLines -ErrorAction SilentlyContinue
        foreach ($line in $content) {
            if ($line -match "error|Error|ERROR") {
                Write-Host $line -ForegroundColor Red
            }
            elseif ($line -match "warning|Warning|WARNING") {
                Write-Host $line -ForegroundColor Yellow
            }
            elseif ($line -match "Completed|SUCCESS") {
                Write-Host $line -ForegroundColor Green
            }
            elseif ($line -match "Building|Compiling") {
                Write-Host $line -ForegroundColor Cyan
            }
            else {
                Write-Host $line
            }
        }
    }
    else {
        Write-Host "Log file not found: $LogFile" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "----------------------------------------" -ForegroundColor DarkGray
    Write-Host "Next refresh: ${Interval}s | Press Ctrl+C to exit" -ForegroundColor DarkGray
    
    Start-Sleep -Seconds $Interval
}
