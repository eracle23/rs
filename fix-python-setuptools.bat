@echo off
REM 修复 python-setuptools 安装失败问题
REM 删除已生成的安装脚本，让 CMake 使用新的配置重新生成

echo 正在清理 python-setuptools 的生成文件...

cd /d V:\b

REM 删除 python-setuptools 的生成文件
if exist "slicersources-build\python-setuptools-prefix" (
    echo 删除 python-setuptools-prefix 目录...
    rmdir /s /q "slicersources-build\python-setuptools-prefix"
)

if exist "python-setuptools-requirements.txt" (
    echo 删除 python-setuptools-requirements.txt...
    del "python-setuptools-requirements.txt"
)

echo.
echo 清理完成！
echo.
echo 现在可以继续构建：
echo   ninja -j8
echo.
echo 或者重新配置（推荐）：
echo   del CMakeCache.txt
echo   cmake -G Ninja ...
echo.

pause
