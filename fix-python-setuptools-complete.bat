@echo off
REM 完整修复 python-setuptools 问题
REM 包括清理和重新配置

echo ========================================
echo 完整修复 python-setuptools
echo ========================================
echo.

cd /d V:\b

REM 步骤 1: 清理旧的生成文件
echo [1/3] 清理旧的生成文件...
if exist "slicersources-build\python-setuptools-prefix" (
    echo   删除 python-setuptools-prefix...
    rmdir /s /q "slicersources-build\python-setuptools-prefix" 2>nul
)
if exist "python-setuptools-requirements.txt" (
    echo   删除 python-setuptools-requirements.txt...
    del "python-setuptools-requirements.txt" 2>nul
)
echo   完成
echo.

REM 步骤 2: 重新配置 CMake（关键步骤！）
echo [2/3] 重新配置 CMake（重新生成安装脚本）...
echo   这很重要：CMake 需要重新读取修改后的 External_python-setuptools.cmake
echo.
cmake .

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo 错误: CMake 配置失败
    echo 请检查错误信息
    pause
    exit /b 1
)

echo   配置完成
echo.

REM 步骤 3: 验证生成的脚本
echo [3/3] 验证生成的安装脚本...
set SCRIPT_FILE=V:\b\slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Debug.cmake
if exist "%SCRIPT_FILE%" (
    findstr /C:"mirrors.aliyun.com" "%SCRIPT_FILE%" >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ 脚本已包含阿里云镜像配置
    ) else (
        echo   ✗ 警告: 脚本可能仍使用旧配置
        echo   请检查: %SCRIPT_FILE%
    )
) else (
    echo   脚本文件不存在（将在构建时生成）
)
echo.

REM 步骤 4: 构建 python-setuptools
echo 开始构建 python-setuptools...
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
    echo 请查看错误信息和日志文件
)

echo.
pause
