#!/usr/bin/env python3
"""
PlexChem Font & Icon Baker for TI-84 Plus CE (320x240, 8bpp paletted LCD)
------------------------------------------------------------------------
Bakes crisp 2-bit anti-aliased proportional bitmap fonts (Inter + STIX Two /
TeX Gyre Termes Math) and minimalist 16x16 UI icons directly into Flash ROM
C headers (src/fonts_baked.h and src/icons_baked.h).

Usage:
    python3 tools/bake_fonts.py

If STIXTwoMath-Regular.otf / STIXTwoText-Italic.otf / Inter-*.otf are placed
in ./fonts/, they are used automatically; otherwise system-installed Inter
and TeX Gyre Termes Math / Nimbus Roman fonts are used seamlessly.
"""

import os
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
FONTS_DIR = ROOT / "fonts"
SRC_DIR.mkdir(parents=True, exist_ok=True)

# Custom single-byte glyph encoding (0x20..0x9F) for zero-overhead C rendering
# 0x20..0x7E: Standard ASCII
# 0x80..0x97: Chemistry & Math symbols (true glyphs, never ASCII ^ hacks!)
EXTRA_GLYPHS = {
    0x80: "\u0394",  # Δ Delta
    0x81: "\u2192",  # → Reaction arrow
    0x82: "\u21CC",  # ⇌ Equilibrium double harpoon
    0x83: "\u00D7",  # × Multiplication
    0x84: "\u00B7",  # · Middle dot (L·s)
    0x85: "\u2212",  # − True minus sign
    0x86: "\u2248",  # ≈ Approximately equal
    0x87: "\u00B0",  # ° Degree (°C)
    0x88: "\u2264",  # ≤ Less than or equal
    0x89: "\u2265",  # ≥ Greater than or equal
    0x8A: "\u221E",  # ∞ Infinity
    0x8B: "\u2032",  # ′ Prime (k′)
    0x8C: "\u03BC",  # μ Micro (μmol)
    0x8D: "\u2191",  # ↑ Up arrow
    0x8E: "\u2193",  # ↓ Down arrow
    0x8F: "\u2260",  # ≠ Not equal
    0x90: "\u221D",  # ∝ Proportional to
}

GLYPH_CODES = list(range(0x20, 0x7F)) + sorted(EXTRA_GLYPHS.keys())


def find_font(candidates):
    for p in candidates:
        if os.path.exists(p):
            return p
    raise FileNotFoundError(f"None of the candidate fonts found: {candidates}")


FONT_INTER_REG = find_font([
    str(FONTS_DIR / "Inter-Regular.otf"),
    str(FONTS_DIR / "Inter-Regular.ttf"),
    "/usr/share/fonts/opentype/inter/Inter-Regular.otf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
])

FONT_INTER_SEMI = find_font([
    str(FONTS_DIR / "Inter-SemiBold.otf"),
    str(FONTS_DIR / "Inter-SemiBold.ttf"),
    str(FONTS_DIR / "Inter-Bold.otf"),
    "/usr/share/fonts/opentype/inter/Inter-SemiBold.otf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
])

FONT_MATH_REG = find_font([
    str(FONTS_DIR / "STIXTwoMath-Regular.otf"),
    str(FONTS_DIR / "STIXTwoText-Regular.otf"),
    "/usr/share/texmf/fonts/opentype/public/tex-gyre-math/texgyretermes-math.otf",
    "/usr/share/fonts/truetype/liberation2/LiberationSerif-Regular.ttf",
])

FONT_MATH_ITAL = find_font([
    str(FONTS_DIR / "STIXTwoText-Italic.otf"),
    str(FONTS_DIR / "STIXTwoText-Italic.ttf"),
    "/usr/share/fonts/opentype/urw-base35/NimbusRoman-Italic.otf",
    "/usr/share/fonts/truetype/liberation2/LiberationSerif-Italic.ttf",
])

FONT_MATH_SYM = find_font([
    str(FONTS_DIR / "STIXTwoMath-Regular.otf"),
    "/usr/share/texmf/fonts/opentype/public/tex-gyre-math/texgyretermes-math.otf",
    "/usr/share/fonts/truetype/dejavu/DejaVuMathTeXGyre.ttf",
])


