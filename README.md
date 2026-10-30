# 回归资料补充归档：重建候选源码

本分支用于保存重建候选源码和后续补充的回归资料，按真实整理时间追加记录。

**当前源码于 2026-10-28 依据现有源码与测试报告重建，尚不能认定为三个历史测试日期的原始受测版本。**

本次归档说明编制时间：**2026-10-30T12:02:18+08:00**。

完整的阶段索引、来源、历史日期实验经过及证据缺口见 [回归归档说明](回归归档说明.md)。原始候选包的来源与限制见 [重建说明](重建说明.txt)。

## 历史提交日期说明

历史中三个标题含有 **DEMO** 的提交用于 GitHub 日期显示实验，其作者日期、提交者日期均为人为设置的示例日期，不能作为当日上传、受测、通过回归或正式归档的证据。

该实验在 2026-10-30 进行，并通过强制推送替换了当时的分支提交链；这段经过在归档说明中保留。从本次说明开始，以真实提交时间追加归档和修订记录。

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
