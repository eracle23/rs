@echo off
REM 手动安装 setuptools，绕过构建步骤

echo ========================================
echo 手动安装 setuptools
echo ========================================
echo.

cd /d V:\b

echo 正在手动安装 setuptools==70.0.0...
echo.

REM 尝试使用阿里云镜像安装
V:\b\python-install\bin\PythonSlicer.exe -m pip install --trusted-host mirrors.aliyun.com --index-url https://mirrors.aliyun.com/pypi/simple/ setuptools==70.0.0

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✓ setuptools 安装成功！
    echo.
    echo 验证安装...
    V:\b\python-install\bin\PythonSlicer.exe -m pip list | findstr setuptools
    echo.
    echo 现在需要标记 python-setuptools 构建为已完成...
    echo.
    
    REM 创建标记文件，让 CMake 认为安装已完成
    if not exist "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp" (
        mkdir "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp"
    )
    
    REM 创建所有配置的标记文件（使用 cmake -E touch 确保格式正确）
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install"
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Debug"
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-RelWithDebInfo"
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Release"
    
    echo ✓ 已创建标记文件
    echo.
    echo 现在可以继续构建:
    echo   ninja -j8
) else (
    echo.
    echo ✗ setuptools 安装失败
    echo.
    echo 尝试使用官方 PyPI（如果网络允许）...
    V:\b\python-install\bin\PythonSlicer.exe -m pip install setuptools==70.0.0
    
    if %ERRORLEVEL% EQU 0 (
        echo ✓ 使用官方 PyPI 安装成功
        REM 创建标记文件
        if not exist "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp" (
            mkdir "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp"
        )
        "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install"
        "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Debug"
        "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-RelWithDebInfo"
        "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Release"
        echo.
        echo 现在可以继续构建: ninja -j8
    ) else (
        echo ✗ 所有安装方法都失败了
        echo 请检查网络连接或使用代理
    )
)

echo.
pause
