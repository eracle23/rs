#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成《网络安全研究报告》图1：网络环境图（SVG + PNG）。"""

from __future__ import annotations

import math
import shutil
import zipfile
from pathlib import Path

import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = Path(
    r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料\软件资料需求2026.06.23"
)
REPO_OUT = ROOT / "Applications/RadianceApp/Docs"

SVG_NAME = "网络环境图.svg"
PNG_NAME = "网络环境图.png"
MMD_NAME = "网络环境图.mmd"

W = 980
H = 800

# 统一布局常量（SVG / PNG 共用）
LAYOUT = {
    "outer_x": 100,
    "outer_y": 188,
    "outer_w": 780,
    "outer_h": 348,
    "pc_x": 160,
    "pc_y": 222,
    "pc_w": 660,
    "pc_h": 232,
    "app_x": 240,
    "app_y": 248,
    "app_w": 500,
    "app_h": 58,
    "mod_y": 318,
    "mod_h": 88,
    "sec_y": 418,
    "sec_h": 34,
    "media_y": 468,
    "media_h": 58,
    "media_local_x": 200,
    "media_local_w": 280,
    "media_pacs_x": 580,
    "media_pacs_w": 220,
    "note_y": 652,
    "note_h": 118,
}


def _el(tag: str, attrib: dict | None = None, text: str | None = None) -> ET.Element:
    node = ET.Element(tag, attrib or {})
    if text is not None:
        node.text = text
    return node


