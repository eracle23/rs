# Alice/RadianceSuite 构建与打包指南（Windows）

本指南沉淀了本仓库在 VS2022 多配置环境下的稳定黄金路径，覆盖扩展安装踩坑、日常开发构建（RelWithDebInfo）、安装、打包分发（NSIS）以及排错要点。默认前缀与路径如下，可按需替换。

* 顶层源码：`C:/RS`
* 内层构建树：`C:/S/vs-dev`（由 `CMakePresets.json: vs17-dev` 生成）
* 安装前缀：`C:/S/rs-install`
* 结构化日志：`C:/S/logs/*.binlog`

## Landmark Registration & General Registration (Elastix) 坑位速览

1. **CPack 报 `file INSTALL cannot find …/CMakeFiles/AliceW.exe`**
   * 原因：GUI 启动器 `AliceW.exe` 由 `AliceUpdateLauncherWIcon` 自定义目标生成，`PACKAGE` 前并不保证构建。
   * 解决：在 `Applications/RadianceApp/CMakeLists.txt` 增加 `AliceLauncherArtifacts (ALL)` 依赖 `AliceUpdateLauncherWIcon`（兼容旧目标名时再 fallback），或手工先构建该目标。

2. **扩展在 inner（`Slicer-build/E`）配置失败：`include(${Slicer_EXTENSION_GENERATE_CONFIG})` 等变量为空**
   * 原因：`E/SlicerConfig.cmake` 是精简版，不导出 `Slicer_EXTENSION_*`、`Slicer_PYTHON_MODULE_TEST_TEMPLATES_DIR` 等。
   * 解决：通过 `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` 注入 `_rs_ext_init.cmake` 在 `project()` 前回填这些变量，或在扩展 `CMakeLists` 中进行兜底。

3. **SlicerElastix 需要测试模板路径**
   * 现象：独立配置 SlicerElastix 时，若 `Slicer_PYTHON_MODULE_TEST_TEMPLATES_DIR` 为空会触发 “Configuring incomplete”。
   * 解决：回填为 `…/slicersources-src/Base/QTCore/Testing/Python`，或显式 `-DSlicer_BUILD_TESTING=OFF -DBUILD_TESTING=OFF`。

4. **CTK 宏未加载：`ctkFunctionExtractOptimizedLibrary` 未定义**
   * 原因：没 `include(UseCTK.cmake)` 时，扩展马上用到了 CTK 函数。
   * 解决：把 `include(UseCTK.cmake)` 与 `include(ctkFunctionExtractOptimizedLibrary.cmake)` 拆成两个独立 `if()`，任一存在即可工作。

5. **CTKAppLauncher 相关变量缺失**
   * 现象：扩展 CPack / 生成器偶尔需要 `CTKAppLauncher_DIR`。
   * 解决：在 `_rs_ext_init.cmake` 中自动探测（`CTKAPPLAUNCHER`、`CTKAppLauncherLib-build` 等）并回填。

6. **打包顺序与缓存**
   * 教训：必须 **`ALL_BUILD → INSTALL → PACKAGE`**；只跑 `INSTALL/PACKAGE` 会经常缺库。重试打包前清空 `_CPack_Packages`，避免旧残留。

7. **Elastix UI vs 运行库**
   * UI = **SlicerElastix**；运行库 = **`elastix.exe/transformix.exe`**。务必把 SlicerElastix 作为扩展参加 SuperBuild，它自带的 SuperBuild 会下载/编译 elastix/transformix 并随安装包发布，不要再手拷 Python 目录或依赖 Extension Manager 的二进制包。

---

## 先决条件

* 安装 VS2022（含 C++ 工具链与 Windows SDK），安装 Git、CMake 3.29+、Ninja（推荐 1.11.x）。
* 安装 Qt5.15.2 MSVC2019_64，或设置 `QT5_DIR` 指到其 CMake 包路径。
* **确认 NSIS 3.x** 已安装且在 `PATH` 中；若尚未配置，可临时使用 `cpack -G ZIP` 做打包冒烟测试。
* 一键准备（可选）：
  * 引导：`pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey`
  * 设置 Qt：`pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`

## 预设与脚本

