@echo off
REM 完全清理并重新开始构建（适用于 CMake 版本变更）
REM 使用前请确保已设置 VS 2022 x64 环境

setlocal EnableExtensions EnableDelayedExpansion

echo ========================================
echo 清理并重新开始构建
echo ========================================
echo.

REM 检查构建目录
set BUILD_DIR=V:\b
set SOURCE_DIR=E:\GitHub\rs0\rs

if not exist "%BUILD_DIR%" (
    echo 构建目录不存在: %BUILD_DIR%
    echo 将创建新的构建目录
    echo.
) else (
    echo 警告: 将删除构建目录: %BUILD_DIR%
    echo 这包括所有已构建的外部依赖（VTK, ITK, CTK 等）
    echo.
    set /p CONFIRM="确认删除? (Y/N): "
    if /i not "!CONFIRM!"=="Y" (
        echo 取消操作
        exit /b 0
    )
)

REM 检测 Visual Studio 2022
set VS2022_PATH=
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS2022_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set VS2022_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set VS2022_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise
)

if "%VS2022_PATH%"=="" (
    echo 错误: 未找到 Visual Studio 2022
    pause
    exit /b 1
)

echo 设置 VS 2022 x64 环境...
call "%VS2022_PATH%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 (
    echo 错误: VS 环境设置失败
    pause
    exit /b 1
)

echo.
echo 检查工具...
where cmake.exe
where ninja.exe
echo.

REM 清理构建目录
echo ========================================
echo 步骤 1: 清理构建目录
echo ========================================
if exist "%BUILD_DIR%" (
    echo 删除: %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%" 2>nul
    if errorlevel 1 (
        echo 警告: 删除失败，某些文件可能被占用
        echo 请关闭可能正在使用这些文件的程序（VS, CMake GUI 等）
        pause
        exit /b 1
    )
    echo 清理完成
) else (
    echo 构建目录不存在，跳过清理
)

echo.
echo ========================================
echo 步骤 2: 创建新的构建目录
echo ========================================
mkdir "%BUILD_DIR%" 2>nul
cd /d "%BUILD_DIR%"
echo 当前目录: %CD%
echo.

echo ========================================
echo 步骤 3: 配置 CMake
echo ========================================
echo 源目录: %SOURCE_DIR%
echo 构建目录: %BUILD_DIR%
echo.

REM 检查 CMake 版本
cmake --version
echo.

REM 配置 CMake（使用 Ninja）
echo 运行 CMake 配置...
cmake -G Ninja ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_INSTALL_PREFIX=C:/S/rs-install ^
    -DSlicer_SUPERBUILD=ON ^
    -DSlicer_BUILD_TESTING=OFF ^
    -DSlicer_BUILD_QTSCRIPTEDMODULES=ON ^
    -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT=ON ^
    -DQt5_DIR=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
    "%SOURCE_DIR%"

if errorlevel 1 (
    echo.
    echo 错误: CMake 配置失败
    echo 请检查上面的错误信息
    pause
    exit /b 1
)

echo.
echo ========================================
echo 配置成功！
echo ========================================
echo.
echo 下一步: 运行构建命令
echo   ninja -j8
echo.
echo 或者使用 VS/MSBuild:
echo   cmake --build . --config RelWithDebInfo -- /m
echo.

pause
