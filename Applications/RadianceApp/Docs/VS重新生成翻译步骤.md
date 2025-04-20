# 在 Visual Studio 中重新生成翻译（按你的路径）

适用路径：
- **源码**：`E:\GitHub\rs0\rs`
- **构建目录**：`E:\build`

---

## 一、你修改的是哪种翻译？

1. **.ts 文件**（如 `SlicerLanguageTranslations-main\translations\Slicer_zh_CN.ts`）  
   → 需要重新用 lrelease 把 .ts 编译成 .qm，然后重新编译用到翻译的工程。  
2. **补丁里的中文**（`Patches\Slicer\slicer-zh-CN-localization.patch`）  
   → 需要重新配置（让补丁重新应用），再完整编译 Slicer 内层。

下面按这两种情况分别写步骤。

---

## 二、修改了 .ts 文件时（重新生成 .qm）

### 步骤 1：用你的路径重新配置（如需）

若你改过 CMake 里翻译路径或想确保配置一致，在 **开发者 PowerShell 或 cmd** 里执行：

```bat
cd /d E:\build
cmake E:\GitHub\rs0\rs
```

（若一直用 `E:\build` 且从没改过翻译相关 CMake，可跳过，直接步骤 2。）

### 步骤 2：在 VS 里打开解决方案

1. 打开 **Visual Studio 2022**。
2. **文件 → 打开 → 项目/解决方案**。
3. 选择：**`E:\build\VisionMagicEcosystem.sln`**（顶层 SuperBuild 解决方案）。
4. 若你只编译内层 Slicer（含翻译目标），可打开：**`E:\build\Slicer-build\Slicer.sln`**。

### 步骤 3：重新生成

- **若打开的是顶层** `E:\build\VisionMagicEcosystem.sln`：  
  **生成 → 重新生成解决方案**（或先“生成解决方案”）。  
  会先构建 SuperBuild，再构建 Slicer 内层，翻译目标会在内层构建时执行。
- **若打开的是内层** `E:\build\Slicer-build\Slicer.sln`：  
  在“解决方案资源管理器”里找到 **SlicerTranslations_zh_CN** 目标，对其“重新生成”；或直接 **生成 → 重新生成解决方案**。

### 步骤 4：确认 .qm 输出

- .qm 一般会生成在 Slicer 内层构建目录下的 `translations` 或 CMake 配置的 QM 输出目录，例如：  
  `E:\build\Slicer-build\Slicer-build\...\translations` 或项目属性里写的输出路径。
- 安装后会在安装目录的 `bin\translations`（或你配置的安装路径）下看到 `.qm` 文件。

---

## 三、修改了补丁（slicer-zh-CN-localization.patch）时

补丁是在 CMake 配置阶段应用的，需要让 CMake 重新跑一遍并重新应用补丁。

### 步骤 1：删除补丁标记（让补丁重新应用）

在资源管理器中删除（若存在）：

- **`E:\build\_deps\slicersources-src\.rs_patches_applied`**（使用 FetchContent 下载时）  
  或  
- **`C:\b\slicersources-src\.rs_patches_applied`**（使用 `slicersources_SOURCE_DIR=C:/b/slicersources-src` 时）  
  或  
- 若 Slicer 源码在别的本地目录：在该 **Slicer 源码根目录** 下删除 **`.rs_patches_applied`**。

### 步骤 2：重新配置

在 **开发者命令提示** 或 **PowerShell** 中：

```bat
cd /d E:\build
cmake E:\GitHub\rs0\rs
```

（若你用了 `-Dslicersources_SOURCE_DIR=...`，需要把同一参数再带上，例如：  
`cmake -Dslicersources_SOURCE_DIR=C:/b/slicersources-src -Dslicersources_BINARY_DIR=E:/build/slicersources-subbuild E:\GitHub\rs0\rs`。）

### 步骤 3：在 VS 里重新生成

1. 打开 **`E:\build\VisionMagicEcosystem.sln`**（或先关闭再重新打开，以加载新配置）。
2. **生成 → 重新生成解决方案**。
3. 若要只编译内层：打开 **`E:\build\Slicer-build\Slicer.sln`**，再 **重新生成解决方案**。

---

## 四、快速对照（按你的路径）

| 操作           | 命令/位置 |
|----------------|-----------|
| 源码目录       | `E:\GitHub\rs0\rs` |
| 构建目录       | `E:\build` |
| 顶层解决方案   | `E:\build\VisionMagicEcosystem.sln` |
| 内层 Slicer    | `E:\build\Slicer-build\Slicer.sln` |
| .ts 文件目录   | `E:\GitHub\rs0\rs\SlicerLanguageTranslations-main\translations` |
| 补丁文件       | `E:\GitHub\rs0\rs\Patches\Slicer\slicer-zh-CN-localization.patch` |
| 重新配置命令   | `cd /d E:\build` 然后 `cmake E:\GitHub\rs0\rs` |
| 使用本地 Slicer | `cmake -Dslicersources_SOURCE_DIR=C:/b/slicersources-src -Dslicersources_BINARY_DIR=E:/build/slicersources-subbuild E:\GitHub\rs0\rs` |

---

## 五、若使用 CMake 预设（路径不同时）

若你用的是 `vs17-dev` 预设，构建目录是 **`C:/S/vs-dev`**，则：

- 打开的是 **`C:\S\vs-dev\VisionMagicEcosystem.sln`**；
- 内层为 **`C:\S\vs-dev\Slicer-build\Slicer.sln`**；
- 重新配置：`cd /d C:\S\vs-dev`，然后 `cmake --preset vs17-dev`（在源码目录 `E:\GitHub\rs0\rs` 下执行）。

其余逻辑与上面相同：改 .ts 就重新生成解决方案；改补丁就先删 `.rs_patches_applied`，再运行 cmake，再在 VS 里重新生成。
