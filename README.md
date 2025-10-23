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
- Quick build: `pwsh Tools/Invoke-RadianceBuild.ps1 -QtDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
- Shared Slicer (one-time): `pwsh Tools/Setup-SharedSlicer.ps1 -SetEnv`
- Use shared Slicer: `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer`
- Set env vars (one-time): `pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
- Out-of-tree is default (../RS-build); no flag needed

![RadianceSuite by Radiance Labs](Applications/RadianceApp/Resources/Images/LogoFull.png?raw=true)
