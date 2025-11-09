# Alice/RadianceSuite 构建与打包指南（Windows）

本指南沉淀了本仓库在 VS2022 多配置环境下的“稳定黄金路径”，覆盖日常开发构建（RelWithDebInfo）、安装、打包分发（NSIS）以及排错要点。默认前缀与路径如下，可按需替换。

- 顶层源码：`C:/RS`
- 内层构建树：`C:/S/vs-dev`（由 `CMakePresets.json: vs17-dev` 生成）
- 安装前缀：`C:/S/rs-install`
- 结构化日志：`C:/S/logs/*.binlog`

## 先决条件

- 安装 VS2022（含 C++ 工具链与 Windows SDK），安装 Git、CMake 3.29+、Ninja（推荐 1.11.x）。
- 安装 Qt5.15.2 MSVC2019_64，或设置 `QT5_DIR` 指到其 CMake 包路径。
- 确认 NSIS 3.x 已安装且在 `PATH` 中；若尚未配置，可临时使用 `cpack -G ZIP` 做打包冒烟测试。
- 一键准备（可选）：
  - 引导：`pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey`
  - 设置 Qt：`pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`

## 预设与脚本

- 配置预设（VS 多配置）：`vs17-dev`（见根目录 `CMakePresets.json`）
- 构建脚本：`Tools/Invoke-RadianceBuild.ps1`
  - 关键参数：
    - `-InnerOnly`：直驱“内层三步”（不触发母目标 `Slicer`）。
    - `-InnerConfig <Debug|Release|RelWithDebInfo|MinSizeRel>`：内层配置，默认 `RelWithDebInfo`。
    - `-Jobs N`：并行度；R5 3600 建议 `6`，32GB+NVMe 可试 `8`。
    - `-ConfigureOnly`：仅配置，跳过编译。

> 说明：VS 方案内不存在 `Slicer-configure/Slicer-build/Slicer-install` 这类 step 目标，`-InnerOnly` 会直接对 `slicersources-src/Slicer-build` 调用 CMake 的 configure/build/install，完全避开母目标。

## 黄金路径（RelWithDebInfo）

1) 生成顶层 VS 方案（很快）

```
cmake --preset vs17-dev
```

2) 内层构建 + 安装（稳定、清晰）

方式 A（推荐，一条命令全流程并生成结构化日志）：

```
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -InnerOnly -InnerConfig RelWithDebInfo -Jobs 6
```

方式 B（与脚本等价的手工三步）：

```
# 仅首次或切换配置时需要
cmake -G "Visual Studio 17 2022" -A x64 -S C:/S/vs-dev/slicersources-src -B C:/S/vs-dev/Slicer-build -C C:/S/vs-dev/slicersources-build/Slicer-prefix/tmp/Slicer-cache-RelWithDebInfo.cmake

# 构建（结构化日志）
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo -- /m:6 /v:m /bl:C:/S/logs/inner-relwithdebinfo.binlog
```

> ⚠️ `INSTALL` / `PACKAGE` 不会自动补齐缺失的库。**手工执行时务必坚持 “ALL_BUILD → INSTALL → PACKAGE” 的顺序**，这样 SimpleITK、MRML、qSlicer\* 等依赖才会被完整拷贝到 staging/安装前缀。脚本 `Invoke-RadianceBuild.ps1 -InnerOnly` 已经内置该顺序，仅在单独调用 CMake/MSBuild 时需要逐条执行：
>
> ```powershell
> # 2) 内层构建（等价于 --target Slicer，但推荐 ALL_BUILD 以免混淆）
> cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target ALL_BUILD -- /m:6 /v:m
> # 2.5) 安装到统一前缀（建议在打包前至少执行一次）
> cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target INSTALL -- /m:6
> # 3) 打包（NSIS/ZIP）
> cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target PACKAGE -- /m:6 /v:m
> # 若调试 NSIS，可用 ZIP 验证：
> # cpack -C RelWithDebInfo -G ZIP --config C:/S/vs-dev/Slicer-build/CPackConfig.cmake
> ```

3) 打包（可分发 NSIS 安装器）

若首次打包 SimpleITK 未就绪，先构建外层的 SITK（RelWithDebInfo），再执行 package：

```
# 准备（一次性）：
cmake --build C:/S/vs-dev/slicersources-build --config RelWithDebInfo --target SimpleITK -- /m:6

# 打包（CPack 会完成完整依赖修复/Qt 部署）
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target package -- /m:6 /v:m /bl:C:/S/logs/inner-relwithdebinfo-package.binlog

# 产物：C:/S/vs-dev/Slicer-build/Alice-<version>-win-amd64.exe
```

