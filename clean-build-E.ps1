# 彻底清理 E:\build 并重新配置（解决 C:/b/slicersources-src 缓存残留）
# 在 PowerShell 中运行: .\clean-build-E.ps1

$buildDir = "E:\build"
$sourceDir = "E:\GitHub\rs0\rs"
$slicerLocal = "C:/b/slicersources-src"

Write-Host "=== 清理 E:\build 并重新配置 ===" -ForegroundColor Cyan

# 1. 删除整个构建目录
if (Test-Path $buildDir) {
    Write-Host "删除 $buildDir ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
    Write-Host "已删除。" -ForegroundColor Green
} else {
    Write-Host "构建目录不存在，跳过删除。" -ForegroundColor Gray
}

# 2. 重新创建并配置
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
Set-Location $buildDir

Write-Host "`n运行 cmake（使用 C:/b/slicersources-src）..." -ForegroundColor Yellow
& cmake -Dslicersources_SOURCE_DIR=$slicerLocal -Dslicersources_BINARY_DIR=$buildDir/slicersources-subbuild $sourceDir

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n配置成功。请用 Visual Studio 打开 $buildDir\VisionMagicEcosystem.sln 进行生成。" -ForegroundColor Green
} else {
    Write-Host "`n配置失败，请检查上方错误信息。" -ForegroundColor Red
}