* 配置预设（VS 多配置）：`vs17-dev`（见根目录 `CMakePresets.json`）
* 构建脚本：`Tools/Invoke-RadianceBuild.ps1`
  * 关键参数：
    * `-InnerOnly`：直驱“内层三步”（不触发母目标 `Slicer`）。
    * `-InnerConfig <Debug|Release|RelWithDebInfo|MinSizeRel>`：内层配置，默认 `RelWithDebInfo`。
    * `-Jobs N`：并行度；R5 3600 建议 `6`，32GB+NVMe 可试 `8`。
    * `-ConfigureOnly`：仅配置，跳过编译。

> 说明：VS 方案内不存在 `Slicer-configure/Slicer-build/Slicer-install` 这类 step 目标，`-InnerOnly` 会直接对 `slicersources-src/Slicer-build` 调用 CMake 的 configure/build/install。

## 黄金路径（RelWithDebInfo）

### 1) 生成顶层 VS 方案（很快）

```powershell
cmake --preset vs17-dev
```

### 2) 内层构建（稳定、清晰）

方式 A（推荐，一条命令全流程并生成结构化日志）：

```powershell
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -InnerOnly -InnerConfig RelWithDebInfo -Jobs 6
```

方式 B（与脚本等价的手工三步）：

```powershell
# 仅首次或切换配置时需要
cmake -G "Visual Studio 17 2022" -A x64 `
  -S C:/S/vs-dev/slicersources-src `
  -B C:/S/vs-dev/Slicer-build `
  -C C:/S/vs-dev/slicersources-build/Slicer-prefix/tmp/Slicer-cache-RelWithDebInfo.cmake

# 构建（结构化日志）
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo `
  -- /m:6 /v:m /bl:C:/S/logs/inner-relwithdebinfo.binlog
```

> ⚠️ `INSTALL` / `PACKAGE` 不会自动补齐缺失库。手工执行时务必坚持 **`ALL_BUILD → INSTALL → PACKAGE`**：
>
> ```powershell
> # 2) 内层构建
> cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target ALL_BUILD -- /m:6 /v:m
> # 2.5) 安装到统一前缀（建议打包前至少执行一次）
> cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target INSTALL -- /m:6
> # 3) 打包（NSIS/ZIP）
> cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target PACKAGE -- /m:6 /v:m
> # 如需快速验证可用：
> # cpack -C RelWithDebInfo -G ZIP --config C:/S/vs-dev/Slicer-build/CPackConfig.cmake
> ```

### 3) 打包（可分发 NSIS 安装器）

若首次打包 SimpleITK 未就绪，先构建外层 SITK（RelWithDebInfo），再执行 package：

```powershell
# 准备（一次性）
cmake --build C:/S/vs-dev/slicersources-build --config RelWithDebInfo --target SimpleITK -- /m:6

# 打包（CPack 会完成完整依赖修复/Qt 部署）
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target package `
  -- /m:6 /v:m /bl:C:/S/logs/inner-relwithdebinfo-package.binlog

# 产物通常位于：
# C:/S/vs-dev/Slicer-build/Alice-<version>-win-amd64.exe
# 或（视 CPACK 变量）C:/RS/Alice-<version>-win-amd64.exe
```

### 3.5) 预检安装（强烈建议）

打包前做一次“离线安装”可快速发现缺件：

```powershell
cmake --install C:/S/vs-dev/Slicer-build --config RelWithDebInfo --prefix C:/S/_stage_preflight
```

## 打包产物自检

```powershell
# 包名/输出目录/组件配置
Select-String C:/S/vs-dev/Slicer-build/CPackConfig.cmake `
  -Pattern "CPACK_PACKAGE_FILE_NAME|CPACK_PACKAGE_DIRECTORY|CPACK_INSTALL_CMAKE_PROJECTS"

# 安装前缀
Select-String C:/S/vs-dev/Slicer-build/CMakeCache.txt -Pattern "CMAKE_INSTALL_PREFIX"

# NSIS 是否就绪
where makensis

# 安装包是否生成（同时看大小）
Get-ChildItem C:/S/vs-dev/Slicer-build -Recurse -Filter "Alice-*.exe" | Select FullName,Length
```

## 常用验证

* 安装树启动：`C:/S/rs-install/Alice.exe`
* Python 控制台：

```python
import vtk, qt, ctk, slicer
print('VTK:', vtk.vtkVersion.GetVTKVersion())
print('CTK OK:', hasattr(ctk, 'ctkCollapsibleButton'))
```

