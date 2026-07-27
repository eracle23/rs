# 使用 VS/MSBuild 构建指南

## 问题分析

- **CMake 3.19.7** 不支持 "Visual Studio 17 2022" 生成器
- **Slicer 5.8.1** 限制 CMake 版本为 3.16.3-3.19.7
- 要使用 VS 2022 生成器，需要 **CMake 3.21+**

## 解决方案：修改版本限制并升级 CMake

### 步骤 1: 修改 Slicer 的 CMake 版本限制

修改 `slicer-v5.8.1/CMakeLists.txt`：

```cmake
# 原版本（第 1 行）：
cmake_minimum_required(VERSION 3.16.3...3.19.7 FATAL_ERROR)

# 修改为（允许更高版本，但避开有问题的版本）：
cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)
```

Slicer 已经内置了对问题版本的检查（3.21.0 和 3.25.0-3.25.2），所以只需要移除上限即可。

### 步骤 2: 升级 CMake

下载并安装 **CMake 3.21.1** 或更高版本（推荐 3.22+ 或最新稳定版）：
- 下载地址：https://cmake.org/download/
- 安装时选择 "Add CMake to system PATH"

**避开的问题版本**：
- ❌ 3.21.0（已知 bug）
- ❌ 3.25.0 - 3.25.2（已知 bug）
- ✅ 3.21.1, 3.22.x, 3.23.x, 3.24.x, 3.25.3+ 都可以

### 步骤 3: 配置和构建

```cmd
# 1. 设置虚拟驱动器
subst V: C:\S\vs-dev

# 2. 切换到构建目录
cd /d V:\b

# 3. 清理旧的配置（如果存在）
del CMakeCache.txt 2>nul
rmdir /s /q CMakeFiles 2>nul

# 4. 配置（使用 VS 2022 生成器）
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON ^
  -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON ^
  E:\GitHub\rs0\rs

# 5. 构建（多配置生成器，使用 RelWithDebInfo）
cmake --build . --config RelWithDebInfo -- /m

# 或使用 MSBuild 直接构建
msbuild RadianceSuite.sln /p:Configuration=RelWithDebInfo /m
```

### 步骤 4: 在 Visual Studio 中打开

```cmd
# 打开解决方案文件
start V:\b\RadianceSuite.sln
```

然后在 VS 中：
- 选择配置：RelWithDebInfo（推荐）或 Release
- 右键解决方案 → 生成解决方案
- 或按 F5 运行

## 方案 2: 不升级 CMake（使用 NMake + MSBuild）

如果不想升级 CMake，可以使用 NMake 生成器，但构建体验不如 VS 生成器：

```cmd
# 1. 在 VS 2022 Developer Command Prompt 中运行
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 2. 配置
cd /d V:\b
cmake -G "NMake Makefiles" ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON ^
  -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON ^
  -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo ^
  E:\GitHub\rs0\rs

# 3. 构建（单线程，较慢）
nmake
```

**缺点**：
- 单配置生成器
- 不支持并行构建（除非使用 JOM）
- 不能在 VS IDE 中打开

## 推荐方案对比

| 方案 | 优点 | 缺点 | 推荐度 |
|------|------|------|--------|
| **升级 CMake + VS 2022 生成器** | ✅ 官方推荐<br>✅ 多配置<br>✅ VS IDE 支持<br>✅ 最稳定 | ❌ 需要升级 CMake<br>❌ 需要修改 Slicer 限制 | ⭐⭐⭐⭐⭐ |
| **NMake + MSBuild** | ✅ 不需要升级 CMake | ❌ 单配置<br>❌ 不能在 VS 中打开<br>❌ 构建慢 | ⭐⭐ |

## 我的建议

**强烈推荐方案 1**（升级 CMake）：
1. 只需要修改一行代码（移除版本上限）
2. CMake 升级很简单（下载安装包即可）
3. 可以获得最佳的构建体验
4. 符合项目官方推荐

如果确实不能升级 CMake，那么使用 Ninja 是更好的选择，而不是 NMake。
