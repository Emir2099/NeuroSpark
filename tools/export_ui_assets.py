#!/usr/bin/env python3
"""Export UI icon atlas and 12x18 font assets into C headers.

- With Pillow + IBM Plex Mono (downloaded to assets/ui/): crisp TTF-rasterized fonts.
- Without Pillow: reads kernel/font.h and upscales 8x8 → 12x18 (legacy path).

Icons: 96x144 RGBA assets/ui/icon_atlas_96x144.png (2 columns x 3 rows of 48px tiles).
If the PNG is missing or invalid, builds the bitmap in Python and writes the PNG
via stdlib (zlib). Embeds the same pixels into assets/ui/icon_atlas_96x96.h.
"""

from __future__ import annotations

import ast
import math
import os
import re
import struct
import sys
import urllib.error
import urllib.request
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FONT_H = ROOT / "kernel" / "font.h"
ASSET_DIR = ROOT / "assets" / "ui"
ICON_HEADER = ASSET_DIR / "icon_atlas_96x96.h"
FONT_HEADER = ASSET_DIR / "font12x18.h"
IBM_PLEX_MONO_URL = (
    "https://github.com/google/fonts/raw/main/ofl/ibmplexmono/IBMPlexMono-Regular.ttf"
)
IBM_PLEX_TTF = ASSET_DIR / "IBMPlexMono-Regular.ttf"
ICON_ATLAS_PNG = ASSET_DIR / "icon_atlas_96x144.png"
ICON_ATLAS_PNG_96 = ASSET_DIR / "icon_atlas_96x96.png"

W = 96
H = 144
TILE = 48

P_ACCENT_CYAN = 0x00FFFF

try:
    from PIL import Image, ImageDraw, ImageFont

    HAS_PIL = True
except ImportError:
    HAS_PIL = False


def ensure_download(url: str, dest: Path) -> None:
    if dest.exists() and dest.stat().st_size > 1000:
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {dest.name} ...")
    urllib.request.urlretrieve(url, dest)


def line_put(buf: list[int], x: int, y: int, color: int) -> None:
    if 0 <= x < W and 0 <= y < H:
        buf[y * W + x] = color


def line_draw(buf: list[int], x0: int, y0: int, x1: int, y1: int, color: int) -> None:
    dx = x1 - x0
    dy = y1 - y0
    sx = 1 if dx >= 0 else -1
    sy = 1 if dy >= 0 else -1
    adx = dx if dx >= 0 else -dx
    ady = dy if dy >= 0 else -dy
    err = (adx if adx > ady else -ady) // 2

    while True:
        line_put(buf, x0, y0, color)
        if x0 == x1 and y0 == y1:
            break
        e2 = err
        if e2 > -adx:
            err -= ady
            x0 += sx
        if e2 < ady:
            err += adx
            y0 += sy


