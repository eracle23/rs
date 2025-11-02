# 日常开发提速与增量构建指南（Windows）

> 目的：在不牺牲稳定性的前提下，显著降低本地迭代耗时。适用于使用共享 Slicer（UseSharedSlicer）的日常开发流程。

## 一次性/首轮构建为何很慢
- SuperBuild 会从源码编译 VTK/ITK/CTK/Python/HDF5/Teem 等大型依赖，首次耗时长属正常。
- 稳定后，日常迭代只需编译增量目标或直接同步脚本/资源，绝大多数改动不需要重跑整套 SuperBuild。

## 推荐工作流（最重要）
- 在 VS 开发环境里构建
  - 打开“x64 Native Tools Command Prompt for VS 2022”或“Developer PowerShell for VS 2022”。
  - 确保 `cl`/`rc`/`mt` 可用，SDK 路径完整，避免 `kernel32.lib` 等链接失败导致反复尝试耗时。
- 共享 Slicer，避免反复 SuperBuild（VS 预设）
  - 首次/必要时：`pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -UseSharedSlicer -ForceConfigure -Jobs 0`
  - 日常：`pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -UseSharedSlicer -Jobs 0`
  - 不要频繁 `-ForceConfigure`，仅在缓存损坏或预设/工具链变更时使用。
- 仅构建应用目标（C++ 改动，指向内层 Slicer-build）
  - `cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target RadianceApp -- /m`
  - 更快链接：关闭正在运行的 `<AppName>.exe`，避免 `LNK1168 cannot open *.dll`。
- Python/界面（零编译增量）
  - 运行中热重载：`slicer.util.reloadScriptedModule("YourModule")`
- 启用编译缓存（sccache）
  - 首次配置时加：`-ExtraCMakeArgs '-DCMAKE_CXX_COMPILER_LAUNCHER=sccache;-DCMAKE_C_COMPILER_LAUNCHER=sccache'`
  - 查看命中率：`sccache --show-stats`

## 进一步的稳态优化
- 短路径/长命令行
  - Preset 或脚本已启用：`-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON`、`-DCMAKE_OBJECT_PATH_MAX=128`。
- 杀软白名单（减少 try_run/小测试被拦截）
  - 将以下加入白名单：`C:\W\Slicer-build\*`、`C:\RS-build\win-ninja-dev\*`、`C:\Program Files\CMake\bin\cmake.exe`、`C:\ProgramData\chocolatey\bin\ninja.exe`。
- 避免不必要的 CMake 变更
  - 小改动尽量不动顶层 `CMakeLists.txt`；参数尽量通过 `CMakePresets.json` 或脚本的 `-ExtraCMakeArgs` 传递。

## 常用命令速查
- 首次或必要时的全配置（共享 Slicer）：
  - `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -UseSharedSlicer -ForceConfigure -Jobs 0`
- 日常快速构建（共享 Slicer）：
  - `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -UseSharedSlicer -Jobs 0`
- 仅编应用（C++ 改动，内层）：
  - `cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target RadianceApp -- /m`
- 运行：
  - `C:/S/vs-dev/Slicer-build/bin/RelWithDebInfo/<AppName>-real.exe`

## 故障排查（避免全仓重编）
- 链接失败 `LNK1168`：关闭正在运行的 `<AppName>.exe` 后重试，仅编目标。
- 共享 Slicer 失败在第三方库：
  - 仅重建对应目标：例如 `cmake --build C:/W/Slicer-build -j 1 --target teem`（通过后再整体构建）。
- 内层生成器损坏/需要重配：
  - 在共享 Slicer 目录用 VS 生成器重新配置：
    - `cmake -G "Visual Studio 17 2022" -A x64 -S C:/W/Slicer -B C:/W/Slicer-build -DQt5_DIR=C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
    - `cmake --build C:/W/Slicer-build --config RelWithDebInfo -- /m`

> 说明：本指南与脚本 `Tools/Invoke-RadianceBuild.ps1` 协同工作；脚本已内置短路径/稳定化参数与常见问题补救（Windows SDK 路径、Teem QNaN 探测修复、HDF5/VTK 兼容项等）。
