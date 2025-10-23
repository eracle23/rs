# Build and Package RadianceSuite

This document summarizes how to build and package RadianceSuite on Windows.

RadianceSuite is a custom Slicer application. Reading the [3D Slicer Developer Documentation](https://slicer.readthedocs.io/en/latest/developer_guide/index.html) may help answer additional questions.

The initial source files were created using [KitwareMedical/SlicerCustomAppTemplate](https://github.com/KitwareMedical/SlicerCustomAppTemplate).

## Prerequisites

- Setting up your git account:

  - Create a [Github](https://github.com) account.

  - Setup your SSH keys following [these](https://help.github.com/articles/generating-ssh-keys) instructions.

  - Setup [your git username](https://help.github.com/articles/setting-your-username-in-git) and [your git email](https://help.github.com/articles/setting-your-email-in-git).

  - If not already done, email `FirstName LastName <firstname.lastname@RadianceLabs.com>` to be granted access to
    the [RadianceLabs/RadianceSuite](https://github.com/RadianceLabs/RadianceSuite) repository.

## Checkout

1. Start `Git Bash`
2. Checkout the source code into a directory `C:\W\` by typing the following commands:

```bat
cd /c
mkdir W
cd /c/W
git clone https://github.com/RadianceLabs/RadianceSuite.git R
```

Note: use short source and build directory names to avoid the [maximum path length limitation](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#maximum-path-length-limitation).

## Build

Note: The first full superbuild can take hours (Slicer + dependencies). The steps below prioritize modern tooling and incremental speed.

<b>Option 0 (Fast, Modern): Ninja + CMake Presets + sccache</b>

Prereqs: Visual Studio 2022 (C++), CMake 3.23+, Ninja, Git, optional sccache, NSIS for packaging.

- Install tools (recommended): VS 2022 with Desktop C++ workload, CMake, Ninja, NSIS 3.x, sccache.
- Ensure Qt is installed and note its CMake path (e.g. `C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`).

Steps (from an “x64 Native Tools Command Prompt for VS 2022” or equivalent dev shell):

```pwsh
# In repo root (out-of-tree is default; artifacts in ../RS-build)
pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5  # one-time

# Dev build (RelWithDebInfo)
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -Jobs 0

# Release build
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -Jobs 0

# Package (NSIS installer)
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -Package

# Use shared Slicer sources/build (env SLICER_SRC_DIR/SLICER_BIN_DIR or defaults C:\W\Slicer, C:\W\Slicer-build)
pwsh Tools/Setup-SharedSlicer.ps1 -SetEnv   # one-time (clone + set env)
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer
```

This uses `CMakePresets.json` to configure Ninja builds with `BUILD_TESTING=OFF` and `sccache` (if available) for compiler caching.

<b>Option 1: CMake GUI and Visual Studio</b>

1. Start [CMake GUI](https://cmake.org/runningcmake/), select source directory `C:\W\R` and set build directory to `C:\W\RR`.
2. Add an entry `Qt5_DIR` pointing to `C:/Qt/${QT_VERSION}/${COMPILER}/lib/cmake/Qt5`.
3. Generate the project.
4. Open `C:\W\RR\RadianceSuite.sln`, select `Release` and build the project.

<b>Option 2: Command Line (Visual Studio generator)</b>

1. Start the [Command Line Prompt](http://windows.microsoft.com/en-us/windows/command-prompt-faq) (VS dev shell).
2. Configure and build the project in `C:\W\RR` by typing the following commands:

```bat
cd C:\W\
mkdir RR
cd RR
cmake -G "Visual Studio 17 2022" -A x64 -DQt5_DIR:PATH=C:/Qt/${QT_VERSION}/${COMPILER}/lib/cmake/Qt5 ..\R
cmake --build . --config Release -- /maxcpucount:4
```

## Package

Install [NSIS 3.x](https://nsis.sourceforge.io/Download)

<b>Option 1: CMake and Visual Studio</b>

1. In the `C:\W\RR\Slicer-build` directory, open `Slicer.sln` and build the `PACKAGE` target

<b>Option 2: Command Line</b>

1. Start the [Command Line Prompt](http://windows.microsoft.com/en-us/windows/command-prompt-faq)
2. Build the `PACKAGE` target by typing the following commands:

```bat
cd C:\W\RR\Slicer-build
cmake --build . --config Release --target PACKAGE
```
