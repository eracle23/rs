# -*- coding: utf-8 -*-
"""Patch section 4.1 text in the user manual docx (no image changes)."""

from docx import Document

DOCX_PATH = r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料\医学影像三维重建软件说明书.docx"
OUT_PATH = r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料\医学影像三维重建软件说明书-4.1修订.docx"

REPLACEMENTS = {
    258: (
        "步骤 1：DCM 导入本地 DICOM 目录，在 DICOM 数据库中选择序列并加载。",
        "步骤 1：导入 CT/MRI 体数据。推荐将本地 DICOM 文件夹或 .dcm 文件拖入主窗口；"
        "也可点击工具栏 DATA（添加数据）按钮。系统打开「添加数据到场景中」对话框。"
        "（亦可通过 DICOM 模块导入到数据库后选择序列加载，见 3.4 节。）",
    ),
    261: (
        "图4-1点击添加数据按钮",
        "图4-1 点击 DATA（添加数据）按钮，或将 DICOM 文件夹拖入主窗口",
    ),
    263: (
        "图4-2",
        "图4-2 打开「添加数据到场景中」对话框",
    ),
    265: (
        "图4-3选中文件",
        "图4-3 点击「选择要添加的目录」或「选择要添加的文件」，选中 CT 所在的 DICOM 文件夹或文件",
    ),
    267: (
        "图4-4点击ok按钮",
        "图4-4 在列表中勾选待导入项，确认「描述」列为 DICOM import",
    ),
    270: (
        "图4-4 选择序列进行加载",
        "图4-4 点击「确定」，将 DICOM 数据加载到当前场景",
    ),
    273: (
        "图4-5 进入体数据界面",
        "图4-5 加载完成后，中央视窗显示体数据，可切换到体数据模块",
    ),
    292: (
        "这里的阈值设置，建议右端保持最大，调整左侧来达到自己想要的效果，比如我这里是尽量让骨头模型高亮。",
        "阈值设置时，建议保持阈值上限为最大值，通过调整下限使目标组织（如骨组织）在视图中充分显示。",
    ),
    299: (
        "使用最基本的三维重建可以不用逻辑运算。",
        "进行基础三维重建时，可跳过逻辑运算步骤，直接显示 3D 结果。",
    ),
}


def main() -> None:
    doc = Document(DOCX_PATH)
    ok = 0
    for idx, (old, new) in REPLACEMENTS.items():
        p = doc.paragraphs[idx]
        current = p.text.strip()
        if current != old:
            print(f"WARN para {idx}: expected {old!r}, got {current!r}")
            continue
        p.text = new
        ok += 1
        print(f"OK para {idx}")
    try:
        doc.save(DOCX_PATH)
        out = DOCX_PATH
    except PermissionError:
        doc.save(OUT_PATH)
        out = OUT_PATH
        print("NOTE: original file is locked (close Word?), saved copy instead.")
    print(f"saved {out} ({ok}/{len(REPLACEMENTS)} paragraphs updated)")


if __name__ == "__main__":
    main()
