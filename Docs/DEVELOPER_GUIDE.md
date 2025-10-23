# Developer Guide

RadianceSuite is a custom 3D Slicer application with a fast, modern Windows build pipeline.

- Defaults: out-of-tree builds, Ninja + sccache, shared Slicer superbuild, automatic long-path and Qt fixes.
- Targets Windows x64 (Windows 10/11).

## Prerequisites

- Visual Studio 2022 Build Tools (C++ workload) + Windows 10 SDK
- CMake 3.23+ (4.1+ OK), Ninja, Git, NSIS 3.x (for packaging)
- Optional: sccache
- Qt 5.15.2 MSVC 2019 x64 CMake files: `C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`

One‑click via Chocolatey:

```
pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey -VSBuildTools -WindowsSDK
```

Optional components: `-All` (CMake/Git), `-NsisVersion`, `-CMakeVersion`.

## Environment Setup

Set env vars once (slashes normalized for CMake):

```
pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5
```

Prepare shared Slicer (recommended for speed/cache reuse):

```
pwsh Tools/Setup-SharedSlicer.ps1 -SetEnv
```

This clones Slicer to `C:\W\Slicer`, checks out the pinned commit, and sets `SLICER_*`. Build caches live under `C:\W\Slicer-build`.

## Build

Dev build (RelWithDebInfo, out‑of‑tree by default):

```
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -Jobs 0
```

Release build:

```
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -UseSharedSlicer -Jobs 0
```

Package installer (NSIS):

```
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -UseSharedSlicer -Package
```

Presets (`CMakePresets.json`) include:

- Ninja generator, out‑of‑tree `../RS-build/...`
- `CMAKE_NINJA_FORCE_RESPONSE_FILE=ON` (long command lines handled by rsp)
- `CMAKE_OBJECT_PATH_MAX=128` (shorter object paths)
- MSVC compilers, sccache launchers
- Shared Slicer variants respect `SLICER_SRC_DIR` / `SLICER_BIN_DIR`

Key script flags (`Tools/Invoke-RadianceBuild.ps1`):

- `-UseSharedSlicer` reuse shared Slicer superbuild
- `-ForceConfigure` force full CMake reconfigure
- `-Jobs <N>` set Ninja parallelism
- `-ExtraCMakeArgs '-DName=Value'` pass extra `-D` cache entries
- `-AutoShortDriveSlicer:$true|$false` auto map short drive for Slicer on long‑path failures (default true)
- `-ShortDriveLetter 'R'` choose drive letter; `-KeepShortDriveMapping` keep mapping after build
- `-OutOfTree` prefers out‑of‑tree variants (default already out‑of‑tree)

## Run

From dev build:

```
C:\RS-build\win-ninja-dev\Slicer-build\Alice.exe
```

If VC runtime is missing, run from VS tools shell or package and install.

## Output Locations

- Project build: `C:\RS-build\win-ninja-<dev|rel>\...`
- Slicer internal build: `C:\RS-build\win-ninja-dev\Slicer-build\...`
- Shared Slicer caches: `C:\W\Slicer-build\...`

## What the Scripts Do

`Tools/Invoke-RadianceBuild.ps1`

- Normalizes `QT5_DIR` to forward slashes to avoid CMake escape issues.
- Prefers MSVC toolchain; injects `mt.exe`/`rc.exe` into PATH and CMake.
- Sanitizes superbuild caches (`*-prefix/tmp/*cache-*.cmake`) to:
  - fix `Qt5_DIR` slashes
  - enforce `CMAKE_NINJA_FORCE_RESPONSE_FILE=ON`
  - enforce `CMAKE_OBJECT_PATH_MAX=128`
- Auto‑creates Python `python3.lib` alias if only `python312.lib` exists and retries once.
- On persistent `.rsp`/path length failures, automatically:
  - `subst` maps a short drive (default `R:`) to `Slicer-build`
  - runs `ninja` to finish Slicer
  - returns to top‑level and resumes superbuild (mapping removed unless `-KeepShortDriveMapping` is set)

`Tools/Bootstrap-Prereqs.ps1`

- Installs Ninja/sccache/NSIS; optional CMake/Git; optional VS Build Tools and SDK.

`Tools/Setup-SharedSlicer.ps1`

- Clone pinned Slicer commit to `C:\W\Slicer`, create `C:\W\Slicer-build`, set `SLICER_*`.

`Tools/Setup-BuildEnv.ps1`

- Sets `QT5_DIR` and optionally `SLICER_*` (slashes normalized).

## Troubleshooting

- Qt path escape error (Invalid character escape `\Q`):
  - Script auto‑normalizes `Qt5_DIR` and fixes superbuild caches; rerun with `-ForceConfigure` if needed.
- Python link error (LNK1104: cannot open `python3.lib`):
  - Script auto‑creates alias from `python312.lib`; if needed add to LIB search path: `set "LIB=%LIB%;C:\RS-build\win-ninja-dev\python-build\libs"`.
- Command line too long / D8022 / `.rsp` cannot open:
  - Enabled response files + short object paths; script auto‑subst short drive to finish Slicer, then resumes top‑level build.
- Wrong linker (Strawberry/MinGW `ld.exe` picked):
  - Script fixes PATH precedence and injects MSVC link/lib tools.
- Missing SDK / `kernel32.lib` not found:
  - Ensure VS Build Tools + Windows 10 SDK; use “x64 Native Tools” shell or let script import `VsDevCmd`.
- Network flakiness (Git/Python downloads):
  - Rerun same build; caches resume; `git config --global http.version HTTP/1.1` can help.
- Disk/Memory:
  - Reserve 40–80 GB; reduce `-Jobs` on low memory.

## Cheat Sheet

Bootstrap:

```
pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey -VSBuildTools -WindowsSDK
```

Setup env:

```
pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5
```

Shared Slicer:

```
pwsh Tools/Setup-SharedSlicer.ps1 -SetEnv
```

Dev build:

```
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -Jobs 0
```

Package:

```
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -UseSharedSlicer -Package
```

Run:

```
C:\RS-build\win-ninja-dev\Slicer-build\Alice.exe
```

## File Map

- `CMakePresets.json` — Ninja presets (out‑of‑tree default, response files, short object paths)
- `Tools/Invoke-RadianceBuild.ps1` — main build driver (auto‑fixes, short‑drive fallback)
- `Tools/Bootstrap-Prereqs.ps1` — Chocolatey bootstrap
- `Tools/Setup-SharedSlicer.ps1` — clone/config shared Slicer
- `Tools/Setup-BuildEnv.ps1` — set `QT5_DIR`/`SLICER_*`

