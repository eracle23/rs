# RadianceSuite 完整构建指南 (CMake 3.19.7 + VS 2022)

## 环境要求

- **CMake**: 3.19.7（符合 Slicer 5.8.1 要求：3.16.3-3.19.7）
- **Visual Studio**: 2022 社区版（已安装）
- **Qt**: 5.15.2 msvc2019_64（路径：`D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`）
- **Git**: 已安装
- **Ninja**: 已安装在 `C:\Users\lie76\ninja\ninja.exe`
- **磁盘空间**: 至少 15GB（Release）或 60GB（Debug）
- **构建时间**: 首次构建 3-8 小时（取决于硬件）

## 前置准备

### 1. 启用 Windows 长路径支持（必须，已启用）

```powershell
# 以管理员身份运行 PowerShell
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
```

**重要**: 设置后需要**重启计算机**才能生效。

### 2. 设置虚拟驱动器（重启后需要重新设置）

```cmd
subst V: C:\S\vs-dev
```

### 3. 设置环境变量（可选，但推荐）

```cmd
set Qt5_DIR=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5
```

或者使用 PowerShell：
```powershell
$env:Qt5_DIR = "D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5"
```

## 完整构建流程

### 步骤 1: 准备构建目录

```cmd
# 创建构建目录（如果不存在）
mkdir C:\S\vs-dev\b 2>nul

# 设置虚拟驱动器
subst V: C:\S\vs-dev

# 切换到构建目录
cd /d V:\b
```

### 步骤 2: 配置 CMake（使用 Ninja 生成器）

由于 CMake 3.19.7 不支持 "Visual Studio 17 2022" 生成器，我们使用 Ninja：

```cmd
cd /d V:\b

cmake -G "Ninja" ^
  -DCMAKE_MAKE_PROGRAM=C:/Users/lie76/ninja/ninja.exe ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON ^
  -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON ^
  -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo ^
  E:\GitHub\rs0\rs
```

**配置说明**:
- `-G "Ninja"`: 使用 Ninja 生成器（CMake 3.19.7 支持）
- `-DCMAKE_MAKE_PROGRAM`: 指定 Ninja 可执行文件路径
- `-DQt5_DIR`: Qt5 的 CMake 配置路径
- `-DCMAKE_BUILD_TYPE=RelWithDebInfo`: 带调试信息的发布版本（推荐）
- `-DSlicer_SUPERBUILD=ON`: 启用 SuperBuild（会自动构建所有依赖）

**首次配置可能需要 10-30 分钟**，因为需要：
- 下载 Slicer 5.8.1 源码
- 配置所有外部依赖（VTK, ITK, CTK, Python 等）

### 步骤 3: 构建项目

```cmd
cd /d V:\b

# 使用 8 个并行任务构建（根据 CPU 核心数调整）
ninja -j8
```

或者使用 CMake 构建命令：

```cmd
cmake --build V:\b --config RelWithDebInfo -j 8
```

**构建时间**:
- 首次完整构建：3-8 小时
- 增量构建：几分钟到几十分钟（取决于修改的文件）

**构建过程**:
1. 首先构建外部依赖（VTK, ITK, CTK, Python, SimpleITK 等）
2. 然后构建 Slicer 核心
3. 最后构建 RadianceApp

### 步骤 4: 安装（可选）

```cmd
cd /d V:\b
ninja install
```

或者：

```cmd
cmake --build V:\b --target install
```

安装目录：`C:/S/rs-install`

### 步骤 5: 运行应用程序

```cmd
# 运行主程序
C:\S\rs-install\bin\RadianceApp.exe

# 或者从构建目录运行
V:\b\Slicer-build\bin\RelWithDebInfo\RadianceApp.exe
```

## 常见问题处理

### 问题 1: SimpleITK SSL 错误

**已修复**: `slicer-v5.8.1/SuperBuild/External_SimpleITK.cmake` 已更新为使用阿里云镜像源。

如果仍遇到问题，检查生成的安装脚本：
```cmd
type V:\b\SimpleITK_install_step.cmake
```

