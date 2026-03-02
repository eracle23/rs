@echo off
REM 删除 Slicer 内层构建的 CMake 缓存，解决 "generator platform: x64 Does not match" 错误
REM 在 E:\build 的父目录或任意位置运行均可（使用绝对路径）

set BUILD_DIR=E:\build

echo 正在删除 Slicer 内层构建缓存...
echo.

if exist "%BUILD_DIR%\slicersources-subbuild\Slicer-prefix" (
    rmdir /s /q "%BUILD_DIR%\slicersources-subbuild\Slicer-prefix"
    echo 已删除 slicersources-subbuild\Slicer-prefix
) else (
    echo slicersources-subbuild\Slicer-prefix 不存在，跳过
)

if exist "%BUILD_DIR%\Slicer-build" (
    if exist "%BUILD_DIR%\Slicer-build\CMakeCache.txt" (
        del "%BUILD_DIR%\Slicer-build\CMakeCache.txt"
        echo 已删除 Slicer-build\CMakeCache.txt
    )
    if exist "%BUILD_DIR%\Slicer-build\CMakeFiles" (
        rmdir /s /q "%BUILD_DIR%\Slicer-build\CMakeFiles"
        echo 已删除 Slicer-build\CMakeFiles
    )
) else (
    echo Slicer-build 不存在，跳过
)

echo.
echo 完成。请重新在 VS 中执行「生成」。
pause