def rasterize_font(name, font_path, size_px, sym_fallback=None):
    """Rasterize glyphs at 2-bit alpha (4 levels: 0..3) packed 4 pixels/byte."""
    font = ImageFont.truetype(font_path, size_px)
    sym_font = ImageFont.truetype(sym_fallback, size_px) if sym_fallback else font
    ascent, descent = font.getmetrics()
    line_height = ascent + descent

    glyph_meta = []
    bitmap_bytes = bytearray()

    for code in GLYPH_CODES:
        ch = chr(code) if code < 0x80 else EXTRA_GLYPHS[code]
        use_font = sym_font if code >= 0x80 else font

        # Render onto oversampled canvas then downsample for clean 2-bit AA
        pad = 6
        canvas_w = size_px * 3 + pad * 2
        canvas_h = line_height + pad * 2
        img = Image.new("L", (canvas_w, canvas_h), 0)
        draw = ImageDraw.Draw(img)

        try:
            adv = int(round(use_font.getlength(ch)))
        except Exception:
            adv = size_px // 2
        adv = max(adv, 2)

        draw.text((pad, pad), ch, fill=255, font=use_font)
        bbox = img.getbbox()
        if bbox is None or ch == " ":
            glyph_meta.append((code, 0, 0, 0, 0, adv, 0))
            continue

        x0, y0, x1, y1 = bbox
        w = x1 - x0
        h = y1 - y0
        x_off = x0 - pad
        y_off = y0 - pad

        offset = len(bitmap_bytes)
        cropped = img.crop((x0, y0, x1, y1))
        px = cropped.load()

        # Pack 4 pixels per byte (2 bits/pixel: 0=bg, 1=33%, 2=67%, 3=100%)
        for yy in range(h):
            byte_val = 0
            bit_pos = 6
            for xx in range(w):
                v = px[xx, yy]
                if v >= 200:
                    q = 3
                elif v >= 120:
                    q = 2
                elif v >= 45:
                    q = 1
                else:
                    q = 0
                byte_val |= (q << bit_pos)
                if bit_pos == 0:
                    bitmap_bytes.append(byte_val)
                    byte_val = 0
                    bit_pos = 6
                else:
                    bit_pos -= 2
            if bit_pos != 6:
                bitmap_bytes.append(byte_val)

        glyph_meta.append((code, w, h, x_off, y_off, adv, offset))

    return {
        "name": name,
        "size": size_px,
        "ascent": ascent,
        "line_height": line_height,
        "meta": glyph_meta,
        "data": bytes(bitmap_bytes),
    }


def write_fonts_header(fonts_list):
    out = SRC_DIR / "fonts_baked.h"
    lines = [
        "/* Auto-generated by tools/bake_fonts.py — DO NOT EDIT BY HAND */",
        "/* Stored in Flash ROM (const) — 0 bytes of TI-84 Plus CE user RAM! */",
        "#ifndef PLEXCHEM_FONTS_BAKED_H",
        "#define PLEXCHEM_FONTS_BAKED_H",
        "#include <stdint.h>",
        "",
        "/* Custom 1-byte chemistry & math glyph codes (0x80..0x90) */",
        "#define SYM_DELTA   \"\\x80\"  /* Δ */",
        "#define SYM_ARROW   \"\\x81\"  /* → */",
        "#define SYM_EQUIL   \"\\x82\"  /* ⇌ */",
        "#define SYM_TIMES   \"\\x83\"  /* × */",
        "#define SYM_DOT     \"\\x84\"  /* · */",
        "#define SYM_MINUS   \"\\x85\"  /* − */",
        "#define SYM_APPROX  \"\\x86\"  /* ≈ */",
        "#define SYM_DEG     \"\\x87\"  /* ° */",
        "#define SYM_LEQ     \"\\x88\"  /* ≤ */",
        "#define SYM_GEQ     \"\\x89\"  /* ≥ */",
        "#define SYM_INF     \"\\x8A\"  /* ∞ */",
        "#define SYM_PRIME   \"\\x8B\"  /* ′ */",
        "#define SYM_MU      \"\\x8C\"  /* μ */",
        "#define SYM_UP      \"\\x8D\"  /* ↑ */",
        "#define SYM_DOWN    \"\\x8E\"  /* ↓ */",
        "#define SYM_NEQ     \"\\x8F\"  /* ≠ */",
        "#define SYM_PROP    \"\\x90\"  /* ∝ */",
        "",
        "typedef struct {",
        "    uint8_t w;",
        "    uint8_t h;",
        "    int8_t  xoff;",
        "    int8_t  yoff;",
        "    uint8_t adv;",
        "    uint16_t offset;",
        "} GlyphMetric;",
        "",
        "typedef struct {",
        "    uint8_t ascent;",
        "    uint8_t line_height;",
        "    const GlyphMetric *metrics; /* indexed 0..(0x90-0x20) */",
        "    const uint8_t *bitmap;",
        "} BakedFont;",
        "",
    ]

    total_bytes = 0
    for f in fonts_list:
        name = f["name"]
        # Build dense table 0x20..0x90 (113 entries)
        by_code = {m[0]: m for m in f["meta"]}
        lines.append(f"static const uint8_t {name}_bmp[{len(f['data'])}] = {{")
        for i in range(0, len(f["data"]), 16):
            chunk = ", ".join(f"0x{b:02X}" for b in f["data"][i : i + 16])
            lines.append(f"    {chunk},")
        lines.append("};")
        lines.append(f"static const GlyphMetric {name}_met[113] = {{")
        for code in range(0x20, 0x91):
            if code in by_code:
                _, w, h, xo, yo, adv, off = by_code[code]
                lines.append(f"    {{{w}, {h}, {xo}, {yo}, {adv}, {off}}}, /* 0x{code:02X} */")
            else:
                lines.append(f"    {{0, 0, 0, 0, 4, 0}}, /* 0x{code:02X} unused */")
        lines.append("};")
        lines.append(
            f"static const BakedFont {name} = {{{f['ascent']}, {f['line_height']}, {name}_met, {name}_bmp}};"
        )
        lines.append("")
        total_bytes += len(f["data"]) + 113 * 7

    lines.append("#endif /* PLEXCHEM_FONTS_BAKED_H */")
    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"Baked {len(fonts_list)} fonts -> {out} ({total_bytes:,} bytes Flash ROM)")


