@echo off
REM 最终解决方案：手动安装 setuptools 并创建标记文件

echo ========================================
echo 手动安装 setuptools（绕过 SSL 问题）
echo ========================================
echo.

cd /d V:\b

echo 步骤 1: 手动安装 setuptools...
echo.

REM 设置环境变量禁用 SSL 验证
set PYTHONHTTPSVERIFY=0
set CURL_CA_BUNDLE=
set REQUESTS_CA_BUNDLE=

REM 尝试安装
V:\b\python-install\bin\PythonSlicer.exe -m pip install --trusted-host mirrors.aliyun.com --trusted-host pypi.org --trusted-host files.pythonhosted.org --index-url https://mirrors.aliyun.com/pypi/simple/ setuptools==70.0.0

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo 尝试使用官方 PyPI...
    V:\b\python-install\bin\PythonSlicer.exe -m pip install setuptools==70.0.0
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✓ setuptools 安装成功！
    echo.
    echo 步骤 2: 创建标记文件...
    
    REM 确保目录存在
    if not exist "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp" (
        mkdir "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp"
    )
    
    REM 使用 cmake -E touch 创建标记文件（与 CMake 生成的一致）
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install"
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Debug"
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-RelWithDebInfo"
    "C:\Program Files\cmake-3.16.3\bin\cmake.exe" -E touch "slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp\python-setuptools-install-Release"
    
    echo ✓ 标记文件已创建
    echo.
    echo 验证安装...
    V:\b\python-install\bin\PythonSlicer.exe -m pip list | findstr setuptools
    echo.
    echo ========================================
    echo 完成！现在可以继续构建:
    echo   ninja -j8
    echo ========================================
) else (
    echo.
    echo ✗ setuptools 安装失败
    echo.
    echo 请检查:
    echo 1. 网络连接
    echo 2. 代理设置
    echo 3. 防火墙设置
    echo.
    echo 或者尝试离线安装:
    echo 1. 从其他机器下载 setuptools-70.0.0-py3-none-any.whl
    echo 2. 复制到 V:\b\
    echo 3. 运行: V:\b\python-install\bin\PythonSlicer.exe -m pip install V:\b\setuptools-70.0.0-py3-none-any.whl
)

echo.
pause
