@echo off
REM 单独测试 python-setuptools 构建
REM 用于快速验证修复是否有效

echo ========================================
echo 测试 python-setuptools 构建
echo ========================================
echo.

cd /d V:\b

REM 检查是否在构建目录
if not exist "build.ninja" (
    echo 错误: 当前目录不是构建目录
    echo 请先运行: cd /d V:\b
    pause
    exit /b 1
)

echo 步骤 1: 清理旧的 python-setuptools 生成文件...
if exist "slicersources-build\python-setuptools-prefix" (
    echo   删除 python-setuptools-prefix 目录...
    rmdir /s /q "slicersources-build\python-setuptools-prefix" 2>nul
)
if exist "python-setuptools-requirements.txt" (
    echo   删除 python-setuptools-requirements.txt...
    del "python-setuptools-requirements.txt" 2>nul
)
echo   完成
echo.

echo 步骤 2: 单独构建 python-setuptools...
echo   命令: ninja python-setuptools
echo.
ninja python-setuptools

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo ✓ python-setuptools 构建成功！
    echo ========================================
    echo.
    echo 验证安装...
    V:\b\python-install\bin\PythonSlicer.exe -m pip list | findstr setuptools
    echo.
    echo 现在可以继续完整构建:
    echo   ninja -j8
) else (
    echo.
    echo ========================================
    echo ✗ python-setuptools 构建失败
    echo ========================================
    echo.
    echo 请检查错误信息，可能需要:
    echo 1. 重新配置 CMake
    echo 2. 检查网络连接
    echo 3. 查看日志文件:
    echo    V:\b\slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-*.log
)

echo.
pause