def draw_icon_aa(draw_fn, size=18):
    """Draw a minimalist vector icon at 4x supersampling -> 2-bit alpha bitmap."""
    S = size * 4
    img = Image.new("L", (S, S), 0)
    d = ImageDraw.Draw(img)
    draw_fn(d, S)
    small = img.resize((size, size), Image.Resampling.LANCZOS)
    px = small.load()
    data = bytearray()
    for y in range(size):
        b = 0
        bp = 6
        for x in range(size):
            v = px[x, y]
            q = 3 if v >= 190 else (2 if v >= 110 else (1 if v >= 40 else 0))
            b |= (q << bp)
            if bp == 0:
                data.append(b)
                b = 0
                bp = 6
            else:
                bp -= 2
        if bp != 6:
            data.append(b)
    return bytes(data)


def icon_rates(d, S):
    # Minimalist stopwatch / rate-slope icon
    d.ellipse([S*0.18, S*0.24, S*0.82, S*0.88], outline=255, width=6)
    d.line([S*0.40, S*0.12, S*0.60, S*0.12], fill=255, width=6)
    d.line([S*0.50, S*0.12, S*0.50, S*0.24], fill=255, width=6)
    d.line([S*0.50, S*0.56, S*0.50, S*0.36], fill=255, width=6)
    d.line([S*0.50, S*0.56, S*0.68, S*0.62], fill=255, width=6)


def icon_table(d, S):
    # Minimalist experimental data grid icon
    d.rounded_rectangle([S*0.14, S*0.18, S*0.86, S*0.84], radius=8, outline=255, width=6)
    d.line([S*0.14, S*0.38, S*0.86, S*0.38], fill=255, width=6)
    d.line([S*0.14, S*0.60, S*0.86, S*0.60], fill=255, width=5)
    d.line([S*0.42, S*0.38, S*0.42, S*0.84], fill=255, width=5)
    d.line([S*0.66, S*0.38, S*0.66, S*0.84], fill=255, width=5)


def icon_graph(d, S):
    # Minimalist linear plot + scatter dots icon
    d.line([S*0.16, S*0.16, S*0.16, S*0.84, S*0.86, S*0.84], fill=255, width=6)
    d.line([S*0.26, S*0.72, S*0.80, S*0.24], fill=255, width=6)
    for cx, cy in [(0.32, 0.66), (0.52, 0.49), (0.74, 0.30)]:
        r = 6
        d.ellipse([S*cx-r, S*cy-r, S*cx+r, S*cy+r], fill=255)


def icon_mech(d, S):
    # Minimalist multi-step mechanism pathway icon
    r = 9
    d.ellipse([S*0.12, S*0.22, S*0.12+2*r, S*0.22+2*r], outline=255, width=6)
    d.ellipse([S*0.66, S*0.22, S*0.66+2*r, S*0.22+2*r], outline=255, width=6)
    d.ellipse([S*0.39, S*0.62, S*0.39+2*r, S*0.62+2*r], outline=255, width=6)
    d.line([S*0.36, S*0.34, S*0.62, S*0.34], fill=255, width=5)
    d.line([S*0.54, S*0.27, S*0.62, S*0.34, S*0.54, S*0.41], fill=255, width=5)
    d.line([S*0.28, S*0.46, S*0.44, S*0.64], fill=255, width=5)
    d.line([S*0.72, S*0.46, S*0.56, S*0.64], fill=255, width=5)


