# 解决 Python 包下载失败问题

## 问题描述

构建时出现错误：
```
CMake Error: Command failed: 2
'V:/b/python-install/bin/PythonSlicer.exe' '-m' 'pip' 'install' '--require-hashes' '-r' 'V:/b/python-setuptools-requirements.txt'
```

这是网络连接问题，无法从 PyPI 下载 Python 包（如 `setuptools`、`pip`、`wheel` 等）。

## 已修复的文件

我已经修改了以下文件，添加了阿里云镜像支持：

1. **`slicer-v5.8.1/SuperBuild/External_python-setuptools.cmake`**
   - 添加 `--trusted-host mirrors.aliyun.com`
   - 添加 `--index-url https://mirrors.aliyun.com/pypi/simple/`

2. **`slicer-v5.8.1/SuperBuild/External_python-pip.cmake`**
   - 添加相同的镜像配置

3. **`slicer-v5.8.1/SuperBuild/External_python-wheel.cmake`**
   - 添加相同的镜像配置

## 解决方案

### 方案 1: 重新配置并构建（推荐）

由于修改了 CMake 文件，需要重新配置：

```cmd
# 1. 清理旧的配置（可选，但推荐）
cd /d V:\b
del CMakeCache.txt 2>nul
rmdir /s /q CMakeFiles 2>nul

# 2. 重新配置
cmake -G Ninja ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  E:\GitHub\rs0\rs

# 3. 继续构建
ninja -j8
```

### 方案 2: 直接继续构建（如果配置已完成）

如果 CMake 配置已经完成，可以直接继续构建，CMake 会使用新的配置：

```cmd
cd /d V:\b
ninja -j8
```

**注意**：如果构建仍然失败，可能需要清理并重新配置（使用方案 1）。

### 方案 3: 手动安装 setuptools（临时方案）

如果上述方案都不行，可以手动安装：

```cmd
# 1. 激活 Python 环境
cd /d V:\b
V:\b\python-install\bin\PythonSlicer.exe -m pip install --trusted-host mirrors.aliyun.com --index-url https://mirrors.aliyun.com/pypi/simple/ setuptools==70.0.0

# 2. 然后继续构建
ninja -j8
```

## 其他可能失败的 Python 包

如果其他 Python 包也失败（如 `numpy`、`scipy`、`requests` 等），可以按照相同的方式修改对应的 `.cmake` 文件：

在 `INSTALL_COMMAND` 中添加：
```cmake
--trusted-host mirrors.aliyun.com --index-url https://mirrors.aliyun.com/pypi/simple/
```

例如，修改 `External_python-numpy.cmake`：
```cmake
INSTALL_COMMAND ${PYTHON_EXECUTABLE} -m pip install --trusted-host mirrors.aliyun.com --index-url https://mirrors.aliyun.com/pypi/simple/ --require-hashes -r ${requirements_file}
```

## 验证修复

构建成功后，可以验证 Python 包是否已安装：

```cmd
V:\b\python-install\bin\PythonSlicer.exe -m pip list | findstr setuptools
```

应该显示：
```
setuptools          70.0.0
```

## 注意事项

- 阿里云镜像在中国大陆访问速度更快、更稳定
- `--trusted-host` 选项允许 pip 信任镜像服务器的 SSL 证书
- `--require-hashes` 仍然保留，确保包的安全性
- 如果使用其他镜像（如清华、中科大），可以替换 URL

## 其他镜像选项

如果阿里云镜像不可用，可以尝试：

1. **清华大学镜像**：
   ```cmake
   --trusted-host pypi.tuna.tsinghua.edu.cn --index-url https://pypi.tuna.tsinghua.edu.cn/simple/
   ```

2. **中科大镜像**：
   ```cmake
   --trusted-host mirrors.ustc.edu.cn --index-url https://mirrors.ustc.edu.cn/pypi/web/simple/
   ```

3. **豆瓣镜像**：
   ```cmake
   --trusted-host pypi.douban.com --index-url https://pypi.douban.com/simple/
   ```