* 模块检查：
  * `Dcm2niixGUI` 可见且可打开；能找到 `Resources/bin/dcm2niix.exe` 或 `app/bin/dcm2niix.exe`。
  * Segment Editor 效果下拉包含 `LocalThreshold / FastMarching / DrawTube` 等。
* 注册类模块包含 **General Registration (Elastix)** 与 **Landmark Registration**。
* 构建完应已在包内找到 `elastix/transformix`（由 SlicerElastix 托管），如仍报错请先确认 `SuperBuild/Extensions.cmake` 已启用该扩展；必要时再用 `Edit → Application Settings → Modules → SlicerElastix` 手动指定外部可执行或把二进制复制到 `<App>/bin/`。

## 结构化日志与进度估算

* 任意长流程可加 MSBuild binlog：`/v:m /bl:C:/S/logs/<name>.binlog`
* 用 MSBuild Structured Log Viewer 观察“当前活跃 target”。

## 切换/清理注意事项

* 不混用 Release 与 RelWithDebInfo。若误跑过 Release，可清理：

```powershell
Remove-Item -Recurse -Force C:/S/vs-dev/Slicer-build/bin/Release -EA SilentlyContinue
Remove-Item -Recurse -Force C:/S/vs-dev/Slicer-build/lib/Release -EA SilentlyContinue
```

* 仅在“初始 cache 失效或要强制更新内层默认值”时做最小清理：

```powershell
Remove-Item -Recurse -Force C:/S/vs-dev/Slicer-build -EA SilentlyContinue
Get-ChildItem C:/S/vs-dev/slicersources-build/Slicer-prefix/tmp/ -Filter 'Slicer-cache-*.cmake' -EA SilentlyContinue | Remove-Item -Force
```

## 变量与扩展（已内置转发）

* 顶层 `CMakeLists.txt` 已明确并转发：
  * `Slicer_USE_SimpleITK=ON`
  * `Slicer_EXTENSION_SOURCE_DIRS`、`Slicer_EXTENSION_INSTALL_DIRS`
* 脚本 `-InnerOnly` 会把上述定义同步到所有 `Slicer-cache-*.cmake`，且不会触发顶层 `--target Slicer`，但会在 `Slicer-build` 内完整执行 `ALL_BUILD → INSTALL`。

### 扩展打包：Elastix + LandmarkRegistration

SlicerElastix 自带 `SuperBuild/`，把扩展加入 `Slicer_EXTENSION_SOURCE_DIRS` 即可顺带构建 `elastix/transformix` 并随安装包发布：

```cmake
_bundle_ext(SlicerElastix        https://github.com/lassoan/SlicerElastix.git                   021d715c1de4db3b0ce3ec2f14345aab1bc1c15a)
_bundle_ext(LandmarkRegistration https://github.com/SlicerIGT/LandmarkRegistration.git          <tag>)
```

> 说明：
>
> 1. `GIT_TAG` 必须固定在验证过的提交（示例为 2025-11-05 `021d715c...`），否则 elastix/transformix 版本会随 upstream 漂移。
> 2. LandmarkRegistration 亦建议 pin 提交（当前沿用 `master`）。构建时顶层 `CMakeLists.txt` 会自动把每个扩展的 `SuperBuild/` 目录加入 `EXTERNAL_PROJECT_ADDITIONAL_DIRS`，SlicerElastix 的内部 SuperBuild 因此会被触发，无需再维护单独的“Elastix CLI”扩展。

## 常见故障排查

* **CPack 提示 `file INSTALL cannot find .../AliceW.exe`**
  * 解决：保证 `AliceUpdateLauncherWIcon` 先于打包执行，可使用下列片段（同时兼容旧目标名，并让 `package` 目标也依赖它）：

    ```cmake
    if(WIN32)
      set(_launcher_artifacts_target "${SLICERAPP_APPLICATION_NAME}LauncherArtifacts")
      if(NOT TARGET ${_launcher_artifacts_target})
        add_custom_target(${_launcher_artifacts_target} ALL)
      endif()
      if(TARGET ${SLICERAPP_APPLICATION_NAME}UpdateLauncherWIcon)
        add_dependencies(${_launcher_artifacts_target} ${SLICERAPP_APPLICATION_NAME}UpdateLauncherWIcon)
      elseif(TARGET UpdateLauncherWIcon) # legacy name
        add_dependencies(${_launcher_artifacts_target} UpdateLauncherWIcon)
      endif()
      if(TARGET package)
        add_dependencies(package ${_launcher_artifacts_target})
      endif()
    endif()
    ```

  * 或手工先构建 `--target AliceUpdateLauncherWIcon`。

