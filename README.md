# 回归源码补充归档

本分支用于保存对照单元、集成和系统回归测试报告整理的候选源码，以及后续补充的回归资料。

**现有候选源码于 2026-10-28 依据现有源码与测试报告重建，并于 2026-10-30 进行本分支的日期调整和补充归档。历史节点日期按报告参考日期设置，不表示当日实际提交或推送。**

本次说明更新时间：**2026-10-30T12:05:45+08:00**。

开发与测试活动的时间、版本对应关系应以可核实的本地原始记录为依据。目前未提供三次回归的原始源码快照或构建产物，现有候选源码与当时受测版本的一致性尚未证实。

阶段索引、来源和日期调整经过见 [回归归档说明](回归归档说明.md)；源码重建范围见 [重建说明](重建说明.txt)。后续归档以真实整理日期正常追加提交。

---

# RadianceSuite by Radiance Labs

A re-skinned medical imaging workstation built on Slicer

_This project is in active development and may change from version to version without notice._

## Table of contents

- [Features](#features)
- [Development](#development)

## Features

- Radiance Studio shell without visible Slicer branding, curated menus, and custom toolbars.
- Home workspace module with quick actions, workflow cards, and documentation links.
- Radiance workspace layout (3D + paired axial/sagittal) registered alongside classic four-up.
- Branded palette and component styling delivered through global QSS and `qAppStyle`.
- Settings HUD to toggle full Slicer UI and theme at runtime for debugging.

## Development

- [Contributing](CONTRIBUTING.md)
- [Building](BUILD.md)
- Bootstrap prereqs (admin): `pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey`
- Quick build (VS/MSBuild, recommended): `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -Jobs 0`
- Incremental app-only build (VS inner tree): `cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target RadianceApp -- /m`
- Shared Slicer (one-time): `pwsh Tools/Setup-SharedSlicer.ps1 -SetEnv`
- Use shared Slicer (VS): `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -UseSharedSlicer`
- Set env vars (one-time): `pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir D:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
- Out-of-tree for VS: `C:/S/vs-dev`（由 `CMakePresets.json` 定义）

![RadianceSuite by Radiance Labs](Applications/RadianceApp/Resources/Images/LogoFull.png?raw=true)
