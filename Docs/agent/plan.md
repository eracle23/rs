下面是一份**“只改 UI、不动底层逻辑/算法，且最大程度去 Slicer 痕迹”**的完整技术方案与实施计划（基于 **Slicer Custom Application Template / SlicerCAT**）。方案坚持**不修改上游 Slicer 核心源码**、所有改造集中在“自定义外壳 + 扩展模块 + 资源/样式”层，这样既能深度换脸，又能平滑跟上游版本。关键做法均来自官方文档与核心开发者公开指引。([kitware.com][1])

---

## 目标与原则

**目标**

1. 视觉与信息架构“焕然一新”，普通用户**不易辨认**它基于 Slicer；
2. **零侵入**：不改 MRML/VTK/ITK/CTK 等核心与内置算法；
3. 通过 CMake 变量、外壳主窗口子类、QSS 样式、脚本/扩展模块完成改造；
4. 保持与上游版本的**易同步性**（仅需更新外壳依赖的 Slicer `GIT_TAG`）。([3D Slicer Community][2])

**非目标**

* 不在本阶段重写算法/管线；
* 不做数据库/云协作等后端功能；
* 不做大规模 QML 全量替换（仅在“叠加面板/HUD”层使用，必要时）。

---

## 总体技术路线（三层）

1. **外壳层（SlicerCAT）**

   * 通过模板生成自定义应用，**改应用名/图标/启动图/版权页**；在外壳的 `MainWindow` 子类里**隐藏/重排**标准菜单与工具栏。([kitware.com][1])
   * 用 CMake 变量控制**默认首页模块**、**收藏模块**、**禁用内置模块清单**等，不动 Slicer 源码。([3D Slicer][3])
   * 全局 **QSS** 主题与 `qAppStyle` 自定义控件风格。([kitware.com][4])

2. **UI/布局层**

   * 用 **qSlicerLayoutManager** 注册**自定义布局 XML**，重塑 2D/3D 视图编排；并往布局菜单/工具栏**注入自定义入口**。([slicer.readthedocs.io][5])
   * 适度使用 **QQuickWidget（可选）** 做**HUD/叠加面板**（状态提示、操作导览），不改变 VTK 视图渲染本体。

3. **流程外观层（脚本/扩展）**

   * 以 **Guidelet/脚本模块** 封装关键工作流的“新界面”，内部仍调用原有 MRML/CLI/Loadable 模块，达到“看起来不同、逻辑不变”。([slicerigt.org][6])

---

## 里程碑与可交付物

> 注：以下各里程碑有明确输入/产出与验收要点；如需，我们可把任务拆到“文件与类名级别”待办清单。

### M0｜基线与仓库脚手架

**任务**

* 用 cookiecutter 生成 SlicerCAT 项目骨架；在外壳 `CMakeLists.txt` 固定上游 Slicer `GIT_REPOSITORY` 与 `GIT_TAG`（可指向稳定版 tag/commit）。
  **验收**
* 可本地编译运行的“素壳”应用；CI 初步可编。([kitware.com][1])

---

### M1｜品牌与显性“去 Slicer 化”

**任务**

* **命名与品牌**：`SlicerApp_APPLICATION_NAME`、`Slicer_DISCLAIMER_AT_STARTUP`、`Slicer_DEFAULT_HOME_MODULE`、`Slicer_DEFAULT_FAVORITE_MODULES`；替换应用图标、启动图、About 页面资源。
* **隐藏识别性 UI**：在 `q<YourApp>AppMainWindow.cxx` 里**关闭或重命名**主要菜单/工具栏（例如 Module Selector、View/MouseMode/Capture 等）。官方示例演示了逐项 `setVisible(false)` 的做法。([3D Slicer][3])
* **禁用内置模块**：用 `Slicer_QTLOADABLEMODULES_DISABLED` / `Slicer_QTSCRIPTEDMODULES_DISABLED` 精简模块列表。([3D Slicer][3])
  **验收**
* 窗口标题、关于框、启动页、菜单与工具栏**不出现 “Slicer” 文案/图标**；功能仍可用。

---

### M2｜信息架构与导航重构

**任务**

* **Home/工作台模块**：自定义“首页”模块（脚本或 Loadable），提供场景入口、最近项目、教程/流程按钮。
* **侧边导航**：弱化/隐藏模块选择下拉，改为侧栏图标导航或任务卡片。
* **布局入口**：在布局下拉菜单/工具栏**注入按钮**切换到自定义布局。脚本库示例展示了如何向布局选择菜单/工具栏**添加入口动作**。([slicer.readthedocs.io][7])
  **验收**
