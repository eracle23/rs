$stampDir = Join-Path 'C:\RS\build\slicersources-build' 'Slicer-prefix\src\Slicer-stamp'
$logs = Get-ChildItem $stampDir -Recurse -Filter '*Slicer*.log' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime
if (-not $logs) {
    Write-Warning 'No Slicer logs found.'
    return
}
$latest = $logs[-1]
Write-Host "Reading log: $($latest.FullName)" -ForegroundColor Cyan
Get-Content $latest.FullName -Tail 200
