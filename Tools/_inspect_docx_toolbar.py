# -*- coding: utf-8 -*-
import re
import zipfile
from pathlib import Path

src = Path(r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料\医学影像三维重建软件产品技术要求.docx")
outdir = Path(r"E:\GitHub\rs0\rs\Tools\_docx_images")
outdir.mkdir(exist_ok=True)

with zipfile.ZipFile(src) as z:
    xml = z.read("word/document.xml").decode("utf-8")
    parts = xml.split("</w:p>")
    for pi, para in enumerate(parts):
        embeds = re.findall(r'r:embed="(rId\d+)"', para)
        text = "".join(re.findall(r"<w:t[^>]*>([^<]*)</w:t>", para))
        if embeds or (text.strip() and 20 <= pi <= 80):
            print(f"p{pi} embeds={embeds} text={text.strip()[:150]!r}")
    for i in range(1, 43):
        for ext in (".png", ".jpeg", ".jpg", ".svg"):
            name = f"word/media/image{i}{ext}"
            if name in z.namelist():
                (outdir / f"image{i}{ext}").write_bytes(z.read(name))
                print("extracted", name)

    rels = z.read("word/_rels/document.xml.rels").decode("utf-8")
    rid_map = dict(re.findall(r'Id="(rId\d+)"[^>]*Target="([^"]+)"', rels))
    print("--- RID MAP ---")
    for pi in range(44, 82):
        para = parts[pi]
        embeds = re.findall(r'r:embed="(rId\d+)"', para)
        text = "".join(re.findall(r"<w:t[^>]*>([^<]*)</w:t>", para)).strip()
        if embeds:
            img = rid_map.get(embeds[0], "?")
            print(f"p{pi} {img} | {text[:100]}")
