# 计划：主题与构建稳定性推进（Alice）

> 目标：开启 Appearance 模块、默认暗色主题、移除自定义 Home 样式以完全跟随应用主题；并收敛 Windows/Shared Slicer 构建中的不稳定点（Teem、Python、HDF5、长路径等）。本计划不修改代码，仅作为执行手册与排障清单。

## 范围
- UI/主题
  - 默认暗色主题，提供在 Appearance 模块里切换。
  - 取消自定义 Home 样式（QSS），Welcome/Appearance 等随系统主题 100% 同步。
  - 设置页、外观切换里的品牌从 “Slicer” 统一为 “Alice”。
  - 3D 预览区标题栏颜色与主题一致，避免硬编码颜色。
- 构建/运行
  - 以共享 Slicer 方式（UseSharedSlicer）开发调试。
  - 修复/绕过：长命令行、长路径、zlib/vcruntime、RC/MT 缺失、LNK1168 文件被占用、Python3.lib 别名、Teem QNaN 探测等问题。

## 成品交付
- 首启默认暗色：`Resources/Settings/DefaultSettings.ini` 仅提供默认值，用户切换后可持久化。
- Appearance 模块可见并可切换 Light/Dark（品牌文案替换为 Alice）。
- 移除或停用自定义 Home 样式；默认 Home 模块为 Welcome；Home 不再影响主题。
- 构建排障脚本/命令清单（下文）。

---

## 执行步骤

### 1. 默认暗色 + Appearance
- 确认 DefaultSettings.ini 包含：
  - `[Modules] HomeModule=Welcome`
  - `[Styles] Style=Dark Slicer`（先使用 Slicer 内置暗色，品牌文案后面替换为 Alice）。
- 确保未在代码中强行覆盖用户样式选择（保持默认值即可，用户切换后写入用户设置）。
- 确认未隐藏 Appearance 模块（若有 UI 裁剪代码，撤销隐藏）。

### 2. 去自定义样式（零 QSS）
- 移除 Home.qss 及其引用；不要为 Welcome/Home/Settings 等模块附加任何 QSS。
- 对标题栏、工具栏颜色不做硬编码；全部依赖 Qt/应用调色板。

### 3. 品牌替换为 Alice（计划内变更点）
- 外观切换及设置页面的字符串中，将 “Slicer” 文案替换为 “Alice”。两种实现策略：
  - A. 轻量：仅替换展示文案（例如主题名称显示成 “Dark Alice”），不改内部主题键名。
  - B. 深度：扩展 AppStyle/Appearance 模块以支持品牌化主题名（保持内部兼容）。
- 本阶段建议先采用 A，验证稳定后再考虑 B。

### 4. 3D 预览区标题栏颜色
- 不使用固定颜色。若必须微调，优先基于调色板角色（如 Window, Base, Button, Highlight）或最少量 QSS，且仅针对 Dock 标题使用相对色，而非 HEX 常量。

### 5. 运行目录同步/清理
- 运行树位于：`../RS-build/win-ninja-dev/Slicer-build`（或 `R:\lib\Alice-5.9\qt-scripted-modules` 等）。
- 清理历史 Home 残留（否则会被扫描加载并覆盖 Welcome）
  - 删除：`R:\lib\Alice-5.9\qt-scripted-modules\Home.py`
  - 删除：`R:\lib\Alice-5.9\qt-scripted-modules\Resources\Home.qss`
  - 删除：`R:\lib\Alice-5.9\qt-scripted-modules\Resources\UI\Home.ui`
- 如果想指定性地同步文件，可用 Ninja 拷贝目标（不推荐长期依赖）：
  - `ninja lib/Alice-5.9/qt-scripted-modules/Home.py lib/Alice-5.9/qt-scripted-modules/Resources/Home.qss lib/Alice-5.9/qt-scripted-modules/Resources/UI/Home.ui`

---

## 构建与运行指引（Windows）