def build_svg() -> str:
    L = LAYOUT
    ET.register_namespace("", "http://www.w3.org/2000/svg")
    svg = _el(
        "svg",
        {
            "xmlns": "http://www.w3.org/2000/svg",
            "width": str(W),
            "height": str(H),
            "viewBox": f"0 0 {W} {H}",
        },
    )

    defs = _el("defs")
    style = _el("style")
    style.text = """
      .title { font: bold 20px "Microsoft YaHei","SimHei",sans-serif; fill:#111; }
      .subtitle { font: 13px "Microsoft YaHei","SimHei",sans-serif; fill:#555; }
      .zone-title { font: bold 13px "Microsoft YaHei","SimHei",sans-serif; fill:#1F4E79; }
      .box-title { font: bold 12px "Microsoft YaHei","SimHei",sans-serif; fill:#1F4E79; text-anchor:middle; }
      .box-text { font: 11px "Microsoft YaHei","SimHei",sans-serif; fill:#333; text-anchor:middle; }
      .note-title { font: bold 11px "Microsoft YaHei","SimHei",sans-serif; fill:#444; }
      .note { font: 10px "Microsoft YaHei","SimHei",sans-serif; fill:#555; }
      .legend { font: 10px "Microsoft YaHei","SimHei",sans-serif; fill:#2F5597; }
      .legend-gray { font: 10px "Microsoft YaHei","SimHei",sans-serif; fill:#777; }
      .pc-zone { fill:#EEF4FB; stroke:#2E75B6; stroke-width:2; }
      .outer-zone { fill:#F8FAFC; stroke:#1F4E79; stroke-width:1.6; }
      .note-zone { fill:#F5F5F5; stroke:#BBBBBB; stroke-width:1; }
      .app-box { fill:#4472C4; stroke:#1F4E79; stroke-width:1.4; }
      .app-text { font: bold 12px "Microsoft YaHei","SimHei",sans-serif; fill:#fff; text-anchor:middle; }
      .sub-box { fill:#FFFFFF; stroke:#2F5597; stroke-width:1.2; }
      .store-box { fill:#FFF2CC; stroke:#BF8F00; stroke-width:1.2; }
      .user-box { fill:#E2F0D9; stroke:#548235; stroke-width:1.2; }
      .media-box { fill:#EDEDED; stroke:#666666; stroke-width:1.2; }
      .sec-box { fill:#FCE4D6; stroke:#C55A11; stroke-width:1.2; }
      .arrow { stroke:#2F5597; stroke-width:1.5; fill:none; marker-end:url(#arrowhead); }
      .arrow-gray { stroke:#888888; stroke-width:1.2; fill:none; stroke-dasharray:6 4; marker-end:url(#arrowhead-gray); }
    """
    defs.append(style)
    for mid, color in (("arrowhead", "#2F5597"), ("arrowhead-gray", "#888888")):
        m = _el("marker", {"id": mid, "markerWidth": "8", "markerHeight": "8", "refX": "7", "refY": "4", "orient": "auto"})
        m.append(_el("path", {"d": "M0,0 L8,4 L0,8 Z", "fill": color}))
        defs.append(m)
    svg.append(defs)

    def text(x, y, cls, content, anchor=None):
        attrs = {"x": str(x), "y": str(y), "class": cls}
        if anchor:
            attrs["text-anchor"] = anchor
        svg.append(_el("text", attrs, content))

    def rect(x, y, w, h, cls, rx="8"):
        svg.append(_el("rect", {"x": str(x), "y": str(y), "width": str(w), "height": str(h), "rx": rx, "class": cls}))

    def line(x1, y1, x2, y2, cls="arrow"):
        svg.append(_el("line", {"x1": str(x1), "y1": str(y1), "x2": str(x2), "y2": str(y2), "class": cls}))

    # 标题
    text(W / 2, 32, "title", "图1  网络环境图", "middle")
    text(W / 2, 54, "subtitle", "医学影像三维重建软件（Vision Magic Ecosystem）— 单机部署网络环境", "middle")

    # 操作者
    rect(420, 72, 140, 52, "user-box")
    text(490, 92, "box-title", "操作者")
    text(490, 110, "box-text", "临床/技术人员")
    rect(430, 136, 120, 26, "sub-box", "4")
    text(490, 153, "box-text", "显示与输入设备")
    line(490, 124, 490, 136)
    line(490, 162, 490, L["outer_y"])

    # 部署边界
    rect(L["outer_x"], L["outer_y"], L["outer_w"], L["outer_h"], "outer-zone")
    text(L["outer_x"] + 16, L["outer_y"] + 20, "zone-title", "医疗机构内网 / 单机工作站部署边界（正常运行无需连接外部网络）")

    # 工作站
    rect(L["pc_x"], L["pc_y"], L["pc_w"], L["pc_h"], "pc-zone")
    text(L["pc_x"] + L["pc_w"] / 2, L["pc_y"] + 18, "zone-title", "Windows 10/11 64 位 工作站", "middle")

    rect(L["app_x"], L["app_y"], L["app_w"], L["app_h"], "app-box")
    text(L["app_x"] + L["app_w"] / 2, L["app_y"] + 26, "app-text", "医学影像三维重建软件")
    text(L["app_x"] + L["app_w"] / 2, L["app_y"] + 46, "app-text", "VisionMagicEcosystem.exe")

    mod_w = 180
    gap = 24
    x1 = L["pc_x"] + 40
    x2 = x1 + mod_w + gap
    x3 = x2 + mod_w + gap
    my = L["mod_y"]
    mh = L["mod_h"]

    rect(x1, my, mod_w, mh, "sub-box")
    text(x1 + mod_w / 2, my + 22, "box-title", "身份鉴别")
    text(x1 + mod_w / 2, my + 44, "box-text", "UserManager")
    text(x1 + mod_w / 2, my + 64, "box-text", "users.db")

    rect(x2, my, mod_w, mh, "store-box")
    text(x2 + mod_w / 2, my + 22, "box-title", "本地 DICOM 库")
    text(x2 + mod_w / 2, my + 44, "box-text", "DICOMDatabase/")
    text(x2 + mod_w / 2, my + 64, "box-text", "本地索引与缓存")

    rect(x3, my, mod_w, mh, "store-box")
    text(x3 + mod_w / 2, my + 22, "box-title", "审计与日志")
    text(x3 + mod_w / 2, my + 44, "box-text", "AppLogger")
    text(x3 + mod_w / 2, my + 64, "box-text", "logs/ 目录")

    sy, sh = L["sec_y"], L["sec_h"]
    sx = L["pc_x"] + 80
    sw = L["pc_w"] - 160
    rect(sx, sy, sw, sh, "sec-box")
    text(sx + sw / 2, sy + 16, "box-title", "推荐：火绒等杀毒软件（与本软件共存验证）")
    text(sx + sw / 2, sy + 32, "box-text", "操作系统账户权限 + 物理访问控制")

    # 外部数据源（部署边界下方，说明区上方）
    ly, lh = L["media_y"], L["media_h"]
    lx, lw = L["media_local_x"], L["media_local_w"]
    px, pw = L["media_pacs_x"], L["media_pacs_w"]

    rect(lx, ly, lw, lh, "media-box")
    text(lx + lw / 2, ly + 20, "box-title", "本地磁盘 / 移动存储")
    text(lx + lw / 2, ly + 40, "box-text", "DICOM 文件夹 · 场景文件 · 导出结果")
    text(lx + lw / 2, ly + 58, "box-text", "（USB / 光盘等离线介质）")

    rect(px, ly, pw, lh, "media-box")
    text(px + pw / 2, ly + 20, "box-title", "外部网络 / PACS")
    text(px + pw / 2, ly + 40, "box-text", "非默认必需")
    text(px + pw / 2, ly + 58, "box-text", "（可选，按现场配置）")

    # 箭头：数据源 -> 工作站
    line(lx + lw / 2, ly, x2 + mod_w / 2, L["pc_y"] + L["pc_h"], "arrow")
    text(lx + lw / 2 + 20, ly - 12, "legend", "离线导入")
    line(px + pw / 2, ly, x3 + mod_w / 2, my, "arrow-gray")
    text(px + 10, ly - 12, "legend-gray", "可选连接")

    # 底部说明（独立灰色区域，避免与图形重叠）
    ny, nh = L["note_y"], L["note_h"]
    rect(60, ny, W - 120, nh, "note-zone")
    text(76, ny + 22, "note-title", "说明")
    text(76, ny + 44, "note", "1. 产品默认以单机方式部署于用户工作站，影像数据通过本地文件系统或离线介质导入，不依赖持续网络连接。")
    text(76, ny + 64, "note", "2. 用户账户、DICOM 索引、审计日志及处理结果均存储于本机；软件无默认对外网络服务端口。")
    text(76, ny + 84, "note", "3. 可按部署策略启用授权/许可校验；DICOM 网络接收为可选功能，非正常运行所必需。")

    body = ET.tostring(svg, encoding="unicode")
    return ('<?xml version="1.0" encoding="UTF-8"?>\n' + body) if not body.startswith("<?xml") else body


