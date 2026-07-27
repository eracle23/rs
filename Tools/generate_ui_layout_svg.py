#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate 用户界面布局图.svg with valid UTF-8 XML (numeric entities for CJK)."""

from pathlib import Path
import xml.etree.ElementTree as ET

OUTPUT = Path(__file__).resolve().parents[1] / "Applications/RadianceApp/Docs/用户界面布局图.svg"

CONTENT = r"""<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="760" viewBox="0 0 1000 760">
  <defs>
    <style>
      .title { font: bold 20px "Microsoft YaHei", "SimHei", sans-serif; fill: #111; }
      .subtitle { font: 13px "Microsoft YaHei", "SimHei", sans-serif; fill: #333; }
      .region-label { font: bold 14px "Microsoft YaHei", "SimHei", sans-serif; fill: #1F4E79; }
      .region-desc { font: 12px "Microsoft YaHei", "SimHei", sans-serif; fill: #444; }
      .text-white { font: 12px "Microsoft YaHei", "SimHei", sans-serif; fill: #fff; text-anchor: middle; dominant-baseline: middle; }
      .text-white-sm { font: 11px "Microsoft YaHei", "SimHei", sans-serif; fill: #fff; text-anchor: middle; dominant-baseline: middle; }
      .text-dark { font: 12px "Microsoft YaHei", "SimHei", sans-serif; fill: #1F4E79; text-anchor: middle; dominant-baseline: middle; }
      .text-dark-sm { font: 11px "Microsoft YaHei", "SimHei", sans-serif; fill: #2F5597; text-anchor: middle; dominant-baseline: middle; }
      .window { fill: #F5F7FA; stroke: #1F4E79; stroke-width: 2; }
      .titlebar { fill: #2E75B6; stroke: #1F4E79; stroke-width: 1; }
      .menu { fill: #5B9BD5; stroke: #2F5597; stroke-width: 1; }
      .toolbar { fill: #4472C4; stroke: #2F5597; stroke-width: 1; }
      .panel { fill: #D6E4F0; stroke: #2F5597; stroke-width: 1.2; }
      .viewport { fill: #EEF3F8; stroke: #2F5597; stroke-width: 1.2; }
      .view-cell { fill: #fff; stroke: #5B9BD5; stroke-width: 1; }
      .dock { fill: #BDD7EE; stroke: #2F5597; stroke-width: 1.2; }
      .status { fill: #D9D9D9; stroke: #888; stroke-width: 1; }
      .module-btn { fill: #fff; stroke: #2F5597; stroke-width: 1; }
      .legend-box { fill: #fff; stroke: #888; stroke-width: 1; }
      .callout-text { font: 11px "Microsoft YaHei", "SimHei", sans-serif; fill: #C55A11; }
      .num { font: bold 11px "Microsoft YaHei", sans-serif; fill: #fff; text-anchor: middle; dominant-baseline: middle; }
      .num-bg { fill: #C55A11; }
    </style>
  </defs>

  <text x="500" y="30" class="title" text-anchor="middle">&#x533B;&#x5B66;&#x5F71;&#x50CF;&#x4E09;&#x7EF4;&#x91CD;&#x5EFA;&#x8F6F;&#x4EF6; &#x2014; &#x7528;&#x6237;&#x754C;&#x9762;&#x5E03;&#x5C40;&#x56FE;</text>
  <text x="500" y="52" class="subtitle" text-anchor="middle">&#x4E3B;&#x5DE5;&#x4F5C;&#x754C;&#x9762;&#x7A7A;&#x95F4;&#x5E03;&#x5C40;&#x4E0E;&#x5404;&#x529F;&#x80FD;&#x533A;&#x57DF;&#x793A;&#x610F;</text>

  <rect x="60" y="72" width="880" height="620" rx="6" class="window"/>

  <rect x="72" y="84" width="856" height="36" rx="3" class="titlebar"/>
  <text x="500" y="102" class="text-white">&#x533B;&#x5B66;&#x5F71;&#x50CF;&#x4E09;&#x7EF4;&#x91CD;&#x5EFA;&#x8F6F;&#x4EF6;</text>
  <text x="900" y="102" class="text-white-sm">&#x2014; &#x25A1; &#x00D7;</text>
  <circle cx="78" cy="96" r="9" class="num-bg"/><text x="78" y="96" class="num">1</text>

  <rect x="72" y="126" width="856" height="32" rx="2" class="menu"/>
  <text x="500" y="142" class="text-white">&#x5DE5;&#x4F5C;&#x533A; | &#x7528;&#x6237; | &#x5E03;&#x5C40; | &#x5916;&#x89C2; | &#x5E2E;&#x52A9;</text>
  <circle cx="78" cy="142" r="9" class="num-bg"/><text x="78" y="142" class="num">2</text>

  <rect x="72" y="164" width="520" height="34" rx="2" class="toolbar"/>
  <text x="332" y="181" class="text-white-sm">&#x6A21;&#x5757;&#x5DE5;&#x5177;&#x680F;: DICOM | &#x4F53;&#x6570;&#x636E; | &#x5206;&#x5272; | &#x6807;&#x6CE8; | &#x6A21;&#x578B;</text>
  <circle cx="78" cy="181" r="9" class="num-bg"/><text x="78" y="181" class="num">3</text>

  <rect x="600" y="164" width="328" height="34" rx="2" class="toolbar"/>
  <text x="764" y="181" class="text-white-sm">&#x6570;&#x636E;&#x5DE5;&#x5177;&#x680F;: DICOM &#x5BFC;&#x5165; | &#x4FDD;&#x5B58;&#x573A;&#x666F;</text>
  <circle cx="606" cy="181" r="9" class="num-bg"/><text x="606" y="181" class="num">4</text>

  <rect x="72" y="206" width="240" height="400" rx="4" class="panel"/>
  <text x="192" y="228" class="region-label" text-anchor="middle">&#x5DE6;&#x4FA7;&#x5DE5;&#x4F5C;&#x6D41;&#x9762;&#x677F;</text>
  <text x="192" y="246" class="region-desc" text-anchor="middle">(PanelDockWidget)</text>

  <rect x="88" y="258" width="208" height="36" rx="3" fill="#4472C4" stroke="#2F5597"/>
  <text x="192" y="276" class="text-white-sm">&#x6A21;&#x5757;&#x5207;&#x6362;&#x533A;</text>

  <rect x="88" y="304" width="208" height="30" rx="3" class="module-btn"/>
  <text x="192" y="319" class="text-dark-sm">DICOM &#x5BFC;&#x5165;</text>
  <rect x="88" y="340" width="208" height="30" rx="3" class="module-btn"/>
  <text x="192" y="355" class="text-dark-sm">&#x4F53;&#x6570;&#x636E;&#x7BA1;&#x7406;</text>
  <rect x="88" y="376" width="208" height="30" rx="3" fill="#4472C4" stroke="#2F5597"/>
  <text x="192" y="391" class="text-white-sm">&#x5F71;&#x50CF;&#x5206;&#x5272; (&#x5F53;&#x524D;)</text>
  <rect x="88" y="412" width="208" height="30" rx="3" class="module-btn"/>
  <text x="192" y="427" class="text-dark-sm">&#x6D4B;&#x91CF;&#x6807;&#x6CE8;</text>
  <rect x="88" y="448" width="208" height="30" rx="3" class="module-btn"/>
  <text x="192" y="463" class="text-dark-sm">&#x4E09;&#x7EF4;&#x6A21;&#x578B;</text>

  <rect x="88" y="490" width="208" height="104" rx="3" fill="#fff" stroke="#2F5597" stroke-dasharray="4 3"/>
  <text x="192" y="530" class="text-dark-sm">&#x5F53;&#x524D;&#x6A21;&#x5757;</text>
  <text x="192" y="550" class="text-dark-sm">&#x64CD;&#x4F5C;&#x754C;&#x9762;</text>
  <text x="192" y="570" class="region-desc" text-anchor="middle">&#x968F;&#x6A21;&#x5757;&#x5207;&#x6362;&#x66F4;&#x65B0;</text>
  <circle cx="78" cy="406" r="9" class="num-bg"/><text x="78" y="406" class="num">5</text>

  <rect x="324" y="206" width="604" height="400" rx="4" class="viewport"/>
  <text x="626" y="228" class="region-label" text-anchor="middle">&#x4E2D;&#x592E;&#x89C6;&#x7A97;&#x533A;</text>
  <text x="626" y="246" class="region-desc" text-anchor="middle">&#x5E38;&#x89C4;&#x56DB;&#x89C6;&#x7A97;&#x5E03;&#x5C40; (2D &#x5207;&#x7247; + 3D &#x89C6;&#x56FE;)</text>

  <rect x="340" y="262" width="280" height="158" rx="3" class="view-cell"/>
  <text x="480" y="335" class="text-dark">&#x8F74;&#x4F4D;&#x5207;&#x7247; (Axial)</text>
  <rect x="632" y="262" width="280" height="158" rx="3" class="view-cell"/>
  <text x="772" y="335" class="text-dark">&#x77E2;&#x72B6;&#x5207;&#x7247; (Sagittal)</text>
  <rect x="340" y="432" width="280" height="158" rx="3" class="view-cell"/>
  <text x="480" y="505" class="text-dark">&#x51A0;&#x72B6;&#x5207;&#x7247; (Coronal)</text>
  <rect x="632" y="432" width="280" height="158" rx="3" class="view-cell"/>
  <text x="772" y="505" class="text-dark">3D &#x89C6;&#x56FE; (Volume Rendering)</text>
  <circle cx="330" cy="406" r="9" class="num-bg"/><text x="330" y="406" class="num">6</text>

  <rect x="72" y="618" width="856" height="52" rx="3" class="dock"/>
  <text x="500" y="644" class="region-label" text-anchor="middle">&#x5E95;&#x90E8;&#x6279;&#x6CE8;&#x9762;&#x677F; &#x2014; &#x533B;&#x751F;&#x6279;&#x6CE8;</text>
  <circle cx="78" cy="644" r="9" class="num-bg"/><text x="78" y="644" class="num">7</text>

  <rect x="72" y="676" width="856" height="22" rx="2" class="status"/>
  <text x="500" y="687" class="region-desc" text-anchor="middle">&#x72B6;&#x6001;&#x680F;</text>
  <circle cx="78" cy="687" r="9" class="num-bg"/><text x="78" y="687" class="num">8</text>

  <rect x="60" y="708" width="880" height="44" rx="4" class="legend-box"/>
  <text x="80" y="728" class="region-desc">&#x56FE;&#x4F8B;:</text>
  <rect x="120" y="718" width="14" height="14" fill="#2E75B6"/>
  <text x="142" y="728" class="region-desc">&#x6807;&#x9898;/&#x5DE5;&#x5177;&#x680F;</text>
  <rect x="250" y="718" width="14" height="14" fill="#D6E4F0" stroke="#2F5597"/>
  <text x="272" y="728" class="region-desc">&#x529F;&#x80FD;&#x9762;&#x677F;</text>
  <rect x="360" y="718" width="14" height="14" fill="#EEF3F8" stroke="#2F5597"/>
  <text x="382" y="728" class="region-desc">&#x5F71;&#x50CF;&#x89C6;&#x7A97;</text>
  <rect x="470" y="718" width="14" height="14" fill="#BDD7EE" stroke="#2F5597"/>
  <text x="492" y="728" class="region-desc">&#x8F85;&#x52A9;&#x9762;&#x677F;</text>
  <circle cx="580" cy="725" r="7" class="num-bg"/><text x="580" y="725" class="num" font-size="9">n</text>
  <text x="596" y="728" class="region-desc">&#x533A;&#x57DF;&#x7F16;&#x53F7;</text>

  <text x="960" y="102" class="callout-text" text-anchor="end">1 &#x6807;&#x9898;&#x680F;</text>
  <text x="960" y="142" class="callout-text" text-anchor="end">2 &#x83DC;&#x5355;&#x680F;</text>
  <text x="960" y="181" class="callout-text" text-anchor="end">3 &#x6A21;&#x5757;&#x5DE5;&#x5177;&#x680F;</text>
  <text x="960" y="196" class="callout-text" text-anchor="end">4 &#x6570;&#x636E;&#x5DE5;&#x5177;&#x680F;</text>
  <text x="960" y="406" class="callout-text" text-anchor="end">5 &#x5DE6;&#x4FA7;&#x9762;&#x677F;</text>
  <text x="960" y="421" class="callout-text" text-anchor="end">6 &#x4E2D;&#x592E;&#x89C6;&#x7A97;</text>
  <text x="960" y="644" class="callout-text" text-anchor="end">7 &#x533B;&#x751F;&#x6279;&#x6CE8;</text>
  <text x="960" y="687" class="callout-text" text-anchor="end">8 &#x72B6;&#x6001;&#x680F;</text>
</svg>
"""


def main() -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(CONTENT, encoding="utf-8", newline="\n")
    ET.parse(OUTPUT)
    print(f"OK: {OUTPUT} ({OUTPUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
