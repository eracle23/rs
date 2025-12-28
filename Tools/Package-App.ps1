# VisionMagic 完整打包脚本
# 复制构建目录中所有需要的文件（基于 Alice 能启动的事实）

param(
    [string]$BuildRoot = "D:\work\RS\rs-build",
    [string]$OutputDir = "D:\work\RS\package-output",
    [string]$AppName = "VisionMagic"
)

$ErrorActionPreference = "Stop"

$SlicerBuild = "$BuildRoot\Slicer-build"
$Version = "1.0.0"
$Date = Get-Date -Format "yyyy-MM-dd"
$PackageName = "$AppName-$Version-$Date-win-amd64"

Write-Host "=== VisionMagic 完整打包脚本 ===" -ForegroundColor Cyan
Write-Host "构建根目录: $BuildRoot"
Write-Host "Slicer构建: $SlicerBuild"
Write-Host "输出目录: $OutputDir"

# 创建目标目录
$TargetDir = Join-Path $OutputDir $PackageName
if (Test-Path $TargetDir) {
    Write-Host "清理旧目录..." -ForegroundColor Yellow
    Remove-Item $TargetDir -Recurse -Force
}
New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null

# ========== 1. 复制启动器和配置 ==========
Write-Host "`n[1/8] 复制启动器..." -ForegroundColor Yellow

# 使用 Alice 启动器（已经证明能工作），但重命名为 VisionMagic
Copy-Item "$SlicerBuild\Alice.exe" "$TargetDir\$AppName.exe" -Force

# 复制 Alice 的 INI 配置，修改为 VisionMagic
$AliceIni = Get-Content "$SlicerBuild\AliceLauncherSettingsToInstall.ini" -Raw
# 替换应用名称和路径
$VisionMagicIni = $AliceIni -replace "Alice-5\.8", "Alice-5.8" `
                            -replace "AliceApp-real\.exe", "AliceApp-real.exe" `
                            -replace "name=Alice", "name=$AppName"
$VisionMagicIni | Set-Content "$TargetDir\$($AppName)LauncherSettings.ini" -Encoding UTF8

# ========== 2. 复制 bin 目录 ==========
Write-Host "[2/8] 复制 bin 目录..." -ForegroundColor Yellow
$BinDst = "$TargetDir\bin"
New-Item -ItemType Directory -Path $BinDst -Force | Out-Null

# 从 Slicer-build\bin\RelWithDebInfo 复制
if (Test-Path "$SlicerBuild\bin\RelWithDebInfo") {
    Copy-Item "$SlicerBuild\bin\RelWithDebInfo\*" $BinDst -Recurse -Force
}

# 从 install\bin 复制（可能有额外的 DLL）
if (Test-Path "$BuildRoot\install\bin") {
    Copy-Item "$BuildRoot\install\bin\*" $BinDst -Recurse -Force -ErrorAction SilentlyContinue
}

# 复制 SplashScreen
if (Test-Path "$SlicerBuild\Alice-SplashScreen.png") {
    Copy-Item "$SlicerBuild\Alice-SplashScreen.png" "$BinDst\SplashScreen.png" -Force
}

# ========== 3. 复制 lib/Alice-5.8 目录 ==========
Write-Host "[3/8] 复制 lib/Alice-5.8..." -ForegroundColor Yellow
$LibDst = "$TargetDir\lib"
New-Item -ItemType Directory -Path $LibDst -Force | Out-Null

if (Test-Path "$SlicerBuild\lib\Alice-5.8") {
    Copy-Item "$SlicerBuild\lib\Alice-5.8" "$LibDst\Alice-5.8" -Recurse -Force
}

# 从 install\lib 复制其他内容
if (Test-Path "$BuildRoot\install\lib") {
    Get-ChildItem "$BuildRoot\install\lib" | ForEach-Object {
        if (-not (Test-Path "$LibDst\$($_.Name)")) {
            Copy-Item $_.FullName $LibDst -Recurse -Force
        }
    }
}

# ========== 4. 复制 Python ==========
Write-Host "[4/8] 复制 Python..." -ForegroundColor Yellow
$PythonDst = "$LibDst\Python"
New-Item -ItemType Directory -Path $PythonDst -Force | Out-Null

# 从 python-install 复制
if (Test-Path "$BuildRoot\python-install") {
    Copy-Item "$BuildRoot\python-install\*" $PythonDst -Recurse -Force
}

# ========== 5. 复制 Qt 插件 ==========
Write-Host "[5/8] 复制 Qt 插件..." -ForegroundColor Yellow
$QtPluginsDst = "$LibDst\QtPlugins"
New-Item -ItemType Directory -Path $QtPluginsDst -Force | Out-Null