def build_icon_atlas_legacy() -> list[int]:
    """Stdlib line-art; tile layout matches dashboard.c (tile_row, tile_col)."""
    atlas = [0] * (W * H)

    def oxoy(tr: int, tc: int) -> tuple[int, int]:
        return (tc * TILE, tr * TILE)

    # (0,0) Power — lightning
    line_draw(atlas, 16, 10, 20, 20, P_ACCENT_CYAN)
    line_draw(atlas, 20, 20, 12, 25, P_ACCENT_CYAN)
    line_draw(atlas, 12, 25, 20, 38, P_ACCENT_CYAN)
    line_draw(atlas, 20, 38, 24, 20, P_ACCENT_CYAN)
    line_draw(atlas, 17, 11, 19, 19, 0x33E8FF)
    line_draw(atlas, 19, 21, 13, 24, 0x33E8FF)
    line_draw(atlas, 13, 24, 19, 37, 0x33E8FF)

    # (0,1) Viz — vertical bars (top-right tile)
    o0, o1 = oxoy(0, 1)
    base = o1 + 38
    heights = (12, 22, 16, 28, 18)
    for i, h in enumerate(heights):
        bx = o0 + 6 + i * 8
        for yy in range(base - h, base + 1):
            for xx in range(bx, bx + 6):
                line_put(atlas, xx, yy, 0x00CCFF if i % 2 == 0 else 0x33E8FF)

    # (1,0) Shell — hollow frame + prompt (middle-left; not a 3x3 grid)
    o0, o1 = oxoy(1, 0)
    for x in range(o0 + 6, o0 + 42):
        line_put(atlas, x, o1 + 8, P_ACCENT_CYAN)
        line_put(atlas, x, o1 + 40, P_ACCENT_CYAN)
    for y in range(o1 + 8, o1 + 41):
        line_put(atlas, o0 + 6, y, P_ACCENT_CYAN)
        line_put(atlas, o0 + 41, y, P_ACCENT_CYAN)
    line_draw(atlas, o0 + 10, o1 + 26, o0 + 14, o1 + 30, 0x33E8FF)
    line_draw(atlas, o0 + 10, o1 + 30, o0 + 14, o1 + 26, 0x33E8FF)
    for x in range(o0 + 18, o0 + 32):
        line_put(atlas, x, o1 + 31, P_ACCENT_CYAN)

    # (1,1) System — gear / sunburst
    cx = 72
    cy = 72
    for angle_i in range(8):
        if angle_i % 2 == 0:
            line_draw(
                atlas,
                cx,
                cy,
                cx + (8 if angle_i < 4 else -8),
                cy + (0 if angle_i % 4 < 2 else 0),
                P_ACCENT_CYAN,
            )
        else:
            line_draw(
                atlas,
                cx,
                cy,
                cx + (6 if angle_i < 4 else -6),
                cy + (6 if angle_i % 4 < 2 else -6),
                P_ACCENT_CYAN,
            )
    for y in range(cy - 5, cy + 6):
        for x in range(cx - 5, cx + 6):
            dx = x - cx
            dy = y - cy
            if dx * dx + dy * dy < 20:
                line_put(atlas, x, y, P_ACCENT_CYAN)

    # (2,0) Apps — 3x3 grid (bottom-left)
    o0, o1 = oxoy(2, 0)
    for gy in range(3):
        for gx in range(3):
            px = o0 + 10 + gx * 12
            py = o1 + 10 + gy * 12
            for dy in range(8):
                for dx in range(8):
                    line_put(atlas, px + dx, py + dy, P_ACCENT_CYAN)

    return atlas