### 打包产物自检（定位最常见的“找不到安装包/缺库”问题）

```powershell
# 安装包命名/路径（含配置名称）
Select-String C:/S/vs-dev/Slicer-build/CPackConfig.cmake -Pattern "CPACK_PACKAGE_FILE_NAME|CPACK_INSTALL_CMAKE_PROJECTS"
# 安装前缀（确认 INSTALL 指向期望目录）
Select-String C:/S/vs-dev/Slicer-build/CMakeCache.txt -Pattern "CMAKE_INSTALL_PREFIX"
# NSIS 是否可用
where makensis
# 安装包查找（同时可观察大小）
Get-ChildItem C:/S/vs-dev/Slicer-build -Recurse -Filter "Alice-*.exe" | Select-Object FullName, Length
# 关键 DLL 是否已生成（示例：某些 MRML/qSlicer 组件）
Get-ChildItem C:/S/vs-dev/Slicer-build -Recurse -Filter "*MRML*IDIO*.dll"
```

若仍无法出包，可直接运行：

```powershell
cpack -C RelWithDebInfo -G NSIS -V --config C:/S/vs-dev/Slicer-build/CPackConfig.cmake
```

记录日志中的第一条 `FATAL` / `ERROR` 即可快速定位缺失的具体文件。

## 常用验证

- 安装树启动：`C:/S/rs-install/Alice.exe`
- Python 控制台：

```
import vtk, qt, ctk, slicer
print('VTK:', vtk.vtkVersion.GetVTKVersion())
print('CTK OK:', hasattr(ctk, 'ctkCollapsibleButton'))
```

- 模块检查：
  - `Dcm2niixGUI` 可见且可打开；能找到 `Resources/bin/dcm2niix.exe` 或 `app/bin/dcm2niix.exe`。
  - Segment Editor 效果下拉包含 `LocalThreshold / FastMarching / DrawTube` 等。
  - 注册类模块包含 `General Registration (Elastix)`；首次运行如提示找不到 `elastix/transformix`，在模块齿轮或 `Edit → Application Settings → Modules → SlicerElastix` 中指向扩展内的二进制（通常会自动识别）。
  - `Landmark Registration` 可在 Modules → Registration → Landmark Registration 下找到，并能载入示例场景。
  - `Elastix/Transformix` 可执行已随扩展落到安装树：`Get-ChildItem C:/S/rs-install -Recurse -Include elastix.exe,transformix.exe | Select-Object FullName`

## 结构化日志与进度估算

- 所有长流程都可加 MSBuild binlog：`/v:m /bl:C:/S/logs/<name>.binlog`
- 使用 MSBuild Structured Log Viewer 查看“当前活跃 target”，判断是否处于链接/打包等单工序阶段。

## 切换/清理注意事项

- 不混用 Release 与 RelWithDebInfo。若误跑过 Release，可清理：

```
Remove-Item -Recurse -Force C:/S/vs-dev/Slicer-build/bin/Release -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force C:/S/vs-dev/Slicer-build/lib/Release -ErrorAction SilentlyContinue
```

- 仅在“初始 cache 失效或要强制更新内层默认值”时做最小清理：

```
Remove-Item -Recurse -Force C:/S/vs-dev/Slicer-build -ErrorAction SilentlyContinue
Get-ChildItem C:/S/vs-dev/slicersources-build/Slicer-prefix/tmp/ -Filter 'Slicer-cache-*.cmake' -ErrorAction SilentlyContinue | Remove-Item -Force
```

## 变量与扩展（已内置转发）

- 已在顶层 `CMakeLists.txt` 明确并转发：
  - `Slicer_USE_SimpleITK=ON`（已写入 Cache，确保内层生效）
  - `Slicer_EXTENSION_SOURCE_DIRS`、`Slicer_EXTENSION_INSTALL_DIRS`
- 脚本 `-InnerOnly` 会把上述定义同步到所有 `Slicer-cache-*.cmake`，避免不同配置不一致；它不会触发顶层 `--target Slicer`，但会在 `Slicer-build` 内完整执行 `ALL_BUILD → INSTALL`。若手工分步，请自行按该顺序跑齐。

### 扩展打包：Elastix + LandmarkRegistration