# 从 CTK-build 复制 Qt 插件
$CtkPlugins = "$BuildRoot\CTK-build\CTK-build\bin\designer"
if (Test-Path $CtkPlugins) {
    Copy-Item $CtkPlugins "$QtPluginsDst\designer" -Recurse -Force
}

# 尝试从 VTK-build 找 Qt 插件
Get-ChildItem "$BuildRoot" -Directory -Filter "*-build" | ForEach-Object {
    $pluginDir = Join-Path $_.FullName "bin\RelWithDebInfo\plugins"
    if (Test-Path $pluginDir) {
        Get-ChildItem $pluginDir -Directory | ForEach-Object {
            if (-not (Test-Path "$QtPluginsDst\$($_.Name)")) {
                Copy-Item $_.FullName $QtPluginsDst -Recurse -Force
            }
        }
    }
}

# ========== 6. 复制 vtkTeem ==========
Write-Host "[6/8] 复制 vtkTeem..." -ForegroundColor Yellow
if (Test-Path "$BuildRoot\teem-build") {
    $VtkTeemDst = "$LibDst\vtkTeem"
    New-Item -ItemType Directory -Path $VtkTeemDst -Force | Out-Null
    Get-ChildItem "$BuildRoot\teem-build" -Filter "*.dll" -Recurse | ForEach-Object {
        Copy-Item $_.FullName $VtkTeemDst -Force
    }
    Get-ChildItem "$BuildRoot\teem-build" -Filter "*.pyd" -Recurse | ForEach-Object {
        Copy-Item $_.FullName $VtkTeemDst -Force
    }
}

# ========== 7. 复制 share 目录 ==========
Write-Host "[7/8] 复制 share 目录..." -ForegroundColor Yellow
if (Test-Path "$SlicerBuild\share") {
    Copy-Item "$SlicerBuild\share" $TargetDir -Recurse -Force
}

# ========== 8. 复制 libexec 目录 ==========
Write-Host "[8/8] 复制 libexec 目录..." -ForegroundColor Yellow
if (Test-Path "$SlicerBuild\libexec") {
    Copy-Item "$SlicerBuild\libexec" $TargetDir -Recurse -Force
}

# ========== 复制额外的依赖 DLL ==========
Write-Host "`n复制额外依赖..." -ForegroundColor Yellow

# 从各个 -install 目录复制 DLL
$InstallDirs = @(
    "$BuildRoot\curl-install\bin",
    "$BuildRoot\LibArchive-install\bin",
    "$BuildRoot\OpenSSL-install\bin",
    "$BuildRoot\sqlite-install\bin",
    "$BuildRoot\zlib-install\bin",
    "$BuildRoot\bzip2-install\bin",
    "$BuildRoot\LZMA-install\bin",
    "$BuildRoot\tbb-install\bin"
)

foreach ($dir in $InstallDirs) {
    if (Test-Path $dir) {
        Get-ChildItem $dir -Filter "*.dll" | ForEach-Object {
            Copy-Item $_.FullName $BinDst -Force -ErrorAction SilentlyContinue
        }
    }
}

# 从各个 -build 目录复制 DLL
$BuildDirs = @(
    "$BuildRoot\VTK-build\bin\RelWithDebInfo",
    "$BuildRoot\ITK-build\bin\RelWithDebInfo",
    "$BuildRoot\CTK-build\CTK-build\bin\RelWithDebInfo",
    "$BuildRoot\SimpleITK-build\bin\RelWithDebInfo",
    "$BuildRoot\teem-build\bin\RelWithDebInfo"
)

foreach ($dir in $BuildDirs) {
    if (Test-Path $dir) {
        Get-ChildItem $dir -Filter "*.dll" | ForEach-Object {
            Copy-Item $_.FullName $BinDst -Force -ErrorAction SilentlyContinue
        }
    }
}

# ========== 创建 ZIP ==========
Write-Host "`n创建 ZIP 包..." -ForegroundColor Yellow
$ZipPath = Join-Path $OutputDir "$PackageName.zip"
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}
Compress-Archive -Path $TargetDir -DestinationPath $ZipPath -Force

# 显示结果
$ZipSize = (Get-Item $ZipPath).Length / 1MB
$FileCount = (Get-ChildItem $TargetDir -Recurse -File).Count
Write-Host "`n=== 打包完成 ===" -ForegroundColor Green
Write-Host "ZIP 包: $ZipPath"
Write-Host "大小: $([math]::Round($ZipSize, 2)) MB"
Write-Host "文件数: $FileCount"
Write-Host "`n目录结构:"
Get-ChildItem $TargetDir | ForEach-Object { Write-Host "  $($_.Name)" }
Write-Host "`n测试命令:"
Write-Host "  Expand-Archive '$ZipPath' -DestinationPath 'D:\test-pkg' -Force"
Write-Host "  Start-Process 'D:\test-pkg\$PackageName\$AppName.exe'"