def build_icon_atlas_pil() -> list[int]:
    """Vector-style icons: distinct shapes per tile (no duplicate grid vs terminal).

    Tile layout matches dashboard.c draw_icon_from_atlas(tile_row, tile_col):
      (0,0) Power   | (0,1) Viz
      (1,0) Shell   | (1,1) System
      (2,0) Apps    | (unused)
    """
    if not HAS_PIL:
        raise RuntimeError("build_icon_atlas_pil requires Pillow")

    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    dr = ImageDraw.Draw(img)
    cyan = (0x00, 0xE8, 0xFF, 255)
    cyan_hi = (0x66, 0xFF, 0xFF, 255)
    dim = (0x00, 0x66, 0x88, 255)
    panel = (0x08, 0x14, 0x22, 240)

    def tile_origin(tr: int, tc: int) -> tuple[int, int]:
        return (tc * TILE, tr * TILE)

    def to_global(tr: int, tc: int, lx: int, ly: int) -> tuple[int, int]:
        x0, y0 = tile_origin(tr, tc)
        return (x0 + lx, y0 + ly)

    # (0,0) Power — lightning bolt (filled)
    tr, tc = 0, 0
    bolt = [
        to_global(tr, tc, 24, 7),
        to_global(tr, tc, 15, 22),
        to_global(tr, tc, 22, 22),
        to_global(tr, tc, 17, 41),
        to_global(tr, tc, 33, 19),
        to_global(tr, tc, 26, 19),
        to_global(tr, tc, 31, 7),
    ]
    dr.polygon(bolt, fill=cyan_hi, outline=cyan)

    # (0,1) Viz — bar chart / scope trace (top-right tile)
    tr, tc = 0, 1
    x0, y0 = tile_origin(tr, tc)
    base = y0 + 38
    heights = (12, 22, 16, 28, 18)
    for i, h in enumerate(heights):
        bx = x0 + 6 + i * 8
        dr.rectangle(
            (bx, base - h, bx + 5, base),
            fill=cyan if i % 2 == 0 else cyan_hi,
            outline=dim,
        )

    # (1,0) Shell — terminal window with prompt (middle-left; not a 3x3 grid)
    tr, tc = 1, 0
    x0, y0 = tile_origin(tr, tc)
    shell_rect = (x0 + 5, y0 + 6, x0 + 43, y0 + 42)
    rr = getattr(dr, "rounded_rectangle", None)
    if rr:
        rr(shell_rect, radius=4, fill=panel, outline=cyan, width=2)
    else:
        dr.rectangle(shell_rect, fill=panel, outline=cyan, width=2)
    dr.line(
        [(x0 + 9, y0 + 14), (x0 + 39, y0 + 14)],
        fill=dim,
        width=1,
    )
    dr.line([(x0 + 10, y0 + 28), (x0 + 16, y0 + 34)], fill=cyan_hi, width=2)
    dr.line([(x0 + 10, y0 + 34), (x0 + 16, y0 + 28)], fill=cyan_hi, width=2)
    dr.line([(x0 + 20, y0 + 33), (x0 + 32, y0 + 33)], fill=cyan, width=2)

    # (1,1) System — gear
    tr, tc = 1, 1
    cx, cy = to_global(tr, tc, 24, 24)
    dr.ellipse((cx - 14, cy - 14, cx + 14, cy + 14), outline=cyan, width=3)
    dr.ellipse((cx - 7, cy - 7, cx + 7, cy + 7), fill=panel, outline=cyan_hi)
    for a in range(8):
        ang = (a * math.pi * 2) / 8
        x1 = cx + int(17 * math.cos(ang))
        y1 = cy + int(17 * math.sin(ang))
        x2 = cx + int(22 * math.cos(ang))
        y2 = cy + int(22 * math.sin(ang))
        dr.line([(x1, y1), (x2, y2)], fill=cyan_hi, width=3)

    # (2,0) Apps — 3x3 dot grid (bottom-left)
    tr, tc = 2, 0
    x0, y0 = tile_origin(tr, tc)
    for gr in range(3):
        for gc in range(3):
            cx = x0 + 12 + gc * 12
            cy = y0 + 12 + gr * 12
            dr.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=cyan, outline=cyan_hi)

    out = [0] * (W * H)
    pix = img.load()
    for yy in range(H):
        for xx in range(W):
            r, g, b, a = pix[xx, yy]
            out[yy * W + xx] = _rgb_to_pixel(r, g, b, a)
    return out


def _rgb_to_pixel(r: int, g: int, b: int, a: int) -> int:
    if a < 8:
        return 0
    return (r << 16) | (g << 8) | b


def _expand_96x96_to_96x144(top96: list[int]) -> list[int]:
    """Expand a 96x96 atlas into 96x144; synthesize the last row from legacy art."""
    out = [0] * (W * H)
    # Copy first 96 rows verbatim.
    for y in range(96):
        src = y * W
        out[src : src + W] = top96[src : src + W]

    # Fill row 2 tiles from legacy atlas so app/system slots still render.
    legacy = build_icon_atlas_legacy()
    for y in range(96, H):
        src = y * W
        out[src : src + W] = legacy[src : src + W]
    return out


def _resize_atlas_nearest(src: list[int], src_w: int, src_h: int, dst_w: int, dst_h: int) -> list[int]:
    out = [0] * (dst_w * dst_h)
    for y in range(dst_h):
        sy = (y * src_h) // dst_h
        if sy >= src_h:
            sy = src_h - 1
        for x in range(dst_w):
            sx = (x * src_w) // dst_w
            if sx >= src_w:
                sx = src_w - 1
            out[y * dst_w + x] = src[sy * src_w + sx]
    return out


def _scale_box_nearest(
    src: list[int],
    src_w: int,
    src_h: int,
    x0: int,
    y0: int,
    bw: int,
    bh: int,
    dst_w: int,
    dst_h: int,
) -> list[int]:
    out = [0] * (dst_w * dst_h)
    if bw <= 0 or bh <= 0:
        return out
    for y in range(dst_h):
        sy = y0 + (y * bh) // dst_h
        if sy >= src_h:
            sy = src_h - 1
        for x in range(dst_w):
            sx = x0 + (x * bw) // dst_w
            if sx >= src_w:
                sx = src_w - 1
            out[y * dst_w + x] = src[sy * src_w + sx]
    return out


