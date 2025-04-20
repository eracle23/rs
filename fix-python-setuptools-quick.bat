@echo off
REM 快速修复：手动修改生成的安装脚本（临时方案）
REM 如果重新配置不可行，可以使用此方案

echo ========================================
echo 快速修复 python-setuptools（手动修改脚本）
echo ========================================
echo.

cd /d V:\b

set SCRIPT_DIR=V:\b\slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp
set SCRIPT_DEBUG=%SCRIPT_DIR%\python-setuptools-install-Debug.cmake
set SCRIPT_RELWITHDEBINFO=%SCRIPT_DIR%\python-setuptools-install-RelWithDebInfo.cmake
set SCRIPT_RELEASE=%SCRIPT_DIR%\python-setuptools-install-Release.cmake

echo 正在修改生成的安装脚本...
echo.

REM 修改 Debug 配置
if exist "%SCRIPT_DEBUG%" (
    echo 修改 Debug 配置脚本...
    powershell -Command "(Get-Content '%SCRIPT_DEBUG%') -replace 'pip;install;--require-hashes', 'pip;install;--trusted-host;mirrors.aliyun.com;--index-url;https://mirrors.aliyun.com/pypi/simple/;--require-hashes' | Set-Content '%SCRIPT_DEBUG%'"
    echo   ✓ Debug 配置已修改
)

REM 修改 RelWithDebInfo 配置
if exist "%SCRIPT_RELWITHDEBINFO%" (
    echo 修改 RelWithDebInfo 配置脚本...
    powershell -Command "(Get-Content '%SCRIPT_RELWITHDEBINFO%') -replace 'pip;install;--require-hashes', 'pip;install;--trusted-host;mirrors.aliyun.com;--index-url;https://mirrors.aliyun.com/pypi/simple/;--require-hashes' | Set-Content '%SCRIPT_RELWITHDEBINFO%'"
    echo   ✓ RelWithDebInfo 配置已修改
)

REM 修改 Release 配置
if exist "%SCRIPT_RELEASE%" (
    echo 修改 Release 配置脚本...
    powershell -Command "(Get-Content '%SCRIPT_RELEASE%') -replace 'pip;install;--require-hashes', 'pip;install;--trusted-host;mirrors.aliyun.com;--index-url;https://mirrors.aliyun.com/pypi/simple/;--require-hashes' | Set-Content '%SCRIPT_RELEASE%'"
    echo   ✓ Release 配置已修改
)

echo.
echo 修改完成！
echo.
echo 现在可以构建 python-setuptools:
echo   ninja python-setuptools
echo.
echo 注意: 如果重新运行 cmake 配置，这些修改会被覆盖
echo.

pause