- 一次性准备：
  - 安装 VS 2022 Build Tools（含 Windows 10 SDK 10.0.26100.x，包含 rc.exe/mt.exe）。
  - 管理员 PowerShell 启动：
    - `pwsh -ExecutionPolicy Bypass Tools/Bootstrap-Prereqs.ps1 -AutoElevate -InstallChocolatey`
    - `pwsh Tools/Setup-BuildEnv.ps1 -QtCMakeDir C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5`
- 快速开发构建：
  - `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -Jobs 0`
- 运行：
  - `../RS-build/win-ninja-dev/Slicer-build/<AppName>.exe`
- 打包发布：
  - `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-rel -UseSharedSlicer -Package`

---

## 常见构建问题与修复

### A. vcruntime.h / C 编译器 broken / RC/MT 缺失
- 现象：`fatal error C1083: Cannot open include file: 'vcruntime.h'`、`CMAKE_MT-NOTFOUND`、`RC Pass failed`。
- 原因：未导入 VS 开发环境或未安装 SDK。
- 修复：
  - 确保安装 VS 2022 BuildTools + Windows 10 SDK 10.0.26100.x。
  - 始终通过工具脚本导入环境：`pwsh Tools/Setup-BuildEnv.ps1` 或在 VS x64 Native Tools Prompt 运行。

### B. LNK1168：无法写入正在被占用的 DLL
- 现象：`LINK : fatal error LNK1168: cannot open *.dll for writing`（文件被占用）。
- 处理：
  - 退出正在运行的 `<AppName>.exe`，结束残留进程（必要时重启）。
  - 将构建目录与 `cmake.exe`、`ninja.exe` 加入杀软白名单。

### C. 参数过长 / CreateProcess: parameter is incorrect
- 现象：Ninja 报错“is the command line too long?”。
- 处理：在顶层 CMake 传递：
  - `-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON`
  - `-DCMAKE_OBJECT_PATH_MAX=100`
  - `-DMSVC_DEBUG_INFORMATION_FORMAT=ProgramDatabase`

### D. Python3.lib 丢失（LNK1104）
- 现象：`fatal error LNK1104: cannot open file 'python3.lib'`；仅存在 `python312.lib`。
- 修复思路：生成 `python3.lib` 别名到被链接搜索的目录。
- 推荐步骤：
  1) 先构建 Python：`cmake --build C:/W/Slicer-build -j 0 --target python`
  2) 用 PowerShell 生成别名（两处目录）：
     - `powershell -NoProfile -Command "
       $src='C:/W/Slicer-build/python-build/libs/python312.lib';
       $d1='C:/W/Slicer-build/python-build/libs/python3.lib';
       $d2='C:/W/Slicer-build/python-build/CMakeBuild/libpython/Release/python3.lib';
       if (Test-Path $src) { New-Item -ItemType Directory -Force -Path (Split-Path $d2) | Out-Null; Copy-Item $src $d1 -Force; Copy-Item $src $d2 -Force } else { Write-Error 'python312.lib not found' }
       "`
  3) 重新构建失败目标或整体构建。
- 可选：提供 CMake 脚本（在顶层以 `-P` 执行）
  - 见附录《EnsurePython3LibAlias.cmake 示例》。

