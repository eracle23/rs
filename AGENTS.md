# Repository Guidelines

## Project Structure & Module Organization
- `Applications/RadianceApp` — C++/Qt app (e.g., `Main.cxx`, `qRadianceAppMainWindow.*`), resources under `Resources/`.
- `Modules/Scripted/Home` — Python Slicer module with `.py`, `.ui`, `.qss`, and icons.
- `Tools` — PowerShell tooling: `Bootstrap-Prereqs.ps1`, `Setup-BuildEnv.ps1`, `Setup-SharedSlicer.ps1`, `Invoke-RadianceBuild.ps1`.
- `Docs` — developer docs and agent notes.
- Top-level: `CMakeLists.txt`, `CMakePresets.json`. Builds are out-of-tree in `../RS-build/...`.

## Build, Test, and Development Commands
- Bootstrap (admin): `pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey`
- Set Qt path (one-time): `pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
- Fast dev build (Ninja): `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -Jobs 0`
- Release + package: `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -UseSharedSlicer -Package`
- Run from build tree: `../RS-build/win-ninja-dev/Slicer-build/<AppName>.exe`

## Coding Style & Naming Conventions
- C++: `.cxx`/`.h`, 2-space indent, CamelCase types and methods; Qt-style prefixes (e.g., `qRadianceAppMainWindow`).
- CMake: 2-space indent, lower-case commands; prefer cache in `CMakePresets.json` over ad‑hoc edits.
- Python: PEP 8, 4-space indent, snake_case modules; keep UI logic in `Modules/Scripted`; avoid blocking Qt calls.
- Assets: place under adjacent `Resources/` and register via `.qrc`.

## Testing Guidelines
- Tests default OFF (`BUILD_TESTING=OFF`). Enable: `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -ExtraCMakeArgs '-DBUILD_TESTING=ON'`.
- From build dir: `ctest --output-on-failure --preset test-dev`.
- Name tests `*Test.cxx` (C++) or `test_*.py` (Python) and register with `add_test()` in CMake.

## Commit & Pull Request Guidelines
- Commit messages: imperative, scoped (Conventional Commits): `feat(app): ...`, `fix(modules): ...`, `build(tools): ...`.
- One logical change per commit; keep diffs focused.
- PRs: include summary, linked issues (`#123`), build preset used, and screenshots/GIFs for UI changes. Ensure builds pass.

## Security & Configuration Tips
- Do not commit secrets or absolute local paths.
- Prefer out-of-tree builds (`../RS-build`) to avoid long‑path issues; use forward slashes for Qt paths.
- Honored env vars: `QT5_DIR`, `SLICER_SRC_DIR`, `SLICER_BIN_DIR`.

用中文回复问题。