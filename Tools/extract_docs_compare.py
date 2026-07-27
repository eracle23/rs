# -*- coding: utf-8 -*-
"""Extract docx paragraphs for document comparison."""
from __future__ import annotations

import re
import zipfile
from pathlib import Path


def extract_docx(path: Path) -> list[str]:
    with zipfile.ZipFile(path) as z:
        xml = z.read("word/document.xml").decode("utf-8")
    paras: list[str] = []
    for para in xml.split("</w:p>"):
        t = "".join(re.findall(r"<w:t[^>]*>([^<]*)</w:t>", para))
        if t.strip():
            paras.append(t.strip())
    return paras


def main() -> None:
    root = Path(r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料")
    # Prefer newest filenames; skip Word lock files
    names = [
        "医学影像三维重建软件产品技术要求--0616.docx",
        "医学影像三维重建软件产品技术要求0615修.docx",
        "医学影像三维重建软件说明书.docx",
        "软件需求规格说明书0528.docx",
        "软件概要设计说明书.docx",
        "软件详细设计说明书.docx",
    ]
    out_dir = Path(r"E:\GitHub\rs0\rs\Tools\_doc_compare_latest")
    out_dir.mkdir(exist_ok=True)

    for name in names:
        matches = list(root.rglob(name))
        if not matches:
            print("MISSING", name)
            continue
        p = max(matches, key=lambda x: x.stat().st_mtime)
        safe = re.sub(r"[^\w\u4e00-\u9fff.-]+", "_", p.stem)[:80]
        paras = extract_docx(p)
        out = out_dir / f"{safe}.txt"
        out.write_text("\n".join(f"[{i}] {t}" for i, t in enumerate(paras)), encoding="utf-8")
        print(f"OK {p.name} ({len(paras)} paras, {p.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