def _extract_sheet_icon_boxes(sheet: list[int], sw: int, sh: int) -> list[tuple[int, int, int, int, int]]:
    """Return connected-component icon boxes as (area, x0, y0, w, h)."""
    total = sw * sh
    seen = bytearray(total)
    boxes: list[tuple[int, int, int, int, int]] = []

    for y in range(sh):
        row = y * sw
        for x in range(sw):
            idx = row + x
            if seen[idx] or sheet[idx] == 0:
                continue

            stack = [idx]
            seen[idx] = 1
            area = 0
            minx = maxx = x
            miny = maxy = y

            while stack:
                cur = stack.pop()
                cy = cur // sw
                cx = cur - (cy * sw)
                area += 1
                if cx < minx:
                    minx = cx
                if cx > maxx:
                    maxx = cx
                if cy < miny:
                    miny = cy
                if cy > maxy:
                    maxy = cy

                if cx > 0:
                    n = cur - 1
                    if not seen[n] and sheet[n] != 0:
                        seen[n] = 1
                        stack.append(n)
                if cx + 1 < sw:
                    n = cur + 1
                    if not seen[n] and sheet[n] != 0:
                        seen[n] = 1
                        stack.append(n)
                if cy > 0:
                    n = cur - sw
                    if not seen[n] and sheet[n] != 0:
                        seen[n] = 1
                        stack.append(n)
                if cy + 1 < sh:
                    n = cur + sw
                    if not seen[n] and sheet[n] != 0:
                        seen[n] = 1
                        stack.append(n)

            bw = maxx - minx + 1
            bh = maxy - miny + 1
            if area < 200:
                continue
            if bw < 16 or bh < 16:
                continue
            if bw > 1024 or bh > 1024:
                continue
            boxes.append((area, minx, miny, bw, bh))

    boxes.sort(key=lambda t: t[0], reverse=True)
    return boxes


def _build_atlas_from_sheet_components(sheet: list[int], sw: int, sh: int) -> list[int] | None:
    """Auto-pick six largest icon-like components from a stitched sheet."""
    boxes = _extract_sheet_icon_boxes(sheet, sw, sh)
    if len(boxes) < 6:
        return None

    # Keep top candidates by area and stable ordering.
    candidates = boxes[:128]
    candidates.sort(key=lambda t: (t[2], t[1]))
    chosen = candidates[:6]

    atlas = [0] * (W * H)
    slots = [(0, 0), (0, 1), (1, 0), (1, 1), (2, 0), (2, 1)]
    inner = 42  # 3px padding inside each 48x48 tile

    for i, (_, x0, y0, bw, bh) in enumerate(chosen):
        tile = _scale_box_nearest(sheet, sw, sh, x0, y0, bw, bh, inner, inner)
        tr, tc = slots[i]
        ox = tc * TILE + 3
        oy = tr * TILE + 3
        for yy in range(inner):
            dst = (oy + yy) * W + ox
            src = yy * inner
            atlas[dst : dst + inner] = tile[src : src + inner]

    return atlas


def load_icon_atlas_from_png(path: Path) -> list[int]:
    if HAS_PIL:
        img = Image.open(path).convert("RGBA")
        if img.size == (W, H):
            out = [0] * (W * H)
            pix = img.load()
            for y in range(H):
                for x in range(W):
                    r, g, b, a = pix[x, y]
                    out[y * W + x] = _rgb_to_pixel(r, g, b, a)
            return out
        if img.size == (W, 96):
            top = [0] * (W * 96)
            pix = img.load()
            for y in range(96):
                for x in range(W):
                    r, g, b, a = pix[x, y]
                    top[y * W + x] = _rgb_to_pixel(r, g, b, a)
            return _expand_96x96_to_96x144(top)
        # Accept high-resolution proportional atlases and normalize to 96x144.
        # This supports user-authored masters like 1528x2292 (same 2:3 ratio).
        if img.size[0] > 0 and img.size[1] > 0:
            lhs = img.size[0] * H
            rhs = img.size[1] * W
            if abs(lhs - rhs) <= max(lhs, rhs) // 100:  # ~1% aspect tolerance
                img = img.resize((W, H), Image.Resampling.LANCZOS)
                out = [0] * (W * H)
                pix = img.load()
                for y in range(H):
                    for x in range(W):
                        r, g, b, a = pix[x, y]
                        out[y * W + x] = _rgb_to_pixel(r, g, b, a)
                return out
        raise ValueError(f"icon atlas must be {W}x{H} or {W}x96, got {img.size}")
    return load_icon_atlas_from_png_stdlib(path)


