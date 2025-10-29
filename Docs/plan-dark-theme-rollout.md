# 暗色主题默认化与配色一致性计划（Dark Theme Rollout）

> 目标：将应用默认主题设为 Dark，并统一各模块 UI 配色以与主题一致；保留用户在 Appearance 中的手动切换与持久化。

## 今日提交速览（用于考虑变更基线）
- 47c6791 build(tools): 统一追加 CMake 生成器参数（响应文件/路径缩短/PDB）以避免命令行过长
- 638f484 build(presets): 使用 ProgramDatabase 调试信息以缓解路径长度压力
- f51a63c feat(home): 默认显示 Slicer 界面（菜单栏可见）便于访问 Appearance
- 53bd61d fix(style): 暗色主题下尊重系统调色板，不再强制覆写浅色配色
- 535f3ad feat(app): 显示并恢复顶部菜单（Edit/View/Layout/Toolbars）且启用 Appearance 以支持主题切换

上述提交已基本解决：菜单栏可见、Appearance 可用、长命令行构建问题。接下来聚焦“默认暗色 + 全局配色一致性”。

## 范围与不在范围
- 在范围：默认 Dark、调色板/样式统一、脚本模块 QSS 适配、用户可切换与持久化、打包后首次运行体验。
- 不在范围：大规模视觉重设计、图标资产系统性重绘（如需会单独立项）。

## 实施方案

### 1) 默认暗色主题（不覆盖用户选择）
- 策略：
  - 若用户尚无主题偏好（无相关 QSettings 项），首次启动应用时设置默认为 Dark；
  - 若已有用户偏好，尊重其选择，不强制改写。
- 技术落点（优先顺序）：
  1. 在应用初始化阶段设置默认值：
     - 位置候选：`Applications/RadianceApp/Main.cxx` 或 `Applications/RadianceApp/qRadianceAppMainWindow.cxx` 的启动路径中，在加载 UI 之前检查设置并应用 Dark 调色板。
     - 复用现有样式管理：`Applications/RadianceApp/Widgets/qAppStyle.cxx`（已存在亮度判断/调色板逻辑），补充“无用户偏好 => 默认 Dark”的分支。
  2. 与 Appearance 设置打通：
     - 复用 Slicer/Appearance 使用的 QSettings 键（计划内需通过代码检索确认具体键名与值，例如 Theme=Dark/Light 或 UseDarkTheme=true/false），从而保证 UI 中切换与默认值一致。
  3. 增加一个临时启动参数（可选）：`--dark`/`--light` 用于开发验证，不作为最终用户入口。

### 2) 模块 UI 配色一致性（以 Palette 为主，避免硬编码颜色）
- 现状：`Modules/Scripted/Home/Resources/Home.qss` 等可能包含硬编码浅色；暗色下存在对比度/可读性问题。
- 原则：
  - 尽量使用 QPalette 角色，减少绝对颜色值；
  - 尽量避免全局硬覆盖；仅在需要品牌强调时引入有限“语义色”（并提供暗/亮两套或基于调色板推导）。
- 具体工作：
  1. 审计与整理 QSS：
     - 路径：`Modules/Scripted/Home/Resources/Home.qss`、`Modules/Scripted/Home/Resources/UI/Home.ui`；
     - 搜索硬编码颜色（如 `#fff`, `#000`, `rgb(...)` 等），梳理替换方案。
  2. 用 Palette 角色替换：
     - 文本：`color: palette(windowText)` / `palette(text)`；
     - 背景：`background-color: palette(window)` / `palette(base)`；
     - 强调：`color/background: palette(highlight)` 与 `palette(highlightedText)`；
     - 交互：按钮用 `palette(button)` / `palette(buttonText)`。
  3. 品牌强调色（可选）：
     - 若需保留品牌主色，定义“暗/亮两套值”，由 `qAppStyle` 在切换主题时设置全局样式表占位方案（Qt 无 CSS 变量，可通过全局样式表注入选择器或运行时拼接样式实现）。
  4. 图标可见性检查（可选）：
     - PNG 图标在暗色下可能过亮/过灰；先做抽样检查，必要时为少量关键图标提供暗/亮版本。

### 3) Appearance 模块体验
- 菜单入口已恢复：确保 `Edit > Application Settings > Appearance` 可达；

### 4) 构建与验证
- 增量开发构建：
  - `pwsh Tools/Invoke-RadianceBuild.ps1 -Preset win-ninja-dev -UseSharedSlicer -Jobs 0`
  - 首次或切换分支后建议：加 `-ForceConfigure`
- 运行：
  - `../RS-build/win-ninja-dev/Slicer-build/<AppName>.exe`
- 验证要点：
  - 清理旧设置后首次启动默认为 Dark（记录截图）；
  - 在 Appearance 中切换 Light，再次重启仍保持 Light；
  - Home 模块视图文本/背景/高亮对比度满足可读性。

### 5) 兼容性与已知问题处理
- Windows 长命令行/路径：已由今日提交增强（响应文件 + PDB + 驱动器映射）；若再现，优先用 Preset 构建，并保持构建目录在 `../RS-build` 层级。
- 杀软干扰（例如火绒）：为 `../RS-build` 与 `CMake/Ninja` 缓存输出目录添加信任/白名单，避免 `.rsp`/`.pdb` 文件被锁定导致 D8022。
- Git “dubious ownership”：按提示将外部依赖的工作目录加入 safe.directory；示例：
  - `git config --global --add safe.directory C:/RS-build/win-ninja-dev/ITK`
  - `git config --global --add safe.directory C:/RS-build/win-ninja-dev/teem`

## 里程碑与交付
- M1（0.5 天）：样式与设置键梳理（qAppStyle/Appearance/QSettings），形成最终变更点清单与回滚点。
- M2（1 天）：默认 Dark 实现（仅“无偏好时”生效）+ 全局样式接入；
- M3（1 天）：Home.qss/UI 改造，抽样验证其他脚本模块（若有）；
- M4（0.5 天）：QA 验证与可访问性检查（对比度、禁用态、hover/pressed）；
- M5（0.5 天）：文档与打包验证（首次运行体验）。

## 验收标准（Definition of Done）
- 全新用户首次运行默认 Dark；手动切换 Light 后重启仍保持；
- Home 模块无“浅色残留”硬编码影响暗色可读性；
- 菜单/对话框/按钮/输入框颜色与调色板一致，无“黑底黑字/白底白字”；
- 不影响外观切换性能和稳定性；
- 打包版本首次运行行为与开发版一致。

## 风险与回滚
- 风险：第三方模块或上游窗口部件自带样式与全局调色板冲突；
- 缓解：优先 Palette 角色，不直接强行覆盖；对有冲突的局部做最小范围选择器修正；
- 回滚：默认 Dark 改动限于启动初始化与样式桥接，保留单提交回退的能力；Appearance 模块仍可用作人工切换兜底。

---
备注：本阶段仅提交计划，不修改代码。后续按里程碑逐步提交变更（Conventional Commits），并附运行截图与构建 preset 信息。