### E. Teem 配置失败（QNaN 探测 / cache 解析错误）
- 现象 1：`Failed to compile a test ... TestQnanhibit.c`（QNaN 测试失败）。
- 现象 2：`Parse error ... teem-cache-*.cmake`（cache 文件被手工追加破坏）。
- 处理：
  1) 清理并重新生成 Teem 初始 cache：
     - 删除：`C:/W/Slicer-build/teem-prefix/tmp/teem-cache-Debug.cmake`、`.../teem-cache-RelWithDebInfo.cmake`（如存在）。
     - 触发一次顶层配置生成新的 cache：`cmake --build C:/W/Slicer-build -j 0 --target VTK`（或任意会先执行 configure 的目标）。
  2) 仅在干净的 cache 末尾用 PowerShell 可靠追加两行（自动加换行）：
     - `powershell -NoProfile -Command "
       $f='C:/W/Slicer-build/teem-prefix/tmp/teem-cache-Debug.cmake';
       Add-Content -LiteralPath $f -Value ([Environment]::NewLine);
       Add-Content -LiteralPath $f -Value 'set(TEEM_QNANHIBIT "22" CACHE STRING "Initial cache" FORCE)';
       Add-Content -LiteralPath $f -Value 'set(AIR_QNANHIBIT "22" CACHE STRING "Initial cache" FORCE)'
       "`
     - 若有 RelWithDebInfo：把 `$f` 改为对应路径再执行一遍。
  3) 仅重建 Teem：`cmake --build C:/W/Slicer-build -j 1 --target teem`
- 备注：勿用 `cmd.exe` 的 `echo ... >>` 追加（容易破坏括号/换行）。

### F. HDF5/VTK 探测在 Windows 的兼容项
- 若 HDF5 的 try_run/try_compile 使用 POSIX API（popen/pclose）导致链接失败：
  - 传递：`-DCMAKE_C_FLAGS=/D_CRT_DECLARE_NONSTDC_NAMES=1 -DCMAKE_CXX_FLAGS=/D_CRT_DECLARE_NONSTDC_NAMES=1`
  - 或禁用双精度长浮点：`-DHDF5_ENABLE_LDOUBLE=OFF`
  - 或禁用 VTK HDF5 IO：`-DVTK_MODULE_ENABLE_VTK_IOHDF5=NO`

### G. 其他提示
- `-Wno-dev` 可抑制 CMake 的开发者级告警（如 CMP0177）。
- 链接时长路径问题已通过 Q: 映射/OBJECT_PATH_MAX 缓解；仍建议放置到短路径（如 `C:/W`）。

---

## 一次性完整命令示例（共享 Slicer Debug 开发）

- 配置并构建（含稳定化参数）：
  - `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -ForceConfigure -Jobs 0 -ExtraCMakeArgs '-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON;-DCMAKE_OBJECT_PATH_MAX=100;-DMSVC_DEBUG_INFORMATION_FORMAT=ProgramDatabase;-DCMAKE_C_FLAGS=/D_CRT_DECLARE_NONSTDC_NAMES=1;-DCMAKE_CXX_FLAGS=/D_CRT_DECLARE_NONSTDC_NAMES=1'`
- 若出现 Teem 失败，按上节 E 处理后再执行：
  - `cmake --build C:/W/Slicer-build -j 1 --target teem`，再整体：`cmake --build C:/W/Slicer-build -j 0`

---

## 验收清单
- 首启默认暗色（Settings 未写入用户项时）：主题为暗色；Appearance 可见可切换。
- 切换 Light 后，Welcome 页面与 3D 预览区标题栏均随之变亮，无残留深色。
- 设置/外观页内“Dark Slicer”等文案替换为“Dark Alice”。
- 运行目录不含 Home.py/Home.qss/Home.ui 等旧文件。
- 构建阶段不再出现 zlib/vcruntime、Python3.lib、Teem QNaN 等阻断问题；遇到 LNK1168 可按文档快速处理。

---

## 附录 A：EnsurePython3LibAlias.cmake 示例