为保证 `General Registration (Elastix)` UI 与底层 CLI 同步落包，请在 `SuperBuild/Extensions.cmake` 中同时声明运行库扩展与 UI 扩展，例如：

```cmake
_bundle_ext(Elastix              https://github.com/SuperElastix/elastix-slicer-extension.git  <tag>)
_bundle_ext(SlicerElastix        https://github.com/lassoan/SlicerElastix.git                   <tag>)
_bundle_ext(LandmarkRegistration https://github.com/SlicerIGT/LandmarkRegistration.git          <tag>)
```

打包前可在内层目录验证三者均已编译：

```powershell
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target ^
  ext_elastix ext_slicerelastix ext_landmarkregistration -- /m:6 /v:m
```

安装树中若缺少 `elastix.exe/transformix.exe`，说明运行库扩展未被拾取，需要重新构建并安装后再执行 `PACKAGE`。

## 常见误区

- 不要用顶层 VS 方案里的 `--target Slicer`（那是母目标），日常迭代请用“内层三步”或脚本 `-InnerOnly`。**在 `Slicer-build/` 内**，`--target ALL_BUILD`（或等价的 `--target Slicer`）是安全且推荐的。
- VS 方案中没有 `Slicer-configure` 等 step 目标；若直接调用会报不存在。
- 安装树缺 DLL：请优先通过 package，由 CPack 完整修复；开发期临时可在 `bin/AliceLauncherSettings.ini` 的 `[Paths]` 追加外层 EP 的 `.../bin/RelWithDebInfo`。

## 常见故障排查

- **CPack 提示 `file INSTALL cannot find ...` / `File exists`**  
  说明 staging 中缺少构件或残留旧文件。请严格按 `ALL_BUILD → INSTALL → PACKAGE` 重新执行，随后清理旧的打包缓存再试：
  ```powershell
  Remove-Item C:/S/vs-dev/Slicer-build/_CPack_Packages -Recurse -Force -ErrorAction SilentlyContinue
  ```
- **General Registration 只能看到 UI，运行时找不到 `elastix/transformix`**  
  运行库扩展 (`Elastix`) 未被打入包，仅 UI 扩展 (`SlicerElastix`) 生效。补齐 `_bundle_ext(Elastix ...)` 并重新构建/安装/打包；必要时用 `cmake --build ... --target ext_elastix` 检查它是否成功，确保安装树能找到 `elastix.exe` 与 `transformix.exe`。

## Dcm2niixGUI 脚本修复点（已合入预装扩展）

- 路径：`lib/Alice-5.8/qt-scripted-modules/Dcm2niixGUI.py`
- 关键逻辑：
  - Windows 优先：`Resources/bin/dcm2niix.exe`
  - 回退：`<app>/bin/dcm2niix.exe`，最后 `shutil.which("dcm2niix")`
- 若本地化调整，仅需改安装树中的脚本文件，无需重编。

## 附录：FetchContent 扩展的早期注入钩子

部分扩展使用 `FetchContent` 拉取自身依赖，需要在 `project()` 之前拿到 CTK/SEM 宏。可在 `SuperBuild/Extensions.cmake` 顶部写入一次性初始化脚本，并通过 `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` 注入：

```cmake
set(_rs_ext_init "${CMAKE_BINARY_DIR}/E/_rs_ext_init.cmake")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/E")
file(WRITE "${_rs_ext_init}" [=[
  if(NOT DEFINED Slicer_DIR OR "${Slicer_DIR}" STREQUAL "")
    return()
  endif()
  # 1) 解析 superbuild 根，定位 CTK_DIR 并 include(UseCTK.cmake)
  # 2) 如有需要，解析 SlicerExecutionModel_DIR，补齐 CLI/MODULE 搜索路径
  # 3) 去重 CMAKE_PREFIX_PATH / CMAKE_MODULE_PATH，避免重复注入
]=])
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${_rs_ext_init}" CACHE STRING "" FORCE)
```

如此即可在不修改 Slicer 源码的前提下，让每个扩展在最早期就能访问 CTK/SEM 宏及共享前缀设置。

## 常用命令速查

- 顶层配置：`cmake --preset vs17-dev`
- 内层一键（推荐）：`pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -InnerOnly -InnerConfig RelWithDebInfo -Jobs 6`
- 内层手工构建：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo -- /m:6`
- 内层安装：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target INSTALL -- /m:6`
- 打包（NSIS）：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target package -- /m:6`
