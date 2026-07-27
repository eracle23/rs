#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成《网络安全研究报告》图2：数据通讯流程图（SVG + PNG）。"""

from __future__ import annotations

import shutil
import zipfile
from pathlib import Path

import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = Path(
    r"C:\Users\lie76\Desktop\三维重建软件文档\过审材料\软件资料需求2026.06.23"
)
REPO_OUT = ROOT / "Applications/RadianceApp/Docs"

SVG_NAME = "数据通讯流程图.svg"
PNG_NAME = "数据通讯流程图.png"
MMD_NAME = "数据通讯流程图.mmd"

W, H = 980, 720


def _el(tag: str, attrib: dict | None = None, text: str | None = None) -> ET.Element:
    node = ET.Element(tag, attrib or {})
    if text is not None:
        node.text = text
    return node


def build_svg() -> str:
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
      .zone-title { font: bold 14px "Microsoft YaHei","SimHei",sans-serif; fill:#1F4E79; }
      .box-title { font: bold 12px "Microsoft YaHei","SimHei",sans-serif; fill:#1F4E79; text-anchor:middle; }
      .box-text { font: 11px "Microsoft YaHei","SimHei",sans-serif; fill:#333; text-anchor:middle; }
      .box-text-left { font: 11px "Microsoft YaHei","SimHei",sans-serif; fill:#333; }
      .arrow-label { font: 10px "Microsoft YaHei","SimHei",sans-serif; fill:#2F5597; }
      .note { font: 10px "Microsoft YaHei","SimHei",sans-serif; fill:#666; }
      .zone { fill:#F8FAFC; stroke:#1F4E79; stroke-width:1.6; }
      .app-zone { fill:#EEF4FB; stroke:#2E75B6; stroke-width:2; }
      .box { fill:#FFFFFF; stroke:#2F5597; stroke-width:1.2; }
      .box-accent { fill:#D6E4F0; stroke:#2F5597; stroke-width:1.2; }
      .box-core { fill:#4472C4; stroke:#1F4E79; stroke-width:1.2; }
      .box-core-text { font: bold 12px "Microsoft YaHei","SimHei",sans-serif; fill:#fff; text-anchor:middle; }
      .store { fill:#FFF2CC; stroke:#BF8F00; stroke-width:1.2; }
      .source { fill:#E2F0D9; stroke:#548235; stroke-width:1.2; }
      .arrow { stroke:#2F5597; stroke-width:1.4; fill:none; marker-end:url(#arrowhead); }
      .arrow-dashed { stroke:#7F7F7F; stroke-width:1.2; fill:none; stroke-dasharray:5 4; marker-end:url(#arrowhead-gray); }
    """
    defs.append(style)

    marker = _el(
        "marker",
        {
            "id": "arrowhead",
            "markerWidth": "8",
            "markerHeight": "8",
            "refX": "7",
            "refY": "4",
            "orient": "auto",
        },
    )
    marker.append(_el("path", {"d": "M0,0 L8,4 L0,8 Z", "fill": "#2F5597"}))
    defs.append(marker)

    marker_g = _el(
        "marker",
        {
            "id": "arrowhead-gray",
            "markerWidth": "8",
            "markerHeight": "8",
            "refX": "7",
            "refY": "4",
            "orient": "auto",
        },
    )
    marker_g.append(_el("path", {"d": "M0,0 L8,4 L0,8 Z", "fill": "#7F7F7F"}))
    defs.append(marker_g)

    svg.append(defs)

    def text(x, y, cls, content, anchor=None):
        attrs = {"x": str(x), "y": str(y), "class": cls}
        if anchor:
            attrs["text-anchor"] = anchor
        svg.append(_el("text", attrs, content))

    def rect(x, y, w, h, cls, rx="6"):
        svg.append(
            _el("rect", {"x": str(x), "y": str(y), "width": str(w), "height": str(h), "rx": rx, "class": cls})
        )

    def line(x1, y1, x2, y2, cls="arrow"):
        svg.append(_el("line", {"x1": str(x1), "y1": str(y1), "x2": str(x2), "y2": str(y2), "class": cls}))

    def polyline(points: str, cls="arrow"):
        svg.append(_el("polyline", {"points": points, "class": cls}))

    # Title
    text(W / 2, 34, "title", "图2  数据通讯流程图", "middle")
    text(
        W / 2,
        56,
        "subtitle",
        "医学影像三维重建软件（Vision Magic Ecosystem）— 单机本地数据通讯与存储",
        "middle",
    )

    # Outer workstation zone
    rect(40, 78, 900, 590, "zone")
    text(68, 102, "zone-title", "单机工作站运行环境（无需外部网络连接）")

    # Left: data sources
    rect(68, 130, 190, 220, "source")
    text(163, 154, "box-title", "本地数据源")
    text(163, 182, "box-text", "DICOM 文件夹 / .dcm")
    text(163, 204, "box-text", "影像文件 .nrrd / .nii")
    text(163, 226, "box-text", "已保存场景 .mrml / .mrb")
    text(163, 256, "box-text", "（用户本地磁盘选择）")

    # Center: application
    rect(290, 118, 420, 420, "app-zone")
    text(500, 142, "zone-title", "应用软件 VisionMagicEcosystem.exe", "middle")

    # Login
    rect(320, 162, 150, 78, "box-accent")
    text(395, 184, "box-title", "启动与身份鉴别")
    text(395, 206, "box-text", "qLoginDialog")
    text(395, 226, "box-text", "UserManager")

    # Import
    rect(500, 162, 180, 78, "box-accent")
    text(590, 184, "box-title", "DICOM / 数据导入")
    text(590, 206, "box-text", "DICOM 模块 / 添加数据")
    text(590, 226, "box-text", "VolumeImportValidator")

    # Core processing
    rect(350, 270, 300, 88, "box-core")
    text(500, 302, "box-core-text", "MRML 场景 / 内存数据处理")
    text(500, 324, "box-core-text", "2D·3D 浏览 · 分割 · 标注 · 测量")

    # Functions row
    rect(320, 386, 110, 56, "box")
    text(375, 410, "box-title", "三维重建")
    text(375, 430, "box-text", "体绘制/模型")

    rect(445, 386, 110, 56, "box")
    text(500, 410, "box-title", "医生批注")
    text(500, 430, "box-text", "场景保存")

    rect(570, 386, 110, 56, "box")
    text(625, 410, "box-title", "成果导出")
    text(625, 430, "box-text", "本地文件")

    # Logger
    rect(350, 468, 300, 52, "box-accent")
    text(500, 490, "box-title", "审计日志 AppLogger")
    text(500, 510, "box-text", "记录启动/退出、用户操作与错误信息")

    # Right: local storage
    rect(740, 130, 180, 340, "store")
    text(830, 154, "box-title", "本地存储")
    text(830, 182, "box-text", "users.db")
    text(830, 202, "box-text", "（SQLite 用户库）")
    text(830, 232, "box-text", "DICOMDatabase/")
    text(830, 252, "box-text", "（本地 DICOM 索引）")
    text(830, 282, "box-text", "logs/VisionMagic_*.log")
    text(830, 302, "box-text", "（审计日志）")
    text(830, 332, "box-text", "保存场景/导出结果")
    text(830, 352, "box-text", "（.mrml/.mrb 等）")

    # Arrows: source -> import
    line(258, 220, 500, 201)
    text(330, 208, "arrow-label", "本地文件读取")

    # source -> login (optional load scene)
    polyline("258,260 290,260 290,201", "arrow-dashed")
    text(272, 248, "arrow-label", "加载场景")

    # login -> users.db
    line(395, 240, 740, 190)
    text(540, 178, "arrow-label", "账户校验 / 读写")

    # import -> validator -> core
    line(590, 240, 590, 270)
    line(590, 270, 500, 270)

    # core -> functions
    line(500, 358, 375, 386)
    line(500, 358, 500, 386)
    line(500, 358, 625, 386)

    # functions -> storage
    line(625, 442, 740, 360)
    text(680, 392, "arrow-label", "写入本地")

    # logger <- app
    line(500, 442, 500, 468)
    line(395, 240, 395, 492)
    polyline("395,492 350,492")
    line(740, 430, 650, 492)
    polyline("650,492 500,492")

    # core -> storage (DICOM db)
    line(650, 314, 740, 250)
    text(688, 272, "arrow-label", "索引/缓存")

    # Notes
    text(
        60,
        690,
        "note",
        "说明：本软件以单机本地运行为主；数据导入、处理、保存均在用户工作站内完成，正常运行不依赖外部网络传输。",
    )
    text(
        60,
        708,
        "note",
        "可选：按部署策略启用授权/许可校验；DICOM 网络接收非默认必需功能。",
    )

    return ET.tostring(svg, encoding="unicode")


def write_mermaid(path: Path) -> None:
    content = """flowchart LR
    subgraph workstation [单机工作站]
        subgraph source [本地数据源]
            DICOM[DICOM文件夹/影像文件]
            Scene[场景文件 mrml/mrb]
        end

        subgraph app [VisionMagicEcosystem.exe]
            Login[用户登录\\nUserManager]
            Import[DICOM导入\\nVolumeImportValidator]
            Process[MRML场景处理\\n2D/3D/分割/标注]
            Logger[AppLogger审计日志]
        end

        subgraph storage [本地存储]
            UserDB[(users.db)]
            DicomDB[(DICOMDatabase)]
            Logs[(logs目录)]
            Output[(场景/导出文件)]
        end

        DICOM -->|本地文件读取| Import
        Scene -->|加载| Import
        Login <-->|账户校验| UserDB
        Import -->|校验通过| Process
        Process --> Output
        Import --> DicomDB
        Login --> Logger
        Import --> Logger
        Process --> Logger
    end
"""
    path.write_text(content, encoding="utf-8")


def svg_to_png(svg_path: Path, png_path: Path) -> bool:
    try:
        import cairosvg  # type: ignore

        cairosvg.svg2png(url=str(svg_path), write_to=str(png_path), output_width=W, output_height=H)
        return png_path.exists()
    except Exception:
        pass

    try:
        import subprocess

        subprocess.run(
            ["inkscape", str(svg_path), "-o", str(png_path), "-w", str(W), "-h", str(H)],
            check=True,
            capture_output=True,
        )
        return png_path.exists()
    except Exception:
        pass

    try:
        from PIL import Image, ImageDraw, ImageFont

        img = Image.new("RGB", (W, H), "white")
        draw = ImageDraw.Draw(img)

        def load_font(size, bold=False):
            for name in ("msyh.ttc", "msyhbd.ttc", "simhei.ttf", "arial.ttf"):
                try:
                    path = Path("C:/Windows/Fonts") / name
                    if path.exists():
                        return ImageFont.truetype(str(path), size=size)
                except OSError:
                    continue
            return ImageFont.load_default()

        ft_title = load_font(20, True)
        ft_sub = load_font(12)
        ft_zone = load_font(13, True)
        ft_box = load_font(11, True)
        ft_txt = load_font(10)
        ft_note = load_font(9)

        draw.text((W // 2, 18), "图2  数据通讯流程图", fill="#111", font=ft_title, anchor="ma")
        draw.text(
            (W // 2, 44),
            "医学影像三维重建软件（Vision Magic Ecosystem）— 单机本地数据通讯与存储",
            fill="#555",
            font=ft_sub,
            anchor="ma",
        )

        def rbox(x, y, w, h, fill, outline, title, lines):
            draw.rounded_rectangle((x, y, x + w, y + h), radius=8, fill=fill, outline=outline, width=2)
            draw.text((x + w // 2, y + 14), title, fill="#1F4E79", font=ft_box, anchor="ma")
            for i, line in enumerate(lines):
                draw.text((x + w // 2, y + 38 + i * 18), line, fill="#333", font=ft_txt, anchor="ma")

        def arr(x1, y1, x2, y2, color="#2F5597"):
            draw.line((x1, y1, x2, y2), fill=color, width=2)
            # simple arrow head
            import math

            ang = math.atan2(y2 - y1, x2 - x1)
            for da in (2.6, -2.6):
                ax = x2 - 10 * math.cos(ang - da * 0.2)
                ay = y2 - 10 * math.sin(ang - da * 0.2)
                draw.line((x2, y2, ax, ay), fill=color, width=2)

        draw.rounded_rectangle((40, 78, 940, 668), radius=10, fill="#F8FAFC", outline="#1F4E79", width=2)
        draw.text((68, 88), "单机工作站运行环境（无需外部网络连接）", fill="#1F4E79", font=ft_zone)

        rbox(68, 130, 190, 220, "#E2F0D9", "#548235", "本地数据源", ["DICOM 文件夹 / .dcm", "影像 .nrrd / .nii", "场景 .mrml / .mrb", "（用户本地磁盘）"])
        draw.rounded_rectangle((290, 118, 710, 538), radius=10, fill="#EEF4FB", outline="#2E75B6", width=2)
        draw.text((500, 128), "应用软件 VisionMagicEcosystem.exe", fill="#1F4E79", font=ft_zone, anchor="ma")
        rbox(320, 162, 150, 78, "#D6E4F0", "#2F5597", "启动与身份鉴别", ["qLoginDialog", "UserManager"])
        rbox(500, 162, 180, 78, "#D6E4F0", "#2F5597", "DICOM / 数据导入", ["DICOM 模块", "VolumeImportValidator"])
        rbox(350, 270, 300, 88, "#4472C4", "#1F4E79", "MRML 场景 / 内存处理", ["2D·3D 浏览 · 分割 · 标注 · 测量"])
        rbox(320, 386, 110, 56, "#FFFFFF", "#2F5597", "三维重建", ["体绘制/模型"])
        rbox(445, 386, 110, 56, "#FFFFFF", "#2F5597", "医生批注", ["场景保存"])
        rbox(570, 386, 110, 56, "#FFFFFF", "#2F5597", "成果导出", ["本地文件"])
        rbox(350, 468, 300, 52, "#D6E4F0", "#2F5597", "审计日志 AppLogger", ["启动/退出 · 用户操作 · 错误"])
        rbox(740, 130, 180, 340, "#FFF2CC", "#BF8F00", "本地存储", ["users.db", "DICOMDatabase/", "logs/VisionMagic_*.log", "场景/导出文件"])

        arr(258, 220, 500, 201)
        draw.text((330, 200), "本地文件读取", fill="#2F5597", font=ft_note)
        arr(395, 240, 740, 190)
        draw.text((540, 170), "账户校验 / 读写", fill="#2F5597", font=ft_note)
        arr(590, 240, 590, 270)
        arr(590, 270, 500, 270)
        arr(625, 442, 740, 360)
        draw.text((680, 382), "写入本地", fill="#2F5597", font=ft_note)
        arr(650, 314, 740, 250)
        draw.text((688, 262), "索引/缓存", fill="#2F5597", font=ft_note)
        arr(500, 442, 500, 468)

        draw.text((60, 684), "说明：本软件以单机本地运行为主；数据导入、处理、保存均在用户工作站内完成，正常运行不依赖外部网络传输。", fill="#666", font=ft_note)
        draw.text((60, 702), "可选：按部署策略启用授权/许可校验；DICOM 网络接收非默认必需功能。", fill="#666", font=ft_note)

        img.save(png_path, "PNG")
        return png_path.exists()
    except Exception as exc:
        print(f"Pillow PNG 失败: {exc}")
        return False


def replace_docx_image(docx_path: Path, png_path: Path, media_name: str = "image2.png") -> None:
    tmp = docx_path.with_suffix(".tmp.docx")
    target = f"word/media/{media_name}"
    with zipfile.ZipFile(docx_path, "r") as zin, zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zout:
        for item in zin.infolist():
            data = zin.read(item.filename)
            if item.filename == target:
                data = png_path.read_bytes()
            zout.writestr(item, data)
    shutil.move(str(tmp), str(docx_path))


def main() -> None:
    for d in (OUT_DIR, REPO_OUT):
        d.mkdir(parents=True, exist_ok=True)

    svg_text = build_svg()
    svg_header = '<?xml version="1.0" encoding="UTF-8"?>\n'
    if not svg_text.startswith("<?xml"):
        svg_text = svg_header + svg_text

    svg_paths = [OUT_DIR / SVG_NAME, REPO_OUT / SVG_NAME]
    for p in svg_paths:
        p.write_text(svg_text, encoding="utf-8")
        print(f"SVG: {p}")

    mmd_paths = [OUT_DIR / MMD_NAME, REPO_OUT / "数据通讯流程图.mmd"]
    for p in mmd_paths:
        write_mermaid(p)
        print(f"Mermaid: {p}")

    png_path = OUT_DIR / PNG_NAME
    png_ok = svg_to_png(svg_paths[0], png_path)
    if png_ok:
        shutil.copy2(png_path, REPO_OUT / PNG_NAME)
        print(f"PNG: {png_path}")
    else:
        print("PNG: 未生成（缺少 cairosvg/inkscape），请 Word 直接插入 SVG")

    docx = OUT_DIR / "CH3.5.5.11-网络安全研究报告.docx"
    if png_ok and docx.exists():
        try:
            replace_docx_image(docx, png_path)
            print(f"已替换 Word 图2: {docx}")
        except PermissionError:
            print(f"Word 文档被占用，请关闭后手动插入: {png_path}")


if __name__ == "__main__":
    main()
