# Alice/RadianceSuite 构建与打包指南（Windows）

本指南沉淀了本仓库在 VS2022 多配置环境下的“稳定黄金路径”，覆盖日常开发构建（RelWithDebInfo）、安装、打包分发（NSIS）以及排错要点。默认前缀与路径如下，可按需替换。

- 顶层源码：`C:/RS`
- 内层构建树：`C:/S/vs-dev`（由 `CMakePresets.json: vs17-dev` 生成）
- 安装前缀：`C:/S/rs-install`
- 结构化日志：`C:/S/logs/*.binlog`

## 先决条件

- 安装 VS2022（含 C++ 工具链与 Windows SDK），安装 Git、CMake 3.29+、Ninja（推荐 1.11.x）。
- 安装 Qt5.15.2 MSVC2019_64，或设置 `QT5_DIR` 指到其 CMake 包路径。
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

# 安装到统一前缀
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target INSTALL -- /m:6
```

3) 打包（可分发 NSIS 安装器）

若首次打包 SimpleITK 未就绪，先构建外层的 SITK（RelWithDebInfo），再执行 package：

```
# 准备（一次性）：
cmake --build C:/S/vs-dev/slicersources-build --config RelWithDebInfo --target SimpleITK -- /m:6

# 打包（CPack 会完成完整依赖修复/Qt 部署）
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target package -- /m:6 /v:m /bl:C:/S/logs/inner-relwithdebinfo-package.binlog

# 产物：C:/S/vs-dev/Slicer-build/Alice-<version>-win-amd64.exe
```

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
- 脚本 `-InnerOnly` 会把上述定义同步到所有 `Slicer-cache-*.cmake`，避免不同配置不一致。

## 常见误区

- 不要用 `--target Slicer`（顶层母目标），日常迭代请用“内层三步”或脚本 `-InnerOnly`。
- VS 方案中没有 `Slicer-configure` 等 step 目标；若直接调用会报不存在。
- 安装树缺 DLL：请优先通过 package，由 CPack 完整修复；开发期临时可在 `bin/AliceLauncherSettings.ini` 的 `[Paths]` 追加外层 EP 的 `.../bin/RelWithDebInfo`。

## Dcm2niixGUI 脚本修复点（已合入预装扩展）

- 路径：`lib/Alice-5.8/qt-scripted-modules/Dcm2niixGUI.py`
- 关键逻辑：
  - Windows 优先：`Resources/bin/dcm2niix.exe`
  - 回退：`<app>/bin/dcm2niix.exe`，最后 `shutil.which("dcm2niix")`
- 若本地化调整，仅需改安装树中的脚本文件，无需重编。

## 常用命令速查

- 顶层配置：`cmake --preset vs17-dev`
- 内层一键（推荐）：`pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -InnerOnly -InnerConfig RelWithDebInfo -Jobs 6`
- 内层手工构建：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo -- /m:6`
- 内层安装：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target INSTALL -- /m:6`
- 打包（NSIS）：`cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target package -- /m:6`