* **CPack 报 `file INSTALL cannot find ...` 或 `File exists`**
  * 说明 staging 中缺少构件或残留旧文件。
  * 解决：严格执行 `ALL_BUILD → INSTALL → PACKAGE`，并在重试前清理：

    ```powershell
    Remove-Item C:/S/vs-dev/Slicer-build/_CPack_Packages -Recurse -Force -EA SilentlyContinue
    ```

* **SlicerElastix configure 期失败（精简版 `E/SlicerConfig` 缺变量）**
  * 现象：`include(${Slicer_EXTENSION_GENERATE_CONFIG})` / `Slicer_PYTHON_MODULE_TEST_TEMPLATES_DIR` 为空。
  * 解决（任一）：
    1. 在 `_rs_ext_init.cmake` 回填：

       ```cmake
       set(Slicer_PYTHON_MODULE_TEST_TEMPLATES_DIR
         "${_rs_slicer_src}/Base/QTCore/Testing/Python" CACHE PATH FORCE)
       set(Slicer_EXTENSION_GENERATE_CONFIG
         "${_rs_slicer_src}/CMake/SlicerExtensionGenerateConfig.cmake" CACHE FILEPATH FORCE)
       set(Slicer_EXTENSION_CPACK
         "${_rs_slicer_src}/CMake/SlicerExtensionCPack.cmake" CACHE FILEPATH FORCE)
       ```

    2. 配置时禁测：`-DSlicer_BUILD_TESTING=OFF -DBUILD_TESTING=OFF`。

* **CTK 宏未加载：`ctkFunctionExtractOptimizedLibrary` 未定义**
  * 解决：把

    ```cmake
    if(CTK_DIR) include(UseCTK.cmake)
    elseif(CTK_CMAKE_DIR) include(ctkFunctionExtractOptimizedLibrary)
    endif()
    ```

    改为两个独立 `if()`，任一存在即可包含。

* **Elastix 只有 UI、没有运行库**
  * 说明打包只拾取了 `SlicerElastix`。
  * 解决：补上运行库扩展或在设置中指向外部二进制。

## Dcm2niixGUI 脚本修复点（已合入预装扩展）

* 路径：`lib/Alice-5.8/qt-scripted-modules/Dcm2niixGUI.py`
* 关键逻辑：
  * Windows 优先：`Resources/bin/dcm2niix.exe`
  * 回退：`<app>/bin/dcm2niix.exe`，最后 `shutil.which("dcm2niix")`
* 若本地化调整，仅需改安装树中的脚本文件，无需重编。

## 附录：FetchContent 扩展的早期注入钩子

部分扩展使用 `FetchContent` 拉取自身依赖，需要在 `project()` 之前拿到 CTK/SEM 宏与扩展生成脚本。可在 `SuperBuild/Extensions.cmake` 顶部写入一次性初始化脚本，并通过 `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` 注入：