应该包含 `--trusted-host mirrors.aliyun.com` 和 `--index-url https://mirrors.aliyun.com/pypi/simple/`

### 问题 2: 路径长度超过 260 字符

**解决方案**: 
1. 确保已启用 Windows 长路径支持（步骤 1）
2. 已重启计算机
3. 使用虚拟驱动器 `V:\b` 缩短路径

### 问题 3: Ninja 找不到

确保 Ninja 路径正确：
```cmd
C:\Users\lie76\ninja\ninja.exe --version
```

如果路径不同，在 CMake 配置时使用 `-DCMAKE_MAKE_PROGRAM` 指定正确路径。

### 问题 4: 构建失败后重新配置

如果构建失败需要重新配置：

```cmd
cd /d V:\b

# 清理缓存
del CMakeCache.txt
rmdir /s /q CMakeFiles
rmdir /s /q slicersources-subbuild

# 重新配置
cmake -G "Ninja" -DCMAKE_MAKE_PROGRAM=C:/Users/lie76/ninja/ninja.exe ...
```

## 增量构建

修改代码后，只需重新构建：

```cmd
cd /d V:\b
ninja -j8
```

CMake 会自动检测更改的文件并只重新构建必要的部分。

## 只构建应用程序（不重新构建依赖）

如果只修改了应用程序代码，可以只构建 RadianceApp：

```cmd
cd /d V:\b
ninja RadianceApp
```

## 清理构建

### 清理所有构建文件（保留配置）

```cmd
cd /d V:\b
ninja clean
```

### 完全清理（包括配置）

```cmd
cd /d V:\b
del /q CMakeCache.txt
rmdir /s /q CMakeFiles
rmdir /s /q build.ninja
# 然后重新配置
```

## 打包（生成安装程序）

需要先安装 [NSIS 3.x](https://nsis.sourceforge.io/Download)

```cmd
cd /d V:\b\Slicer-build
cmake --build . --config RelWithDebInfo --target PACKAGE
```

安装程序将生成在：`V:\b\Slicer-build\RadianceApp-*.exe`

## 开发工作流

### 日常开发

1. **修改代码**
2. **增量构建**:
   ```cmd
   cd /d V:\b
   ninja -j8
   ```
3. **运行测试**:
   ```cmd
   V:\b\Slicer-build\bin\RelWithDebInfo\RadianceApp.exe
   ```

### 调试

1. 在 Visual Studio 中打开：
   ```cmd
   # 生成 VS 项目文件（用于调试，但不用于构建）
   cd /d V:\b
   cmake -G "Visual Studio 16 2019" -A x64 -T v143 ...
   ```
   然后打开 `V:\b\RadianceSuite.sln`

2. 或者直接附加到进程：
   - 运行 `RadianceApp.exe`
   - 在 VS 中：调试 → 附加到进程

## 注意事项

1. **CMake 版本限制**: Slicer 5.8.1 要求 CMake 3.16.3-3.19.7，不能升级到 3.20+
2. **生成器限制**: CMake 3.19.7 不支持 "Visual Studio 17 2022"，必须使用 Ninja 或 "Visual Studio 16 2019"（但需要 VS 2019）
3. **路径长度**: 始终使用虚拟驱动器 `V:\b` 而不是 `C:\S\vs-dev\b`
4. **重启后**: 每次重启后需要重新运行 `subst V: C:\S\vs-dev`
5. **网络问题**: SimpleITK 安装已配置使用阿里云镜像，如果仍有问题检查网络连接

## 快速参考命令

```cmd
# 1. 设置虚拟驱动器
subst V: C:\S\vs-dev

# 2. 切换到构建目录
cd /d V:\b

# 3. 配置（首次或重新配置）
cmake -G "Ninja" -DCMAKE_MAKE_PROGRAM=C:/Users/lie76/ninja/ninja.exe -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install -DSlicer_SUPERBUILD:BOOL=ON -DSlicer_BUILD_TESTING:BOOL=OFF -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo E:\GitHub\rs0\rs

# 4. 构建
ninja -j8

# 5. 运行
V:\b\Slicer-build\bin\RelWithDebInfo\RadianceApp.exe
```