def load_icon_atlas_from_png_stdlib(path: Path) -> list[int]:
    """RGBA 8-bit PNG reader (no Pillow)."""
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    width = height = None
    idat = bytearray()
    while pos < len(data):
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        ctype = data[pos + 4 : pos + 8]
        chunk = data[pos + 8 : pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            width, height = struct.unpack(">II", chunk[0:8])
        elif ctype == b"IDAT":
            idat.extend(chunk)
        elif ctype == b"IEND":
            break
    if width is None or height is None or width <= 0 or height <= 0:
        raise ValueError("invalid PNG dimensions")
    raw = zlib.decompress(bytes(idat))
    bpp = 4
    stride = width * bpp
    out_h = height
    out = [0] * (width * out_h)
    i = 0
    prev = bytearray(stride)
    for y in range(height):
        f = raw[i]
        i += 1
        row = bytearray(raw[i : i + stride])
        i += stride
        if f == 0:
            pass
        elif f == 1:
            for x in range(stride):
                left = row[x - bpp] if x >= bpp else 0
                row[x] = (row[x] + left) & 0xFF
        elif f == 2:
            for x in range(stride):
                row[x] = (row[x] + prev[x]) & 0xFF
        elif f == 3:
            for x in range(stride):
                left = row[x - bpp] if x >= bpp else 0
                up = prev[x]
                row[x] = (row[x] + ((left + up) // 2)) & 0xFF
        elif f == 4:
            def paeth(a: int, b: int, c: int) -> int:
                p = a + b - c
                pa = abs(p - a)
                pb = abs(p - b)
                pc = abs(p - c)
                if pa <= pb and pa <= pc:
                    return a
                if pb <= pc:
                    return b
                return c

            for x in range(stride):
                left = row[x - bpp] if x >= bpp else 0
                up = prev[x]
                ul = prev[x - bpp] if x >= bpp else 0
                row[x] = (row[x] + paeth(left, up, ul)) & 0xFF
        prev = row
        for x in range(width):
            o = x * 4
            r, g, b, a = row[o], row[o + 1], row[o + 2], row[o + 3]
            out[y * width + x] = _rgb_to_pixel(r, g, b, a)

    if width == W and height == 96:
        return _expand_96x96_to_96x144(out)
    if width == W and height == H:
        return out

    # Accept proportional high-res atlases in stdlib mode (no Pillow available).
    lhs = width * H
    rhs = height * W
    if abs(lhs - rhs) <= max(lhs, rhs) // 100:  # ~1% aspect tolerance
        return _resize_atlas_nearest(out, width, height, W, H)

    # Last resort: treat as stitched sprite sheet and auto-compose 6 icons.
    stitched = _build_atlas_from_sheet_components(out, width, height)
    if stitched is not None:
        return stitched

    raise ValueError(f"expected {W}x{H} or {W}x96 (or proportional custom atlas), got {width}x{height}")


def write_png_rgba_from_atlas(atlas: list[int], path: Path) -> None:
    """Write 96x96 RGBA PNG (stdlib only)."""
    raw = bytearray()
    for y in range(H):
        raw.append(0)
        for x in range(W):
            c = atlas[y * W + x]
            r = (c >> 16) & 0xFF
            g = (c >> 8) & 0xFF
            b = c & 0xFF
            a = 0 if c == 0 else 255
            raw.extend([r, g, b, a])
    compressed = zlib.compress(bytes(raw), 9)

    def chunk(tag: bytes, payload: bytes) -> bytes:
        return struct.pack(">I", len(payload)) + tag + payload + struct.pack(
            ">I", zlib.crc32(tag + payload) & 0xFFFFFFFF
        )

    ihdr = struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", compressed)
        + chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def build_icon_atlas() -> list[int]:
    """Prefer user-authored PNG atlas inputs; fall back to generated or legacy art."""
    # Prefer explicit/custom names first, then the canonical generated names.
    candidates = (
        ASSET_DIR / "icon_atlas_custom.png",
        ASSET_DIR / "icon_atlas.png",
        ICON_ATLAS_PNG_96,
        ICON_ATLAS_PNG,
    )
    for candidate in candidates:
        if candidate.exists():
            try:
                return load_icon_atlas_from_png(candidate)
            except (OSError, ValueError) as e:
                print(
                    f"Warning: could not load {candidate}: {e}; trying fallback",
                    file=sys.stderr,
                )
    if HAS_PIL:
        try:
            return build_icon_atlas_pil()
        except Exception as e:
            print(
                f"Warning: Pillow icon atlas failed ({e}); using legacy fallback",
                file=sys.stderr,
            )
    return build_icon_atlas_legacy()


def load_font_8x8() -> list[list[int]]:
    text = FONT_H.read_text(encoding="utf-8")
    rows = [[0] * 8 for _ in range(128)]

    line_pat = re.compile(
        r"^\s*\[\s*(?:('(?:\\.|[^'\\])')|(\d+))\s*\]\s*=\s*\{([^}]*)\}\s*,?\s*$"
    )

    matched = 0
    for line in text.splitlines():
        m = line_pat.match(line)
        if not m:
            continue
        ch_literal = m.group(1)
        num_literal = m.group(2)
        vals_expr = m.group(3)
        if ch_literal is not None:
            ch = ast.literal_eval(ch_literal)
            idx = ord(ch)
        else:
            idx = int(num_literal)
        vals = [v.strip() for v in vals_expr.split(",") if v.strip()]
        if len(vals) != 8:
            raise RuntimeError(f"Unexpected glyph row count for index {idx}: {vals!r}")
        rows[idx] = [int(v, 0) & 0xFF for v in vals]
        matched += 1

    if matched == 0:
        raise RuntimeError("Failed to parse font initializers in kernel/font.h")

    return rows


def upscale_8x8_to_12x18(row_data: list[int]) -> list[int]:
    """Upscale 8x8 MSB-left rows to 12x18; 12-bit rows MSB-left (matches draw_char_12x18)."""
    out: list[int] = []
    for row in range(18):
        src_row = (row * 8) // 18
        src_bits = row_data[src_row]
        row_bits = 0
        for col in range(12):
            src_col = (col * 8) // 12
            if src_bits & (1 << (7 - src_col)):
                row_bits |= 1 << (11 - col)
        out.append(row_bits)
    return out


def rasterize_glyphs_8x8(ttf_path: Path) -> list[list[int]]:
    rows = [[0] * 8 for _ in range(128)]
    font = ImageFont.truetype(str(ttf_path), 10)
    for code in range(128):
        ch = chr(code)
        img = Image.new("L", (8, 8), 0)
        dr = ImageDraw.Draw(img)
        dr.text((0, 0), ch, font=font, fill=255)
        bbox = img.getbbox()
        if bbox:
            crop = img.crop(bbox)
            crop = crop.resize((8, 8), Image.Resampling.LANCZOS)
            img = Image.new("L", (8, 8), 0)
            img.paste(crop, (0, 0))
        pix = img.load()
        for r in range(8):
            b = 0
            for c in range(8):
                if pix[c, r] > 128:
                    b |= 1 << (7 - c)
            rows[code][r] = b
    return rows


def rasterize_glyphs_12x18(ttf_path: Path) -> list[list[int]]:
    out: list[list[int]] = []
    font = ImageFont.truetype(str(ttf_path), 15)
    for code in range(128):
        ch = chr(code)
        img = Image.new("L", (12, 18), 0)
        dr = ImageDraw.Draw(img)
        dr.text((0, 0), ch, font=font, fill=255)
        bbox = img.getbbox()
        if bbox:
            crop = img.crop(bbox)
            crop = crop.resize((12, 18), Image.Resampling.LANCZOS)
            img = Image.new("L", (12, 18), 0)
            img.paste(crop, (0, 0))
        pix = img.load()
        glyph_rows: list[int] = []
        for r in range(18):
            row_bits = 0
            for c in range(12):
                if pix[c, r] > 128:
                    row_bits |= 1 << (11 - c)
            glyph_rows.append(row_bits)
        out.append(glyph_rows)
    return out


def write_font_h(glyphs: list[list[int]], header_comment: str) -> None:
    lines = [
        header_comment,
        "unsigned char font_8x8_basic[128][8] = {",
    ]
    for i in range(128):
        vals = ", ".join(f"0x{v:02X}" for v in glyphs[i])
        lines.append(f"    [{i}] = {{{vals}}},")
    lines.append("};")
    FONT_H.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_icon_header(atlas: list[int]) -> None:
    lines = []
    lines.append("#ifndef UI_ICON_ATLAS_96X96_H")
    lines.append("#define UI_ICON_ATLAS_96X96_H")
    lines.append("")
    lines.append("#define UI_ICON_ATLAS_W 96")
    lines.append("#define UI_ICON_ATLAS_H 144")
    lines.append("#define UI_ICON_TILE 48")
    lines.append("")
    lines.append(
        "static const unsigned int ui_icon_atlas_96x96[UI_ICON_ATLAS_W * UI_ICON_ATLAS_H] = {"
    )

    for y in range(H):
        row = atlas[y * W : (y + 1) * W]
        chunk = ", ".join(f"0x{v:06X}" for v in row)
        lines.append(f"  {chunk},")

    lines.append("};")
    lines.append("")
    lines.append("#endif")
    ICON_HEADER.write_text("\n".join(lines) + "\n", encoding="ascii")


def write_font_header(font12: list[list[int]]) -> None:
    lines = []
    lines.append("#ifndef UI_FONT12X18_H")
    lines.append("#define UI_FONT12X18_H")
    lines.append("")
    lines.append("#define UI_FONT12X18_GLYPHS 128")
    lines.append("#define UI_FONT12X18_ROWS 18")
    lines.append("")
    lines.append(
        "static const unsigned short ui_font12x18[UI_FONT12X18_GLYPHS][UI_FONT12X18_ROWS] = {"
    )

    for glyph in font12:
        chunk = ", ".join(f"0x{v:04X}" for v in glyph)
        lines.append(f"  {{{chunk}}},")

    lines.append("};")
    lines.append("")
    lines.append("#endif")
    FONT_HEADER.write_text("\n".join(lines) + "\n", encoding="ascii")


def main() -> None:
    ASSET_DIR.mkdir(parents=True, exist_ok=True)

    # Keep UI text deterministic/readable by default.
    # Set NS_UI_FONT_TTF=1 to enable Pillow+TTF rasterization path.
    use_plex = HAS_PIL and ("NS_UI_FONT_TTF" in os.environ)
    if use_plex:
        try:
            ensure_download(IBM_PLEX_MONO_URL, IBM_PLEX_TTF)
            glyphs_8 = rasterize_glyphs_8x8(IBM_PLEX_TTF)
            font12 = rasterize_glyphs_12x18(IBM_PLEX_TTF)
            write_font_h(
                glyphs_8,
                "/* IBM Plex Mono - rasterized 8x8 (tools/export_ui_assets.py, Pillow) */",
            )
        except (OSError, urllib.error.URLError) as e:
            print(f"IBM Plex rasterize failed ({e}); using kernel/font.h + upscale", file=sys.stderr)
            use_plex = False

    if not use_plex:
        font8 = load_font_8x8()
        font12 = [upscale_8x8_to_12x18(g) for g in font8]
        # Preserve previous UI tweak for 'A' when using bitmap upscale
        font12[ord("A")] = [
            0x0780,
            0x0780,
            0x0C60,
            0x0C60,
            0x1830,
            0x1830,
            0x3018,
            0x3018,
            0x3FC0,
            0x3FC0,
            0x3018,
            0x3018,
            0x600C,
            0x600C,
            0x600C,
            0x600C,
            0x0000,
            0x0000,
        ]

    write_font_header(font12)

    atlas = build_icon_atlas()
    write_icon_header(atlas)
    write_png_rgba_from_atlas(atlas, ICON_ATLAS_PNG)

    print(f"Wrote {FONT_HEADER.relative_to(ROOT)}")
    print(f"Wrote {ICON_HEADER.relative_to(ROOT)}")
    print(f"Wrote {ICON_ATLAS_PNG.relative_to(ROOT)}")
    if use_plex:
        print(f"Wrote {FONT_H.relative_to(ROOT)} (IBM Plex Mono)")


if __name__ == "__main__":
    main()