def icon_arrh(d, S):
    # Minimalist activation energy barrier hump icon
    d.line([S*0.14, S*0.82, S*0.86, S*0.82], fill=255, width=5)
    pts = [
        (S*0.14, S*0.64),
        (S*0.26, S*0.64),
        (S*0.38, S*0.36),
        (S*0.50, S*0.20),
        (S*0.62, S*0.38),
        (S*0.74, S*0.72),
        (S*0.86, S*0.72),
    ]
    d.line(pts, fill=255, width=6, joint="curve")
    # Dashed Ea vertical indicator
    d.line([S*0.50, S*0.26, S*0.50, S*0.42], fill=255, width=4)
    d.line([S*0.50, S*0.50, S*0.50, S*0.64], fill=255, width=4)


def icon_ref(d, S):
    # Minimalist formula chart / book icon
    d.rounded_rectangle([S*0.18, S*0.14, S*0.82, S*0.86], radius=7, outline=255, width=6)
    d.line([S*0.32, S*0.34, S*0.68, S*0.34], fill=255, width=5)
    d.line([S*0.32, S*0.50, S*0.68, S*0.50], fill=255, width=5)
    d.line([S*0.32, S*0.66, S*0.56, S*0.66], fill=255, width=5)


def write_icons_header():
    icons = [
        ("ICON_RATES", icon_rates),
        ("ICON_TABLE", icon_table),
        ("ICON_GRAPH", icon_graph),
        ("ICON_MECH", icon_mech),
        ("ICON_ARRH", icon_arrh),
        ("ICON_REF", icon_ref),
    ]
    size = 18
    out = SRC_DIR / "icons_baked.h"
    lines = [
        "/* Auto-generated by tools/bake_fonts.py — 18x18 2-bit AA minimalist icons */",
        "#ifndef PLEXCHEM_ICONS_BAKED_H",
        "#define PLEXCHEM_ICONS_BAKED_H",
        "#include <stdint.h>",
        f"#define ICON_SIZE {size}",
        "",
    ]
    for idx, (name, fn) in enumerate(icons):
        bmp = draw_icon_aa(fn, size=size)
        lines.append(f"static const uint8_t {name}_bmp[{len(bmp)}] = {{")
        for i in range(0, len(bmp), 16):
            chunk = ", ".join(f"0x{b:02X}" for b in bmp[i : i + 16])
            lines.append(f"    {chunk},")
        lines.append("};")
    lines.append("")
    lines.append("static const uint8_t * const ALL_ICONS[6] = {")
    for name, _ in icons:
        lines.append(f"    {name}_bmp,")
    lines.append("};")
    lines.append("#endif /* PLEXCHEM_ICONS_BAKED_H */")
    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"Baked {len(icons)} minimalist 18x18 icons -> {out}")


def main():
    print("Font sources:")
    print("  Inter Regular :", FONT_INTER_REG)
    print("  Inter SemiBold:", FONT_INTER_SEMI)
    print("  Math Serif    :", FONT_MATH_REG)
    print("  Math Italic   :", FONT_MATH_ITAL)
    fonts = [
        rasterize_font("FONT_UI_11", FONT_INTER_REG, 11, FONT_MATH_SYM),
        rasterize_font("FONT_UI_11B", FONT_INTER_SEMI, 11, FONT_MATH_SYM),
        rasterize_font("FONT_UI_13B", FONT_INTER_SEMI, 13, FONT_MATH_SYM),
        rasterize_font("FONT_MATH_14", FONT_MATH_REG, 14, FONT_MATH_SYM),
        rasterize_font("FONT_MATH_14I", FONT_MATH_ITAL, 14, FONT_MATH_SYM),
        rasterize_font("FONT_MATH_20", FONT_MATH_REG, 20, FONT_MATH_SYM),
        rasterize_font("FONT_MATH_20I", FONT_MATH_ITAL, 20, FONT_MATH_SYM),
        rasterize_font("FONT_MATH_10", FONT_MATH_REG, 10, FONT_MATH_SYM),
        rasterize_font("FONT_MATH_10I", FONT_MATH_ITAL, 10, FONT_MATH_SYM),
    ]
    write_fonts_header(fonts)
    write_icons_header()


if __name__ == "__main__":
    main()
