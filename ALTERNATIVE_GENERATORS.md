# 不使用 Ninja 的替代生成器方案

## 方案对比

| 生成器 | 优点 | 缺点 | 推荐度 |
|--------|------|------|--------|
| **NMake Makefiles** | ✅ 系统自带<br>✅ 不需要额外工具<br>✅ 兼容 VS 2022 | ❌ 单配置<br>❌ 不支持并行<br>❌ 构建较慢 | ⭐⭐⭐ |
| **NMake Makefiles JOM** | ✅ 支持并行构建<br>✅ 兼容 VS 2022 | ❌ 需要安装 JOM<br>❌ 单配置 | ⭐⭐⭐⭐ |
| **Visual Studio 16 2019** | ✅ 多配置生成器<br>✅ 可在 VS 中打开 | ❌ 需要 VS 2019 或复杂配置<br>❌ 可能不稳定 | ⭐⭐ |
| **Ninja** | ✅ 构建最快<br>✅ 支持并行<br>✅ 单配置但灵活 | ❌ 需要单独安装 | ⭐⭐⭐⭐⭐ |

## 详细配置步骤

### 方案 1: NMake Makefiles（最简单）

```cmd
# 1. 打开 VS 2022 Developer Command Prompt
# 或手动设置环境：
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 2. 设置虚拟驱动器
subst V: C:\S\vs-dev

# 3. 配置
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

# 4. 构建（单线程，较慢）
nmake

# 或使用多线程（如果安装了 JOM）
jom
```

### 方案 2: NMake Makefiles JOM（推荐，支持并行）

**先安装 JOM**:
- 下载：https://wiki.qt.io/Jom
- 或使用 chocolatey: `choco install jom`
- 解压到某个目录，添加到 PATH

```cmd
# 1. 设置 VS 2022 环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 2. 配置
cd /d V:\b
cmake -G "NMake Makefiles JOM" ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON ^
  -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON ^
  -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo ^
  E:\GitHub\rs0\rs

# 3. 构建（支持并行，使用 8 个线程）
jom -j8
```

### 方案 3: Visual Studio 16 2019 + 工具集（实验性）

```cmd
# 1. 设置环境变量（尝试让 CMake 找到 VS 2022）
set "VS160COMNTOOLS=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\"
set "VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\"

# 2. 配置
cd /d V:\b
cmake -G "Visual Studio 16 2019" -A x64 -T v143 ^
  -DQt5_DIR:PATH=D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 ^
  -DCMAKE_INSTALL_PREFIX:PATH=C:/S/rs-install ^
  -DSlicer_SUPERBUILD:BOOL=ON ^
  -DSlicer_BUILD_TESTING:BOOL=OFF ^
  -DSlicer_BUILD_QTSCRIPTEDMODULES:BOOL=ON ^
  -DSlicer_BUILD_EXTENSIONMANAGER_SUPPORT:BOOL=ON ^
  E:\GitHub\rs0\rs

# 3. 构建（多配置）
cmake --build . --config RelWithDebInfo -- /m
```

**注意**: 此方法可能失败，因为 CMake 会检查 VS 2019 的安装。

## 推荐方案

**如果不想使用 Ninja，推荐使用 NMake Makefiles JOM**：
- ✅ 支持并行构建（速度快）
- ✅ 不需要 VS 2019
- ✅ 只需要安装 JOM（轻量级工具）
- ✅ 兼容 VS 2022

**如果不想安装额外工具，使用 NMake Makefiles**：
- ✅ 系统自带
- ❌ 但构建会很慢（单线程）

## 性能对比（预估）

- **Ninja -j8**: ⭐⭐⭐⭐⭐（最快）
- **JOM -j8**: ⭐⭐⭐⭐（接近 Ninja）
- **NMake**: ⭐⭐（单线程，最慢）
- **VS 生成器**: ⭐⭐⭐（多配置，但首次配置慢）
