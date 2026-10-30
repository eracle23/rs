# 日期显示 DEMO：重建候选源码

> 本分支用于验证 GitHub 对自定义提交日期的显示。三个阶段提交中的历史日期均为人为设置的演示元数据。
> 它们不表示源码在该日创建、上传或通过测试，也不能作为当日测试、发布或归档的证据。

当前阶段：**单元测试回归候选源码（DEMO）**。

源码候选包最初依据现有源码与报告于 **2026-10-28** 重建。

本次日期显示实验的真实执行时间：**2026-10-30T11:35:31+08:00**。实际推送在本次实验期间发生。

| 演示阶段 | 人为设置的示例提交日期 | 改日期前的源码快照提交 |
| --- | --- | --- |
| 单元测试回归 | 2025-12-16 12:00:00 +08:00 | `45285d83b1a1284b688573371889fab9ba584e59` |
| 集成测试回归 | 2026-01-09 12:00:00 +08:00 | `2ed51d0dafc702d0b83ca719e5c4ff42a10e2ee6` |
| 系统测试回归 | 2026-02-03 12:00:00 +08:00 | `0e084091cdf54853d4367e56075c909629b750e3` |

三个阶段的程序源码与上表所列快照一致。本次仅增加 DEMO 说明并调整提交标题、日期和父提交编号。

完整构建与回归执行尚未验证；来源和限制继续保留在 [重建说明.txt](重建说明.txt) 中。

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
