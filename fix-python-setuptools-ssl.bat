@echo off
REM 修复 python-setuptools SSL 连接问题
REM 移除 --require-hashes 参数（临时方案）

echo ========================================
echo 修复 python-setuptools SSL 问题
echo ========================================
echo.

cd /d V:\b

set SCRIPT_DIR=slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp

if not exist "%SCRIPT_DIR%\python-setuptools-install-Debug.cmake" (
    echo 错误: 脚本文件不存在
    pause
    exit /b 1
)

echo 正在修改安装脚本（移除 --require-hashes）...
echo.

REM 修改 Debug 配置 - 移除 --require-hashes
if exist "%SCRIPT_DIR%\python-setuptools-install-Debug.cmake" (
    echo [1/3] 修改 Debug 配置...
    powershell -NoProfile -Command "$file = '%SCRIPT_DIR%\python-setuptools-install-Debug.cmake'; $content = Get-Content $file -Raw; $content = $content -replace ';--require-hashes', ''; Set-Content $file $content -NoNewline"
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ Debug 配置已修改（已移除 --require-hashes）
    ) else (
        echo   ✗ Debug 配置修改失败
    )
)

REM 修改 RelWithDebInfo 配置
if exist "%SCRIPT_DIR%\python-setuptools-install-RelWithDebInfo.cmake" (
    echo [2/3] 修改 RelWithDebInfo 配置...
    powershell -NoProfile -Command "$file = '%SCRIPT_DIR%\python-setuptools-install-RelWithDebInfo.cmake'; $content = Get-Content $file -Raw; $content = $content -replace ';--require-hashes', ''; Set-Content $file $content -NoNewline"
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ RelWithDebInfo 配置已修改
    ) else (
        echo   ✗ RelWithDebInfo 配置修改失败
    )
)

REM 修改 Release 配置
if exist "%SCRIPT_DIR%\python-setuptools-install-Release.cmake" (
    echo [3/3] 修改 Release 配置...
    powershell -NoProfile -Command "$file = '%SCRIPT_DIR%\python-setuptools-install-Release.cmake'; $content = Get-Content $file -Raw; $content = $content -replace ';--require-hashes', ''; Set-Content $file $content -NoNewline"
    if %ERRORLEVEL% EQU 0 (
        echo   ✓ Release 配置已修改
    ) else (
        echo   ✗ Release 配置修改失败
    )
)

echo.
echo ========================================
echo 修改完成！
echo ========================================
echo.
echo 注意: 已移除 --require-hashes 参数（临时方案）
echo 现在可以构建 python-setuptools:
echo   ninja python-setuptools
echo.

pause
