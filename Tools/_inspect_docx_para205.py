# -*- coding: utf-8 -*-
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

p = Path(r"C:\Users\lie76\Desktop\三维重建软件文档\产品技术要求0603.docx")
target = "磁共振影像数据处理软件由系统服务和客户端组成"

with zipfile.ZipFile(p) as z:
    xml = z.read("word/document.xml")

root = ET.fromstring(xml)
ns = {"w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main"}
body = root.find("w:body", ns)
paras = body.findall("w:p", ns)

for i, para in enumerate(paras):
    texts = [t.text or "" for t in para.findall(".//w:t", ns)]
    joined = "".join(texts)
    if target in joined:
        print(f"段落索引（body 内 w:p）: {i}")
        print(f"完整段落文本:\n{joined}\n")
        txbx = para.findall(".//{http://schemas.openxmlformats.org/wordprocessingml/2006/main}txbxContent")
        pict = para.findall(".//{http://schemas.openxmlformats.org/wordprocessingml/2006/main}pict")
        drawing = para.findall(".//{http://schemas.openxmlformats.org/wordprocessingml/2006/main}drawing")
        print(f"本段含: VML pict={len(pict)}, drawing={len(drawing)}, 文本框 txbxContent={len(txbx)}")
        print("\n说明: 该句与「附录A 体系结构图」在同一段落内，位于图形的 VML 文本框之后。")
