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

首选 VS/MSBuild 构建（更稳、更少边角问题）。首轮 SuperBuild 可能耗时较长（Slicer + 依赖）。

<b>Option 0（推荐）：Visual Studio 2022 + CMake Presets</b>

Prereqs: VS 2022（C++ 工作负载/Build Tools + Windows SDK）、CMake ≥ 3.27、Git、NSIS（打包）、Qt 5.15.2 msvc2019_64。

- 一次性设置 Qt 路径（可选）：`pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
- 配置与构建（多配置生成器，使用 RelWithDebInfo）：

```pwsh
# 配置（生成到 C:/S/vs-dev）
cmake --preset vs17-dev

# 构建
cmake --build --preset vs17-dev-rel -- /m

# 安装（聚合到 C:/S/rs-install）
cmake --build --preset vs17-dev-rel --target install -- /m

# 打包（NSIS 安装器）
cmake --build C:/S/vs-dev/Slicer-build --config RelWithDebInfo --target PACKAGE -- /m
```

脚本等价用法：

```pwsh
pwsh Tools/Invoke-RadianceBuild.ps1 -Preset vs17-dev -Jobs 0
pwsh Tools/Dev-Build-Ext.ps1 -Action build -Preset vs17-dev -Jobs 0
```

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

<b>Option 3（可选，CI/加速用）：Ninja + CMake Presets + sccache</b>

仅在需要时使用（例如 CI）。日常开发不推荐 Ninja。参见 `CMakePresets.json` 中 `win-ninja-*` 预设，或脚本 `Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev`。

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