```cmake
# 用法：cmake -D PY_BUILD_ROOT="C:/W/Slicer-build/python-build" -P Tools/EnsurePython3LibAlias.cmake
if(NOT DEFINED PY_BUILD_ROOT)
  message(FATAL_ERROR "Set PY_BUILD_ROOT to python-build directory")
endif()
file(TO_CMAKE_PATH "${PY_BUILD_ROOT}" _ROOT)
set(_SRC "${_ROOT}/libs/python312.lib")
set(_D1  "${_ROOT}/libs/python3.lib")
set(_D2  "${_ROOT}/CMakeBuild/libpython/Release/python3.lib")
if(EXISTS "${_SRC}")
  file(MAKE_DIRECTORY "${_ROOT}/CMakeBuild/libpython/Release")
  file(COPY "${_SRC}" DESTINATION "${_ROOT}/libs" FILE_PERMISSIONS OWNER_READ OWNER_WRITE)
  file(RENAME "${_D1}" "${_D1}") # 无操作，确保存在
  file(COPY "${_SRC}" DESTINATION "${_ROOT}/CMakeBuild/libpython/Release" FILE_PERMISSIONS OWNER_READ OWNER_WRITE)
else()
  message(FATAL_ERROR "python312.lib not found under ${_ROOT}/libs")
endif()
```

---

## 附录 B：问题与上下文（汇总给外部专家）

- 环境
  - Windows，VS 2022 BuildTools 14.44.35207/35217；Windows 10 SDK 10.0.26100.0；CMake 4.1；Ninja；Chocolatey。
  - 源码：`C:/RS`；共享 Slicer 源：`C:/W/Slicer`；SuperBuild：`C:/W/Slicer-build`；App 构建：`C:/RS-build/win-ninja-dev`。
- 关键报错摘要
  - zlib：`fatal error C1083: 'vcruntime.h'`（MSVC/UCRT 环境未导入或 SDK 缺失）。
  - 链接：`LNK1168 cannot open qSlicerBaseQTCore.dll for writing`（文件被占用）。
  - Ninja：`CreateProcess: The parameter is incorrect (command line too long)`（需强制响应文件/缩短路径）。
  - Python：`LNK1104: cannot open file 'python3.lib'`（仅有 `python312.lib`）。
  - VTK/HDF5：`C maximum decimal precision ... Failed`；HDF5 try_run 用到 `popen/pclose`（Windows 需 `_popen/_pclose` 或声明非标准名）。
  - Teem：`Failed to compile a test (TestQnanhibit.c)`；以及 `Parse error`（手工用 cmd echo 追加破坏了 cache）。
  - RC：`CMAKE_MT-NOTFOUND` / `RC Pass 1 ... no such file or directory`（rc.exe/mt.exe 未发现/路径未导入）。
- 已验证手段
  - 使用 `CMAKE_NINJA_FORCE_RESPONSE_FILE=ON`、`CMAKE_OBJECT_PATH_MAX=100`、`MSVC_DEBUG_INFORMATION_FORMAT=ProgramDatabase` 缓解长命令/长路径。
  - 通过 PowerShell `Add-Content` 正确在 Teem 初始 cache 末尾追加 `TEEM_QNANHIBIT/AIR_QNANHIBIT` 两行可稳定通过。
  - 为 Python 生成 `python3.lib` 别名后，相关扩展链接通过。
- UI 问题摘要
  - Home.qss 导致 Welcome 在切换 Light 后仍保持暗色；移除 QSS/使用 Welcome 后可随主题变化。
  - Appearance/设置页中的 “Slicer” 文案需替换为 “Alice”。
  - 3D 预览区标题栏存在颜色丢失/不统一，建议用调色板角色驱动或最少量相对色 QSS。

---

## 风险与回滚
- 任意品牌/文案替换需确保不更改内部主题键名，避免影响已有配置。
- 去 QSS 后如需细节微调，务必限定影响范围（仅 Dock 标题等），并优先使用调色板相对色避免视觉倒挂。
- 构建参数统一在 Preset 或 `-ExtraCMakeArgs` 提供，避免分散配置导致不可复现。

---

## 下一步建议
- 按本计划先实现无 QSS 的主题跟随与默认暗色；
- 完成品牌文案替换（先 A 方案）；
- 汇总本计划与附录 B 交付给外部专家审阅（重点：Teem/Python/HDF5 的处理路径）。

