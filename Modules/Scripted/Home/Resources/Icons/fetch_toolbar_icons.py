#!/usr/bin/env python3
"""
Download toolbar icons from the open-source Tabler Icons set and recolor them
to match Radiance's dark-theme accent palette.

Source: https://tabler-icons.io/ (MIT License)
"""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
from urllib.error import URLError
from urllib.request import urlopen

TABLER_TAG = "v2.47.0"
BASE_URL = f"https://raw.githubusercontent.com/tabler/tabler-icons/{TABLER_TAG}/icons/{{slug}}.svg"
ICON_SIZE = 48


@dataclass(frozen=True)
class IconSpec:
    slug: str
    filename: str
    color: str
    description: str


ICON_SPECS: Iterable[IconSpec] = [
    IconSpec("home", "toolbar_home.svg", "#5CC8FF", "Home / landing"),
    IconSpec("database", "toolbar_data.svg", "#5CE5C4", "Data manager"),
    IconSpec("stack-2", "toolbar_volumes.svg", "#7AC8FF", "Volumes module"),
    IconSpec("section", "toolbar_segment.svg", "#FFB26B", "Segmentation workflows"),
    IconSpec("3d-cube-sphere", "toolbar_render.svg", "#FF83D8", "Rendering & viz"),
    IconSpec("settings", "toolbar_settings.svg", "#FFD166", "Settings / preferences"),
    # Core I/O actions (MainToolBar)
    IconSpec("file-plus", "toolbar_add_data.svg", "#4FC3F7", "Add data"),
    IconSpec("database-import", "toolbar_dicom.svg", "#4ED2B4", "DICOM browser"),
    IconSpec("device-floppy", "toolbar_save.svg", "#8BE371", "Save scene"),
    IconSpec("arrow-back-up", "toolbar_undo.svg", "#FFB347", "Undo"),
    IconSpec("arrow-forward-up", "toolbar_redo.svg", "#FF8A65", "Redo"),
    IconSpec("camera", "toolbar_snapshot.svg", "#F48FB1", "Screenshot / capture"),
    IconSpec("layout-grid-add", "toolbar_layout.svg", "#A892FF", "Layout manager"),
    IconSpec("camera-bolt", "toolbar_scene_view.svg", "#73D2FF", "Scene view capture"),
    IconSpec("puzzle", "toolbar_extensions.svg", "#FF8AD8", "Extensions manager"),
    IconSpec("arrows-move", "toolbar_view_control.svg", "#7BDFF2", "View navigation / mouse mode"),
    IconSpec("adjustments", "toolbar_window_level.svg", "#FF9E7C", "Window/level adjustments"),
    IconSpec("pin", "toolbar_place.svg", "#FFAF7B", "Markups placement"),
    IconSpec("crosshair", "toolbar_crosshair.svg", "#63E6BE", "Crosshair toggle"),
]


def fetch_svg(slug: str) -> str:
    url = BASE_URL.format(slug=slug)
    with urlopen(url) as response:  # nosec: B310 (trusted OSS source)
        return response.read().decode("utf-8")


def recolor_svg(svg_text: str, accent_color: str) -> str:
    ET.register_namespace("", "http://www.w3.org/2000/svg")
    root = ET.fromstring(svg_text)
    root.set("width", str(ICON_SIZE))
    root.set("height", str(ICON_SIZE))
    # enforce consistent visual weight for high-DPI displays
    root.set("stroke-width", "1.8")

    style = root.get("style", "")
    style = f"{style};" if style and not style.endswith(";") else style
    style = f"{style}color:{accent_color}"
    root.set("style", style)
    return ET.tostring(root, encoding="unicode")


def save_icon(target: Path, svg_text: str) -> None:
    target.write_text(svg_text, encoding="utf-8")
    print(f"Wrote {target}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Download and brand toolbar icons from Tabler Icons.")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("Modules/Scripted/Home/Resources/Icons/Toolbar"),
        help="Directory where recolored SVGs should be written.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    for spec in ICON_SPECS:
        try:
            svg = fetch_svg(spec.slug)
        except URLError as exc:
            print(f"Failed to download '{spec.slug}': {exc}", file=sys.stderr)
            return 1
        branded = recolor_svg(svg, spec.color)
        save_icon(args.output_dir / spec.filename, branded)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
