# -*- coding: utf-8 -*-
from pathlib import Path
import re

root = Path(r"E:\GitHub\rs0\rs\Tools\_doc_compare")
out = root / "_gap_analysis.txt"

docs = {
    "tech_req": root / "医学影像三维重建软件产品技术要求0615修.txt",
    "manual": root / "医学影像三维重建软件说明书.txt",
    "srs": root / "软件需求规格说明书0528.txt",
    "hld": root / "软件概要设计说明书.txt",
    "lld": root / "软件详细设计说明书.txt",
}

texts = {k: p.read_text(encoding="utf-8") for k, p in docs.items()}

keywords = [
    "加密狗", "PACS", "WADO", "工作列表", "NIfTI", "NRRD", "信息学",
    "配准", "Dice", "AI 分割", "增强", "去噪", "TXT", "服务器", "宕机",
    "VolumeImportValidator", "±1", "15分钟", "110MB", "15S", "10s",
    "模块白名单", "医生批注", "2.5 m", "2500", "-1050",
    "STL", "OBJ", "PLY", "Windows 11", "Windows 10",
    "单实例", "许可证", "License",
]

lines = ["# Keyword presence matrix", ""]
header = "| Keyword | TechReq | Manual | SRS | HLD | LLD |"
sep = "|---|---|---|---|---|---|"
lines += [header, sep]
for kw in keywords:
    row = [kw]
    for k in docs:
        row.append("Y" if kw in texts[k] else "-")
    lines.append("| " + " | ".join(row) + " |")

lines += ["", "# Headings in SRS", ""]
for m in re.finditer(r"^\[\d+\] (FR-[A-Z0-9-]+|DR-[A-Z0-9-]+|PR-[A-Z0-9-]+|SR-[A-Z0-9-]+|RR-[A-Z0-9-]+|[0-9]+(?:\.[0-9]+)* [^\n]+)", texts["srs"], re.M):
    if len(m.group(1)) < 80:
        lines.append(m.group(1))

lines += ["", "# Headings in HLD (sample)", ""]
for line in texts["hld"].splitlines():
    if re.match(r"^\[\d+\] [0-9]", line) or re.match(r"^\[\d+\] [3-9]\.", line):
        lines.append(line[:120])

lines += ["", "# Headings in LLD (sample)", ""]
for line in texts["lld"].splitlines()[:200]:
    if re.search(r"设计|模块|登录|DICOM|分割|批注|校验|加密", line):
        lines.append(line[:140])

# Extract FR blocks from SRS
lines += ["", "# SRS FR titles", ""]
for m in re.finditer(r"\[(\d+)\] (FR-[A-Z0-9-]+)\n\[(\d+)\] ([^\n]+)", texts["srs"]):
    lines.append(f"{m.group(2)}: {m.group(4)}")

# Manual chapter titles
lines += ["", "# Manual chapters", ""]
for line in texts["manual"].splitlines():
    t = re.sub(r"^\[\d+\] ", "", line)
    if re.match(r"^[0-9]+[\.、]", t) or re.match(r"^第[一二三四五六七八九十]+章", t):
        lines.append(t[:100])

# Tech req unique constraints
lines += ["", "# Tech req constraints snippets", ""]
for line in texts["tech_req"].splitlines():
    if any(x in line for x in ["2.2.2", "2.7", "2.13", "影像导入", "性能", "并发", "版权"]):
        lines.append(line[:160])

out.write_text("\n".join(lines), encoding="utf-8")
print("written", out)
