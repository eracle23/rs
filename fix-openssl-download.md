# OpenSSL 下载失败解决方案

## 问题
OpenSSL 从 GitHub 下载失败：
- URL: `https://github.com/Slicer/Slicer-OpenSSL/releases/download/1.1.1g/OpenSSL_1_1_1g-install-msvc1900-64.tar.gz`
- 可能原因：网络连接问题、GitHub 访问受限、SSL 证书问题

## 解决方案

### 方案 1：手动下载（推荐）

1. **手动下载文件**：
   - 使用浏览器或下载工具下载：
     ```
     https://github.com/Slicer/Slicer-OpenSSL/releases/download/1.1.1g/OpenSSL_1_1_1g-install-msvc1900-64.tar.gz
     ```
   - 或使用镜像站点（如果有）

2. **放置到构建目录**：
   ```bash
   # 将下载的文件放到 V:\b\ 目录
   copy OpenSSL_1_1_1g-install-msvc1900-64.tar.gz V:\b\
   ```

3. **验证 MD5**（可选）：
   ```bash
   # 预期 MD5: f89ea6a4fcfb279af30cbe01c1d7f879
   certutil -hashfile V:\b\OpenSSL_1_1_1g-install-msvc1900-64.tar.gz MD5
   ```

4. **重新构建**：
   ```bash
   ninja -j8
   ```

### 方案 2：禁用 SSL 支持（如果不需要）

如果不需要 Python SSL 支持，可以禁用：

```bash
# 重新配置时添加选项
cmake -G Ninja ^
    -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_INSTALL_PREFIX=C:/S/rs-install ^
    -DSlicer_SUPERBUILD=ON ^
    -DSlicer_BUILD_TESTING=OFF ^
    -DSlicer_BUILD_QTSCRIPTEDMODULES=ON ^
    -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT=ON ^
    -DSlicer_USE_PYTHONQT_WITH_OPENSSL=OFF ^
    -DQt5_DIR=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
    E:\GitHub\rs0\rs
```

### 方案 3：使用代理或镜像

如果使用代理，配置环境变量：

```bash
set HTTP_PROXY=http://proxy.example.com:8080
set HTTPS_PROXY=http://proxy.example.com:8080
```

### 方案 4：使用系统 OpenSSL（如果已安装）

如果系统已安装 OpenSSL，可以配置使用系统版本：

```bash
cmake -G Ninja ^
    ... ^
    -DSlicer_USE_SYSTEM_OpenSSL=ON ^
    -DOPENSSL_INCLUDE_DIR=C:/path/to/openssl/include ^
    -DLIB_EAY_RELEASE=C:/path/to/openssl/lib/libcrypto.lib ^
    -DSSL_EAY_RELEASE=C:/path/to/openssl/lib/libssl.lib ^
    ...
```

## 快速修复脚本

创建 `fix-openssl.bat`：

```bat
@echo off
echo 正在检查 OpenSSL 文件...

if exist "V:\b\OpenSSL_1_1_1g-install-msvc1900-64.tar.gz" (
    echo 文件已存在，跳过下载
    exit /b 0
)

echo.
echo 请手动下载以下文件：
echo https://github.com/Slicer/Slicer-OpenSSL/releases/download/1.1.1g/OpenSSL_1_1_1g-install-msvc1900-64.tar.gz
echo.
echo 下载后，将文件放到: V:\b\
echo.
pause
```
