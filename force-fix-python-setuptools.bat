@echo off
REM 强制修复 python-setuptools - 直接修改生成的脚本文件

echo ========================================
echo 强制修复 python-setuptools
echo ========================================
echo.

cd /d V:\b

set SCRIPT_DIR=slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp

if not exist "%SCRIPT_DIR%" (
    echo 错误: 脚本目录不存在
    echo 请先运行构建，让 CMake 生成脚本文件
    pause
    exit /b 1
)

echo 正在修改所有配置的安装脚本...
echo.

REM 修改 Debug 配置
if exist "%SCRIPT_DIR%\python-setuptools-install-Debug.cmake" (
    echo [1/3] 修改 Debug 配置...
    powershell -NoProfile -Command "$file = '%SCRIPT_DIR%\python-setuptools-install-Debug.cmake'; $content = Get-Content $file -Raw; $content = $content -replace 'pip;install;--require-hashes', 'pip;install;--trusted-host;mirrors.aliyun.com;--index-url;https://mirrors.aliyun.com/pypi/simple/;--require-hashes'; Set-Content $file $content -NoNewline"
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ Debug 配置已修改
    ) else (
        echo   ✗ Debug 配置修改失败
    )
)

REM 修改 RelWithDebInfo 配置
if exist "%SCRIPT_DIR%\python-setuptools-install-RelWithDebInfo.cmake" (
    echo [2/3] 修改 RelWithDebInfo 配置...
    powershell -NoProfile -Command "$file = '%SCRIPT_DIR%\python-setuptools-install-RelWithDebInfo.cmake'; $content = Get-Content $file -Raw; $content = $content -replace 'pip;install;--require-hashes', 'pip;install;--trusted-host;mirrors.aliyun.com;--index-url;https://mirrors.aliyun.com/pypi/simple/;--require-hashes'; Set-Content $file $content -NoNewline"
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ RelWithDebInfo 配置已修改
    ) else (
        echo   ✗ RelWithDebInfo 配置修改失败
    )
)

REM 修改 Release 配置
if exist "%SCRIPT_DIR%\python-setuptools-install-Release.cmake" (
    echo [3/3] 修改 Release 配置...
    powershell -NoProfile -Command "$file = '%SCRIPT_DIR%\python-setuptools-install-Release.cmake'; $content = Get-Content $file -Raw; $content = $content -replace 'pip;install;--require-hashes', 'pip;install;--trusted-host;mirrors.aliyun.com;--index-url;https://mirrors.aliyun.com/pypi/simple/;--require-hashes'; Set-Content $file $content -NoNewline"
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ Release 配置已修改
    ) else (
        echo   ✗ Release 配置修改失败
    )
)

echo.
echo 验证修改...
if exist "%SCRIPT_DIR%\python-setuptools-install-Debug.cmake" (
    findstr /C:"mirrors.aliyun.com" "%SCRIPT_DIR%\python-setuptools-install-Debug.cmake" >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo ✓ 脚本已包含阿里云镜像配置
    ) else (
        echo ✗ 警告: 脚本可能未正确修改
    )
)

echo.
echo ========================================
echo 修改完成！
echo ========================================
echo.
echo 现在可以构建 python-setuptools:
echo   ninja python-setuptools
echo.
echo 或者继续完整构建:
echo   ninja -j8
echo.

pause
