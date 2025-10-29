# 一键开发构建（Windows）

> 目的：把“准备环境变量 + 配置 + 仅编应用”打包，避免误触发 Slicer SuperBuild，加速日常迭代。

## 新脚本：Tools/Dev-Build.ps1

- 核心特性：
  - 使用共享 Slicer（UseSharedSlicer）进行配置与构建，跳过外部依赖 SuperBuild。
  - 自动注入 Qt5 CMake 路径（可显式传入 `-QtCMakeDir`）。
  - 可按需创建共享 Slicer 并可持久设置环境变量（`-SetupShared -PersistEnv`）。
  - 配置阶段禁止任何回退或短盘符触发（内部强制 `-ConfigureOnly` + `-AutoShortDriveSlicer:$false`）。
  - 构建阶段只编制应用目标（默认 `RadianceApp`）。

### 用法示例

- 配置并仅编应用（推荐日常开发）
  - `pwsh Tools/Dev-Build.ps1 -Action build -UseSharedSlicer -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 -Jobs 0`

- 只做配置（修改了 CMake/预设后先配置）
  - `pwsh Tools/Dev-Build.ps1 -Action configure -UseSharedSlicer -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`

- 首次或需要创建共享 Slicer，并持久化环境变量
  - `pwsh Tools/Dev-Build.ps1 -SetupShared -PersistEnv -UseSharedSlicer -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 -Action build`

- 避免 LNK1168（构建前自动结束占用进程）
  - `pwsh Tools/Dev-Build.ps1 -Action build -UseSharedSlicer -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5 -KillRunningApp`

### 参数说明（常用）

- `-Action`：`configure` 或 `build`（默认 `build`）。
- `-Preset`：`win-ninja-dev` 或 `win-ninja-rel`（默认 `win-ninja-dev`）。
- `-QtCMakeDir`：Qt5 CMake 目录（例如 `C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`）。
- `-UseSharedSlicer`：使用共享 Slicer（默认启用）。
- `-SharedSrcDir`/`-SharedBinDir`：共享 Slicer 源码/构建目录（默认 `C:/W/Slicer` 和 `C:/W/Slicer-build`）。
- `-SetupShared`：如不存在则克隆/配置共享 Slicer（内部调用 `Tools/Setup-SharedSlicer.ps1`）。
- `-PersistEnv`：把 `SLICER_SRC_DIR`/`SLICER_BIN_DIR` 写入用户环境变量。
- `-BuildTarget`：应用构建目标（默认 `RadianceApp`）。
- `-Jobs`：并行度（默认 `0` = 自动）。
- `-KillRunningApp`：构建前自动结束 `Alice.exe`/`AliceApp-real.exe`/`SlicerDesigner.exe`，避免链接时报 `LNK1168 cannot open *.dll`。

### 运行与验证

- 运行应用：`R:\Alice.exe`（等价 `../RS-build/win-ninja-dev/Slicer-build/Alice.exe`）。
- 仅 Python/UI 改动：在运行中热重载即可：`slicer.util.reloadScriptedModule("YourModule")`。

## 现有脚本的新增开关

- `Tools/Invoke-RadianceBuild.ps1` 新增 `-ConfigureOnly`：
  - 只执行 CMake 配置，跳过所有构建、打包与回退逻辑。
  - 示例：`pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -ForceConfigure -ConfigureOnly`

## 先决条件

- 已安装 VS C++ 组件与 Windows 10 SDK（脚本会自动导入 VS 开发环境）。
- Qt5 安装并提供 CMake 配置目录（通过 `-QtCMakeDir` 传入或预先设置 `QT5_DIR` 环境变量）。
- 共享 Slicer：推荐 `C:/W/Slicer` 与 `C:/W/Slicer-build`（等价 `Q:`）。