* 打开应用即进自定义 Home；顶部信息架构符合新规范；常用路径 ≤2 次点击可达。

---

### M3｜主题与控件外观（QSS + Style）

**任务**

* 建立 **全局 QSS**（色板、间距密度、圆角、字体体系、控件状态），并配合外壳的 `qAppStyle` 做细节覆盖。
* 使用 **StyleTester**（SandBox 扩展）**快速试验**与比对样式效果，沉淀主题变量表与组件对照表。([3D Slicer Community][8])
* **VTK 视图外观**通过 HUD/叠加控件与主题化标识（轴标、标尺）实现统一观感。
  **验收**
* 暗/亮两个主题可切换；常用控件（按钮、输入、面板、树、表格、Tab）视觉统一且不露“Slicer 风格”。([kitware.com][4])

---

### M4（可选）｜工作流外观模块（Guidelet）

**任务**

* 为 1–2 个关键使用场景开发 **Guidelet 风格脚本模块**：把多面板操作封装为**向导式/卡片式** UI；内部调用 MRML/CLI/现有算法。
* 保持**参数节点**与数据流与原模块一致，确保逻辑零改动。([slicerigt.org][6])
  **验收**
* 用户从 Home 进入“任务卡片”→ 一屏完成主流程；新老流程结果一致。

---


### M5｜扩展与打包（发布就绪）

**任务**

* **内置扩展打包**：在外壳顶层 `CMakeLists.txt` 通过 `Slicer_EXTENSION_SOURCE_DIRS` 与 `FetchContent` **捆绑扩展**（如需要）。([kitware.com][1])
* **安装包/签名**：

  * Windows：按官方指引集成 **SignTool** 对可执行文件与安装器签名。([3D Slicer][10])
  * macOS：准备 Developer ID 证书，完成 **代码签名**与**公证**（Xcode / `notarytool` 流程）。([3D Slicer][11])
* **许可与致谢**：在“关于/许可”页呈现 **Slicer BSD 风格许可**与第三方许可，满足再分发合规。([slicer.readthedocs.io][12])
  **验收**
* 三平台打包可运行；已签名/（macOS）已公证；许可页完整。

---

### M7｜CI / 回归与“不可识别性”验收

**任务**

* GitHub Actions/GitLab CI：三平台构建矩阵 + 缓存依赖；夜构/触发构建；产出安装包。
* **UI 冒烟与截图比对**：Python 脚本进行开屏/主窗/首页截图，对比预期基线（像素或阈值）；自动检查窗口标题与菜单是否仍含“Slicer”。
* **脚本冒烟**：加载示例数据、切换自定义布局、打开 Guidelet 流程，校验 MRML 节点变化。
  **验收**
* CI 生成安装包与报告；**未发现“Slicer”字样**；布局/主题/导航符合规范。

---

## 关键实现位点（示例清单）

* **CMake 变量（外壳层）**：
  `SlicerApp_APPLICATION_NAME`（应用名）、`Slicer_DEFAULT_HOME_MODULE`（默认首页）、`Slicer_DEFAULT_FAVORITE_MODULES`（收藏模块）、`Slicer_QTLOADABLEMODULES_DISABLED` / `Slicer_QTSCRIPTEDMODULES_DISABLED`（禁用模块）、`Slicer_EXTENSION_SOURCE_DIRS`（内置扩展）。([3D Slicer][3])
* **隐藏工具栏/菜单**（外壳主窗口子类）：示例中逐项 `ToolBar->setVisible(false)` 可精确控制显隐。([kitware.com][1])
* **自定义布局**：在运行时通过 `qSlicerLayoutManager` 注册自定义布局 XML，并把“切换动作”加到布局菜单/工具栏（官方脚本库有现成片段）。([slicer.readthedocs.io][5])
* **主题样式**：在外壳的 `qAppStyle` 与全局 QSS 中实现品牌化；可用 **StyleTester** 交互试验与沉淀样式表。([kitware.com][4])
* **工作流 UI 封装**：用 **Guidelet** 模板隐藏默认 GUI、暴露流程化面板，逻辑调用保持原有模块。([slicerigt.org][6])

---

## 合规与品牌化注意事项

* **许可与归属**：Slicer 采用 **BSD 风格 Slicer License**；允许商用/再分发，但需要合规呈现许可与免责声明（可放 About/许可页）。([slicer.readthedocs.io][12])
* **签名/公证**：官方建议对安装包与主要可执行进行**代码签名**；macOS 分发建议完成**公证**流程，避免 Gatekeeper 警告。([3D Slicer][10])
* **不改核心源码**：全部 UI 改动放在外壳与扩展；上游升级基本只需更新 `GIT_TAG` 并少量适配。([3D Slicer Community][2])

