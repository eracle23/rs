# RadianceApp 单元测试用例文档（草案）

> 路径：`Applications/RadianceApp`  
> 目的：先给出一个**可落地、低耦合、可自动化**的单元测试用例，便于后续逐步扩展测试集。

---

## 1. 测试目标

针对 `Widgets/VolumeImportValidator` 的核心行为设计单元测试，验证：

- 当输入体数据（Volume）元信息满足规则时，校验通过；
- 当关键元信息缺失或非法时，校验失败并给出预期错误信息（或错误码）。

选择该模块的原因：

1. 逻辑相对独立，适合单测；
2. 直接影响 DICOM/体数据导入质量；
3. 失败场景明确，断言边界清晰。

---

## 2. 被测对象

- 主要类：`VolumeImportValidator`
- 文件：
  - `Applications/RadianceApp/Widgets/VolumeImportValidator.h`
  - `Applications/RadianceApp/Widgets/VolumeImportValidator.cxx`

---

## 3. 单元测试用例设计

## 用例 ID

`UT-RAD-VIV-001`

## 用例名称

**合法体数据输入应通过校验**

## 前置条件

1. 测试运行环境可创建最小 MRML 场景/节点（或使用 mock/stub 替代）；
2. 可构造一个包含完整基础元信息的 Volume 节点（如尺寸、spacing、像素类型等）。

## 输入数据（示例）

- 维度：`512 x 512 x 120`
- Spacing：`0.7, 0.7, 1.0`
- 标量类型：`short`（或项目认可类型）
- 图像数据非空

## 测试步骤

1. 构造/加载一个最小合法 Volume 节点；
2. 调用 `VolumeImportValidator` 对应校验接口（如 `validate(...)`）；
3. 获取返回结果（bool / 状态对象 / 错误列表）；
4. 断言结果为“通过”。

## 预期结果

- 校验结果为成功（例如 `true`）；
- 错误信息为空（若接口包含错误输出）。

## 断言建议

- `EXPECT_TRUE(result)`
- `EXPECT_TRUE(errors.empty())`（若有错误集合）

---

## 4. 补充负向用例（建议下一步增加）

为保证健壮性，建议至少补齐以下两个负向用例：

1. `UT-RAD-VIV-002`：图像数据为空 -> 校验失败；
2. `UT-RAD-VIV-003`：Spacing 包含 0 或负数 -> 校验失败。

这样可快速形成“1正2负”的最小测试闭环。

---

## 5. 测试实现建议（框架层面）

- C++：优先使用项目现有 CTest 流程，测试代码放入 `Testing/Cxx`（如后续补目录）；
- 若当前模块尚无测试目标，可先新增一个轻量 test target，只链接必要依赖，避免把 UI/主程序全量拉入。

---

## 6. 通过/失败判定标准

- **通过**：该用例在本地与 CI 环境稳定通过（连续运行 >= 10 次无随机失败）；
- **失败**：任一关键断言不满足，或测试依赖外部状态（路径/权限/用户环境）导致不稳定。

---

## 7. 风险与注意事项

1. 若 `VolumeImportValidator` 与全局单例/场景强耦合，需要先做接口解耦或引入 stub；
2. 单测应避免依赖真实 DICOM 大文件，尽量使用最小构造数据；
3. 不在单元测试中验证 UI 弹窗行为（那属于集成/UI 自动化测试范围）。

---

## 8. 后续扩展路线

建议按优先级推进：

1. `VolumeImportValidator`（数据入口质量）
2. `UserManager`（认证与用户信息边界）
3. `LicenseManager`（授权状态判定）
4. `RadianceShellCleaner`（系统交互类逻辑，需 mock）

---

如果你确认这份文档方向没问题，我下一步可以直接给出：

1. 对应的 `C++/CTest` 测试文件骨架；
2. 一个可编译运行的 `UT-RAD-VIV-001` 示例实现；
3. 测试目标在 `CMakeLists.txt` 的最小增量改法。
