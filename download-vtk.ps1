# VTK 自动下载脚本
# 用于解决 VTK git clone 失败问题

$ErrorActionPreference = "Stop"

# 配置
$BuildDir = "V:\b"
$VTKRepo = "https://github.com/slicer/VTK.git"
$VTKCommit = "492821449a5f2a9a1f5c73c3c6dd4389f1059d66"
$VTKDir = Join-Path $BuildDir "VTK"

Write-Host "=== VTK 下载脚本 ===" -ForegroundColor Cyan
Write-Host "目标目录: $VTKDir" -ForegroundColor Yellow
Write-Host "Commit: $VTKCommit" -ForegroundColor Yellow
Write-Host ""

# 检查构建目录
if (-not (Test-Path $BuildDir)) {
    Write-Host "错误: 构建目录不存在: $BuildDir" -ForegroundColor Red
    Write-Host "请先设置虚拟驱动器: subst V: C:\S\vs-dev" -ForegroundColor Yellow
    exit 1
}

# 检查 Git
try {
    $gitVersion = git --version
    Write-Host "Git 版本: $gitVersion" -ForegroundColor Green
} catch {
    Write-Host "错误: 未找到 Git，请先安装 Git" -ForegroundColor Red
    exit 1
}

# 如果 VTK 目录已存在
if (Test-Path $VTKDir) {
    Write-Host "检测到 VTK 目录已存在: $VTKDir" -ForegroundColor Yellow
    
    # 检查是否是 Git 仓库
    if (Test-Path (Join-Path $VTKDir ".git")) {
        Write-Host "检查当前 commit..." -ForegroundColor Yellow
        Push-Location $VTKDir
        try {
            $currentCommit = git rev-parse HEAD
            if ($currentCommit -eq $VTKCommit) {
                Write-Host "✓ VTK 已是最新版本 (commit: $VTKCommit)" -ForegroundColor Green
                Write-Host "跳过下载" -ForegroundColor Green
                exit 0
            } else {
                Write-Host "当前 commit: $currentCommit" -ForegroundColor Yellow
                Write-Host "需要切换到: $VTKCommit" -ForegroundColor Yellow
                Write-Host "正在切换..." -ForegroundColor Yellow
                git fetch origin
                git checkout $VTKCommit
                Write-Host "✓ 已切换到正确 commit" -ForegroundColor Green
                exit 0
            }
        } catch {
            Write-Host "警告: Git 操作失败，可能需要重新克隆" -ForegroundColor Yellow
            Write-Host "删除现有目录并重新下载..." -ForegroundColor Yellow
            Pop-Location
            Remove-Item -Recurse -Force $VTKDir
        }
    } else {
        Write-Host "目录存在但不是 Git 仓库，删除并重新下载..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force $VTKDir
    }
}

# 下载 VTK
Write-Host "开始克隆 VTK 仓库..." -ForegroundColor Cyan
Write-Host "这可能需要几分钟时间（仓库较大）..." -ForegroundColor Yellow
Write-Host ""

Push-Location $BuildDir
try {
    # 使用浅克隆加快速度（只下载指定 commit）
    Write-Host "使用浅克隆（--depth 1）..." -ForegroundColor Yellow
    git clone --depth 1 $VTKRepo $VTKDir
    
    # 切换到指定 commit
    Write-Host "切换到指定 commit: $VTKCommit" -ForegroundColor Yellow
    Push-Location $VTKDir
    git fetch --depth=100 origin
    git checkout $VTKCommit
    Pop-Location
    
    Write-Host ""
    Write-Host "✓ VTK 下载成功！" -ForegroundColor Green
    Write-Host "目录: $VTKDir" -ForegroundColor Green
    Write-Host ""
    Write-Host "现在可以继续构建:" -ForegroundColor Cyan
    Write-Host "  cd /d V:\b" -ForegroundColor White
    Write-Host "  ninja -j8" -ForegroundColor White
    
} catch {
    Write-Host ""
    Write-Host "错误: 下载失败" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ""
    Write-Host "可能的解决方案:" -ForegroundColor Yellow
    Write-Host "1. 检查网络连接" -ForegroundColor White
    Write-Host "2. 配置 Git 代理（如果有）:" -ForegroundColor White
    Write-Host "   git config --global http.proxy http://proxy:port" -ForegroundColor Gray
    Write-Host "3. 使用 GitHub 镜像（如果可用）" -ForegroundColor White
    Write-Host "4. 手动下载后复制到 $VTKDir" -ForegroundColor White
    Pop-Location
    exit 1
} finally {
    Pop-Location
}
