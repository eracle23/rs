# -*- coding: utf-8 -*-
from pathlib import Path
from docx import Document

p = Path(r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料\软件资料需求2026.06.23\CH3.5.5.11-网络安全研究报告.docx")
doc = Document(str(p))
lines = []
for i, para in enumerate(doc.paragraphs):
    t = para.text.strip()
    if t and ("图" in t or "数据" in t or "架构" in t or "通讯" in t):
        lines.append(f"P{i}: {t}")
lines.append("\n--- images in doc ---")
from docx.document import Document as Doc
from docx.oxml.ns import qn
for rel in doc.part.rels.values():
    if "image" in rel.reltype:
        lines.append(rel.target_ref)
Path(r"E:\GitHub\rs0\rs\Tools\_research_fig_dump.txt").write_text("\n".join(lines), encoding="utf-8")
print("done", len(lines))