---

## 交付物清单（阶段性）

* **设计与规范**：导航 IA、布局地图、组件视觉规范（暗/亮）、文案与术语表。
* **代码与资源**：外壳工程、品牌资源（图标/启动图/字体）、QSS 主题、主窗口子类、Home 模块、Guidelet 模块（可选）、自定义布局 XML。
* **构建与发布**：三平台 CI、安装包、签名/公证脚本、许可与关于页。
* **测试材料**：UI 基线截图、自动化脚本、验收 checklist。

---

## 风险与缓解

* **上游 API/Qt 版本变动** → 通过“外壳层隔离”与 `GIT_TAG` 固定版本缓冲，设置**小步升级**与回归脚本。([3D Slicer Community][2])
* **QSS 对 OpenGL 内容有限** → 采用 HUD/叠加控件与布局包裹法统一视觉。
* **多语言覆盖不足** → 结合 SlicerLanguagePacks 与内部 `.ts/.qm` 管理，逐步扩大覆盖面。([3D Slicer Community][13])

---

## 下一步建议（即可着手）

1. 我用 cookiecutter 参数模板帮你**生成外壳骨架与 CMake 变量初始值**（应用名/默认模块/禁用列表）；
2. 提交首版 **QSS 基线 + 主题变量表**（暗/亮各一），并在 CI 里加 StyleTester 预览工况；([3D Slicer Community][8])
3. 交付 **自定义 Home 模块 + 2 个布局 XML + 菜单/工具栏重排**；
4. 选 1 条典型流程做 **Guidelet 外观封装**（可从 SlicerIGT 模板起步）。([slicerigt.org][6])

> 如需，我可以把各里程碑拆成**文件/类级别的待办清单**（含：要改的 CMake 变量、`q<YourApp>AppMainWindow.cxx` 位置、布局 XML 示例、QSS 目录结构与命名、Guidelet 骨架路径），方便直接按清单开工与评审。

[1]: https://www.kitware.com/slicercat-creating-custom-applications-based-on-3d-slicer/?utm_source=chatgpt.com "SlicerCAT: Creating custom applications based on 3D Slicer"
[2]: https://discourse.slicer.org/t/proper-use-of-slicerapp-application-name/7475?utm_source=chatgpt.com "Proper use of SlicerApp_APPLICATION_NAME - Development - 3D Slicer Community"
[3]: https://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Developers/Build_Instructions/Configure?utm_source=chatgpt.com "Documentation/Nightly/Developers/Build Instructions/Configure - Slicer Wiki"
[4]: https://www.kitware.com/slicercat-and-python-creating-custom-slicer-applications-with-qt-stylesheets/?utm_source=chatgpt.com "SlicerCAT and Python: Creating Custom Slicer Applications with Qt Stylesheets"
[5]: https://slicer.readthedocs.io/en/latest/developer_guide/mrml_overview.html?utm_source=chatgpt.com "MRML Overview — 3D Slicer documentation"
[6]: https://www.slicerigt.org/wp/developer-tutorial/?utm_source=chatgpt.com "Developer tutorial | SlicerIGT"
[7]: https://slicer.readthedocs.io/en/5.4/developer_guide/script_repository.html?utm_source=chatgpt.com "Script repository — 3D Slicer documentation"
[8]: https://discourse.slicer.org/t/new-developer-feature-styletester/15509?utm_source=chatgpt.com "New Developer Feature - StyleTester - Development - 3D Slicer Community"
[9]: https://discourse.slicer.org/t/slicer-internationalization/579?utm_source=chatgpt.com "Slicer Internationalization - Development - 3D Slicer Community"
[10]: https://www.slicer.org/wiki/Documentation/Nightly/Developers/Windows_Code_Signing?utm_source=chatgpt.com "Documentation/Nightly/Developers/Windows Code Signing - Slicer Wiki"
[11]: https://www.slicer.org/wiki/Documentation/Nightly/Developers/Mac_OS_X_Code_Signing?utm_source=chatgpt.com "Documentation/Nightly/Developers/Mac OS X Code Signing - Slicer Wiki"
[12]: https://slicer.readthedocs.io/en/latest/user_guide/about.html?utm_source=chatgpt.com "About 3D Slicer — 3D Slicer documentation"
[13]: https://discourse.slicer.org/t/slicerlanguagepacks-new-extension-for-translating-user-interface-of-slicer-to-various-languages/24421?utm_source=chatgpt.com "SlicerLanguagePacks: New extension for translating user interface of Slicer to various languages - Support - 3D Slicer Community"
