# 解决 python-setuptools 安装失败问题

## 问题原因

虽然已经修改了 `External_python-setuptools.cmake` 文件添加了阿里云镜像，但 CMake 已经生成了安装脚本（在 `slicersources-build/python-setuptools-prefix/` 目录下），这些脚本仍然使用旧的命令（没有镜像配置）。

## 解决方案

### 方案 1: 清理并重新配置（推荐）

```cmd
# 1. 切换到构建目录
cd /d V:\b

# 2. 删除 python-setuptools 相关的生成文件
rmdir /s /q slicersources-build\python-setuptools-prefix 2>nul
del python-setuptools-requirements.txt 2>nul

# 3. 清理 CMake 缓存（可选，但推荐）
del CMakeCache.txt 2>nul
rmdir /s /q CMakeFiles 2>nul

# 4. 重新配置
cmake -G Ninja ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON ^
  -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON ^
  E:\GitHub\rs0\rs

# 5. 继续构建
ninja -j8
```

### 方案 2: 只清理 python-setuptools 相关文件（快速）

如果不想完全重新配置，可以只清理 python-setuptools 相关的生成文件：

```cmd
cd /d V:\b

# 删除 python-setuptools 的生成文件
rmdir /s /q slicersources-build\python-setuptools-prefix 2>nul
del python-setuptools-requirements.txt 2>nul

# 继续构建（CMake 会重新生成）
ninja -j8
```

### 方案 3: 手动修改生成的安装脚本（临时方案）

如果上述方案都不行，可以手动修改生成的安装脚本：

```cmd
# 1. 找到安装脚本
cd /d V:\b\slicersources-build\python-setuptools-prefix\src\python-setuptools-stamp

# 2. 编辑 python-setuptools-install-Debug.cmake（或 RelWithDebInfo）
# 找到这一行：
#   'V:/b/python-install/bin/PythonSlicer.exe' '-m' 'pip' 'install' '--require-hashes' '-r' 'V:/b/python-setuptools-requirements.txt'
# 
# 修改为：
#   'V:/b/python-install/bin/PythonSlicer.exe' '-m' 'pip' 'install' '--trusted-host' 'mirrors.aliyun.com' '--index-url' 'https://mirrors.aliyun.com/pypi/simple/' '--require-hashes' '-r' 'V:/b/python-setuptools-requirements.txt'

# 3. 继续构建
cd /d V:\b
ninja -j8
```

**注意**：方案 3 是临时方案，如果重新配置，这些文件会被覆盖。

## 验证修复

构建成功后，验证 setuptools 是否已安装：

```cmd
V:\b\python-install\bin\PythonSlicer.exe -m pip list | findstr setuptools
```

应该显示：
```
setuptools          70.0.0
```

## 注意事项

- 如果使用 Visual Studio 生成器（多配置），可能需要清理所有配置的脚本：
  - `python-setuptools-install-Debug.cmake`
  - `python-setuptools-install-RelWithDebInfo.cmake`
  - `python-setuptools-install-Release.cmake`

- 如果其他 Python 包也失败（pip、wheel 等），使用相同的清理方法。