def write_mermaid(path: Path) -> None:
    path.write_text(
        """flowchart TB
    User[操作者] --> UI[显示与输入设备]
    UI --> PC[Windows工作站]

    subgraph deploy [单机部署边界]
        PC --> App[VisionMagicEcosystem.exe]
        App --> Auth[UserManager / users.db]
        App --> DicomDB[DICOMDatabase]
        App --> Logs[AppLogger / logs]
        App --> AV[火绒等安全软件共存]
    end

    Media[本地磁盘或移动存储] -->|离线导入| App
    PACS[外部网络/PACS] -.->|可选非默认| App
""",
        encoding="utf-8",
    )


def _load_font(size: int):
    from PIL import ImageFont

    for name in ("msyh.ttc", "msyhbd.ttc", "simhei.ttf"):
        p = Path("C:/Windows/Fonts") / name
        if p.exists():
            return ImageFont.truetype(str(p), size=size)
    return ImageFont.load_default()


def render_png(png_path: Path) -> bool:
    try:
        from PIL import Image, ImageDraw

        L = LAYOUT
        img = Image.new("RGB", (W, H), "white")
        draw = ImageDraw.Draw(img)
        ft20 = _load_font(20)
        ft13 = _load_font(13)
        ft12 = _load_font(12)
        ft11 = _load_font(11)
        ft10 = _load_font(10)
        ft9 = _load_font(9)

        def rbox(x, y, w, h, fill, outline, title, lines, ty=14):
            draw.rounded_rectangle((x, y, x + w, y + h), radius=8, fill=fill, outline=outline, width=2)
            draw.text((x + w // 2, y + ty), title, fill="#1F4E79", font=ft12, anchor="ma")
            for i, ln in enumerate(lines):
                draw.text((x + w // 2, y + 34 + i * 18), ln, fill="#333", font=ft10, anchor="ma")

        def arr(x1, y1, x2, y2, color="#2F5597", dashed=False):
            if dashed:
                steps = 24
                for i in range(steps):
                    if i % 2 == 0:
                        t1, t2 = i / steps, (i + 1) / steps
                        draw.line(
                            (x1 + (x2 - x1) * t1, y1 + (y2 - y1) * t1, x1 + (x2 - x1) * t2, y1 + (y2 - y1) * t2),
                            fill=color,
                            width=2,
                        )
            else:
                draw.line((x1, y1, x2, y2), fill=color, width=2)
            ang = math.atan2(y2 - y1, x2 - x1)
            for da in (2.6, -2.6):
                draw.line((x2, y2, x2 - 10 * math.cos(ang - da * 0.2), y2 - 10 * math.sin(ang - da * 0.2)), fill=color, width=2)

        draw.text((W // 2, 16), "图1  网络环境图", fill="#111", font=ft20, anchor="ma")
        draw.text((W // 2, 42), "医学影像三维重建软件（Vision Magic Ecosystem）— 单机部署网络环境", fill="#555", font=ft11, anchor="ma")

        rbox(420, 72, 140, 52, "#E2F0D9", "#548235", "操作者", ["临床/技术人员"])
        rbox(430, 136, 120, 26, "#FFFFFF", "#2F5597", "显示与输入设备", [], ty=8)
        arr(490, 124, 490, 136)
        arr(490, 162, 490, L["outer_y"])

        draw.rounded_rectangle(
            (L["outer_x"], L["outer_y"], L["outer_x"] + L["outer_w"], L["outer_y"] + L["outer_h"]),
            radius=10,
            fill="#F8FAFC",
            outline="#1F4E79",
            width=2,
        )
        draw.text((L["outer_x"] + 16, L["outer_y"] + 8), "医疗机构内网 / 单机工作站部署边界（正常运行无需连接外部网络）", fill="#1F4E79", font=ft12)

        draw.rounded_rectangle(
            (L["pc_x"], L["pc_y"], L["pc_x"] + L["pc_w"], L["pc_y"] + L["pc_h"]),
            radius=10,
            fill="#EEF4FB",
            outline="#2E75B6",
            width=2,
        )
        draw.text((L["pc_x"] + L["pc_w"] // 2, L["pc_y"] + 8), "Windows 10/11 64 位 工作站", fill="#1F4E79", font=ft12, anchor="ma")

        ax, ay, aw, ah = L["app_x"], L["app_y"], L["app_w"], L["app_h"]
        draw.rounded_rectangle((ax, ay, ax + aw, ay + ah), radius=8, fill="#4472C4", outline="#1F4E79", width=2)
        draw.text((ax + aw // 2, ay + 20), "医学影像三维重建软件", fill="#fff", font=ft12, anchor="ma")
        draw.text((ax + aw // 2, ay + 40), "VisionMagicEcosystem.exe", fill="#fff", font=ft11, anchor="ma")

        mod_w, gap = 180, 24
        x1 = L["pc_x"] + 40
        x2, x3 = x1 + mod_w + gap, x1 + 2 * (mod_w + gap)
        my, mh = L["mod_y"], L["mod_h"]
        rbox(x1, my, mod_w, mh, "#FFFFFF", "#2F5597", "身份鉴别", ["UserManager", "users.db"])
        rbox(x2, my, mod_w, mh, "#FFF2CC", "#BF8F00", "本地 DICOM 库", ["DICOMDatabase/", "本地索引与缓存"])
        rbox(x3, my, mod_w, mh, "#FFF2CC", "#BF8F00", "审计与日志", ["AppLogger", "logs/ 目录"])

        sx, sy, sw, sh = L["pc_x"] + 80, L["sec_y"], L["pc_w"] - 160, L["sec_h"]
        rbox(sx, sy, sw, sh, "#FCE4D6", "#C55A11", "推荐：火绒等杀毒软件（与本软件共存验证）", ["操作系统账户权限 + 物理访问控制"], ty=10)

        ly, lh = L["media_y"], L["media_h"]
        lx, lw = L["media_local_x"], L["media_local_w"]
        px, pw = L["media_pacs_x"], L["media_pacs_w"]
        rbox(lx, ly, lw, lh, "#EDEDED", "#666666", "本地磁盘 / 移动存储", ["DICOM · 场景 · 导出", "（USB / 光盘等离线介质）"])
        rbox(px, ly, pw, lh, "#EDEDED", "#666666", "外部网络 / PACS", ["非默认必需", "（可选，按现场配置）"])

        arr(lx + lw // 2, ly, x2 + mod_w // 2, L["pc_y"] + L["pc_h"])
        draw.text((lx + lw // 2 + 24, ly - 14), "离线导入", fill="#2F5597", font=ft9)
        arr(px + pw // 2, ly, x3 + mod_w // 2, my, color="#888888", dashed=True)
        draw.text((px + 8, ly - 14), "可选连接", fill="#888888", font=ft9)

        ny, nh = L["note_y"], L["note_h"]
        draw.rounded_rectangle((60, ny, W - 60, ny + nh), radius=6, fill="#F5F5F5", outline="#BBBBBB", width=1)
        draw.text((76, ny + 12), "说明", fill="#444444", font=ft11)
        notes = [
            "1. 产品默认以单机方式部署于用户工作站，影像数据通过本地文件系统或离线介质导入，不依赖持续网络连接。",
            "2. 用户账户、DICOM 索引、审计日志及处理结果均存储于本机；软件无默认对外网络服务端口。",
            "3. 可按部署策略启用授权/许可校验；DICOM 网络接收为可选功能，非正常运行所必需。",
        ]
        for i, note in enumerate(notes):
            draw.text((76, ny + 34 + i * 22), note, fill="#555555", font=ft9)

        img.save(png_path, "PNG")
        return True
    except Exception as exc:
        print(f"PNG 生成失败: {exc}")
        return False


def svg_to_png(svg_path: Path, png_path: Path) -> bool:
    try:
        import cairosvg  # type: ignore

        cairosvg.svg2png(url=str(svg_path), write_to=str(png_path), output_width=W, output_height=H)
        if png_path.exists():
            return True
    except Exception:
        pass
    return render_png(png_path)


def replace_docx_image(docx_path: Path, png_path: Path, media_name: str) -> bool:
    tmp = docx_path.with_suffix(".tmp.docx")
    target = f"word/media/{media_name}"
    try:
        with zipfile.ZipFile(docx_path, "r") as zin, zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zout:
            for item in zin.infolist():
                data = zin.read(item.filename)
                if item.filename == target:
                    data = png_path.read_bytes()
                zout.writestr(item, data)
        shutil.move(str(tmp), str(docx_path))
        return True
    except (PermissionError, OSError) as exc:
        print(f"无法更新 {docx_path.name}: {exc}")
        if tmp.exists():
            tmp.unlink(missing_ok=True)
        return False


def embed_both_figures(base_docx: Path, out_docx: Path, png1: Path, png2: Path) -> None:
    shutil.copy2(base_docx, out_docx)
    for media, png in (("image1.png", png1), ("image2.png", png2)):
        tmp = out_docx.with_suffix(".tmp.docx")
        target = f"word/media/{media}"
        with zipfile.ZipFile(out_docx, "r") as zin, zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zout:
            for item in zin.infolist():
                data = zin.read(item.filename)
                if item.filename == target:
                    data = png.read_bytes()
                zout.writestr(item, data)
        out_docx.unlink(missing_ok=True)
        shutil.move(tmp, out_docx)


def main() -> None:
    for d in (OUT_DIR, REPO_OUT):
        d.mkdir(parents=True, exist_ok=True)

    svg_text = build_svg()
    for p in (OUT_DIR / SVG_NAME, REPO_OUT / SVG_NAME):
        p.write_text(svg_text, encoding="utf-8")
        print(f"SVG: {p}")

    for p in (OUT_DIR / MMD_NAME, REPO_OUT / MMD_NAME):
        write_mermaid(p)

    png_path = OUT_DIR / PNG_NAME
    if svg_to_png(OUT_DIR / SVG_NAME, png_path):
        shutil.copy2(png_path, REPO_OUT / PNG_NAME)
        print(f"PNG: {png_path}")

    png2 = OUT_DIR / "数据通讯流程图.png"
    base = OUT_DIR / "CH3.5.5.11-网络安全研究报告.docx"
    merged = OUT_DIR / "CH3.5.5.11-网络安全研究报告-图1图2更新.docx"

    if png_path.exists() and png2.exists() and base.exists():
        embed_both_figures(base, merged, png_path, png2)
        print(f"合并版报告: {merged}")

    if png_path.exists() and base.exists():
        copy = OUT_DIR / "CH3.5.5.11-网络安全研究报告-图1更新.docx"
        shutil.copy2(base, copy)
        replace_docx_image(copy, png_path, "image1.png")


if __name__ == "__main__":
    main()