```cmake
set(_rs_ext_init "${CMAKE_BINARY_DIR}/E/_rs_ext_init.cmake")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/E")
file(WRITE "${_rs_ext_init}" [=[
  if(NOT DEFINED Slicer_DIR OR "${Slicer_DIR}" STREQUAL "")
    return()
  endif()

  # 解析 superbuild 根，定位 CTK/SEM，并尝试回填缺失变量
  get_filename_component(_rs_root "${Slicer_DIR}" DIRECTORY)
  get_filename_component(_rs_root "${_rs_root}" DIRECTORY)

  # CTK
  if(NOT DEFINED CTK_DIR)
    if(EXISTS "${_rs_root}/CTK-build/CTKConfig.cmake")
      set(CTK_DIR "${_rs_root}/CTK-build" CACHE PATH FORCE)
      list(APPEND CMAKE_PREFIX_PATH "${CTK_DIR}")
    endif()
  endif()

  # SEM
  if(NOT DEFINED SlicerExecutionModel_DIR)
    if(EXISTS "${_rs_root}/SlicerExecutionModel-build/SlicerExecutionModelConfig.cmake")
      set(SlicerExecutionModel_DIR "${_rs_root}/SlicerExecutionModel-build" CACHE PATH FORCE)
      list(APPEND CMAKE_PREFIX_PATH "${SlicerExecutionModel_DIR}")
    endif()
  endif()

  # CTKAppLauncher
  if(NOT DEFINED CTKAppLauncher_DIR)
    if(EXISTS "${_rs_root}/CTKAppLauncher-build/CTKAppLauncherConfig.cmake")
      set(CTKAppLauncher_DIR "${_rs_root}/CTKAppLauncher-build" CACHE PATH FORCE)
    elseif(EXISTS "${_rs_root}/CTKAppLauncherLib-build/CTKAppLauncherLibConfig.cmake")
      set(CTKAppLauncher_DIR "${_rs_root}/CTKAppLauncherLib-build" CACHE PATH FORCE)
    endif()
  endif()

  # 回填扩展生成/CPack脚本与测试模板
  set(_rs_src "${_rs_root}/slicersources-src")
  if(EXISTS "${_rs_src}/CMakeLists.txt")
    if(NOT Slicer_PYTHON_MODULE_TEST_TEMPLATES_DIR AND
       EXISTS "${_rs_src}/Base/QTCore/Testing/Python")
      set(Slicer_PYTHON_MODULE_TEST_TEMPLATES_DIR
        "${_rs_src}/Base/QTCore/Testing/Python" CACHE PATH FORCE)
    endif()
    if(NOT Slicer_EXTENSION_GENERATE_CONFIG AND
       EXISTS "${_rs_src}/CMake/SlicerExtensionGenerateConfig.cmake")
      set(Slicer_EXTENSION_GENERATE_CONFIG
        "${_rs_src}/CMake/SlicerExtensionGenerateConfig.cmake" CACHE FILEPATH FORCE)
    endif()
    if(NOT Slicer_EXTENSION_CPACK AND
       EXISTS "${_rs_src}/CMake/SlicerExtensionCPack.cmake")
      set(Slicer_EXTENSION_CPACK
        "${_rs_src}/CMake/SlicerExtensionCPack.cmake" CACHE FILEPATH FORCE)
    endif()
  endif()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
]=])
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${_rs_ext_init}" CACHE STRING "Radiance extension init hook" FORCE)
```

## 中文翻译

本项目默认使用中文界面，翻译文件来自 `SlicerLanguageTranslations-main`。

### 自动部署（推荐）

使用 `Invoke-RadianceBuild.ps1` 构建时会**自动部署翻译**，无需手动操作：

```powershell
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -InnerOnly -InnerConfig RelWithDebInfo -Jobs 6
```

### 手动部署

如需手动部署翻译（例如更新翻译后）：

```powershell
# 自动检测安装目录
pwsh Tools/Deploy-Translations.ps1 -BuildDir "C:/S/vs-dev/Slicer-build"

# 或指定输出目录
pwsh Tools/Deploy-Translations.ps1 -OutputDir "C:/S/rs-install/bin/translations"

# 如果Qt路径检测失败，手动指定
pwsh Tools/Deploy-Translations.ps1 -QtDir "C:/Qt/5.15.2/msvc2019_64" -OutputDir "C:/S/rs-install/bin/translations"
```

### 配置说明

* **默认语言**：`zh_CN`（在 `DefaultSettings.ini` 中配置）
* **国际化已启用**：`Slicer_BUILD_I18N_SUPPORT=ON`
* 翻译源文件：`SlicerLanguageTranslations-main/translations/`
  * `Slicer_zh-CN.ts` - Slicer核心界面
  * `CTK_zh-CN.ts` - CTK库
* 编译后的 `.qm` 文件位置：`<安装目录>/bin/translations/`

## 常用命令速查

* 顶层配置：`cmake --preset vs17-dev`
* 内层一键（推荐）：`pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -InnerOnly -InnerConfig RelWithDebInfo -Jobs 6`
* 内层手工构建：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo -- /m:6`
* 内层安装：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target INSTALL -- /m:6`
* 打包（NSIS）：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target package -- /m:6`
* **部署中文翻译**：`pwsh Tools/Deploy-Translations.ps1 -OutputDir "C:/S/rs-install/bin/translations"`
