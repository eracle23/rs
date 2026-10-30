# 回归源码补充归档

本分支用于保存对照单元、集成和系统回归测试报告整理的重建候选源码，以及后续补充的回归资料。

阶段及测试日期以相应报告记载为准，不代表 Git 提交或推送日期。

当前候选源码与历史受测版本的一致性尚待核实。源码来源、日期调整经过及阶段索引见 [回归归档说明](回归归档说明.md)；重建范围见 [重建说明](重建说明.txt)。

后续资料通过新增提交补充，修订记录可在提交历史中查看。

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
