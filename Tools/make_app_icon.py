"""Renders the WOBBLE MACHINE iOS app icon.

Procedural (no external assets): charcoal scope background with an orange glow, a bold W built
from the wobble waveform, and a sub-bass sine underneath. Everything is drawn on
a 2x supersampled canvas so curves stay clean at 29x29.

Usage: python Tools/make_app_icon.py [size]
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "Assets"
SS = 2  # supersampling factor


def mix(c0, c1, t):
    t = min(1.0, max(0.0, t))
    return tuple(int(round(c0[i] + (c1[i] - c0[i]) * t)) for i in range(3))


def vertical_gradient(size, top, bottom):
    img = Image.new("RGB", (size, size))
    draw = ImageDraw.Draw(img)
    for y in range(size):
        draw.line([(0, y), (size, y)], fill=mix(top, bottom, y / max(1, size - 1)))
    return img


def vertical_gradient_range(size, top_y, bottom_y, top, bottom):
    img = Image.new("RGB", (size, size))
    draw = ImageDraw.Draw(img)
    for y in range(size):
        draw.line([(0, y), (size, y)], fill=mix(top, bottom, (y - top_y) / max(1.0, bottom_y - top_y)))
    return img


def solid(size, colour):
    return Image.new("RGB", (size, size), colour)


def add(base, layer, opacity=1.0):
    if opacity < 1.0:
        layer = layer.point(lambda v: int(v * opacity))
    return ImageChops.add(base, layer)


def radial_glow(size, center, radius, colour, strength):
    glow = solid(size, (0, 0, 0))
    draw = ImageDraw.Draw(glow)
    steps = 40
    for i in range(steps, 0, -1):
        t = i / steps
        r = radius * t
        a = strength * (1.0 - t) ** 2.2
        draw.ellipse(
            [center[0] - r, center[1] - r, center[0] + r, center[1] + r],
            fill=(int(colour[0] * a / 255), int(colour[1] * a / 255), int(colour[2] * a / 255)),
        )
    return glow


def stamp_polyline(draw, points, width, colour):
    """Round-pen polyline. Stamps a disc per point so dense curves never gap."""
    r = width / 2.0
    for x, y in points:
        draw.ellipse([x - r, y - r, x + r, y + r], fill=colour)


def curve_points(x0, x1, y_at, samples=360):
    return [(x0 + (x1 - x0) * (i / samples), y_at(i / samples)) for i in range(samples + 1)]


def build(requested_size: int) -> Image.Image:
    size = requested_size * SS
    s = size / 1024.0
    px = lambda v: v * s

    base = vertical_gradient(size, (12, 10, 8), (54, 24, 6))
    base = add(base, radial_glow(size, (px(512), px(500)), px(500), (190, 80, 10), 210))

    # Scope rings and ticks.
    ring = solid(size, (0, 0, 0))
    rd = ImageDraw.Draw(ring)
    for radius, alpha in ((px(316), 1.0), (px(372), 0.75), (px(430), 1.0)):
        rd.ellipse([px(512) - radius, px(512) - radius, px(512) + radius, px(512) + radius],
                   outline=(int(255 * alpha), int(120 * alpha), int(30 * alpha)),
                   width=max(1, int(px(2))))
    for i in range(60):
        angle = i * math.tau / 60
        if math.sin(angle) > 0.45:  # keep the sub-bass area clean
            continue
        r0 = px(338)
        r1 = px(338) + (px(15) if i % 5 == 0 else px(7))
        major = i % 5 == 0
        colour = (255, 150, 60) if major else (120, 60, 20)
        rd.line([(px(512) + math.cos(angle) * r0, px(512) + math.sin(angle) * r0),
                 (px(512) + math.cos(angle) * r1, px(512) + math.sin(angle) * r1)],
                fill=colour, width=max(1, int(px(3))))
    base = add(base, ring, 0.20)

    # Sub-bass sine: the weight under the emblem.
    sub_points = curve_points(px(306), px(718),
                              lambda t: px(748) + math.sin(t * math.tau + math.pi) * px(36))

    sub_glow = solid(size, (0, 0, 0))
    stamp_polyline(ImageDraw.Draw(sub_glow), sub_points, px(40), (150, 70, 15))
    base = add(base, sub_glow.filter(ImageFilter.GaussianBlur(px(24))), 0.85)

    sub_core = Image.new("L", (size, size), 0)
    stamp_polyline(ImageDraw.Draw(sub_core), sub_points, px(24), 255)
    base.paste(solid(size, (255, 150, 60)), (0, 0), sub_core)

    # W glyph: lavender-to-violet gradient through a stroked mask.
    w_points = [
        (px(268), px(300)),
        (px(388), px(672)),
        (px(512), px(452)),
        (px(636), px(672)),
        (px(756), px(300)),
    ]
    mask = Image.new("L", (size, size), 0)
    md = ImageDraw.Draw(mask)
    md.line(w_points, fill=255, width=int(px(88)), joint="curve")
    stamp_polyline(md, w_points, px(88), 255)

    # Dark halo separates the glyph from its own bloom.
    halo = mask.filter(ImageFilter.MaxFilter((int(px(11)) | 1)))
    base.paste(solid(size, (18, 10, 4)), (0, 0), Image.eval(halo, lambda v: int(v * 0.85)))

    glow_mask = mask.filter(ImageFilter.GaussianBlur(px(26)))
    glow = solid(size, (0, 0, 0))
    glow.paste((255, 120, 20), (0, 0), glow_mask)
    base = add(base, glow, 0.60)

    base.paste(vertical_gradient_range(size, px(300), px(672), (255, 244, 232), (255, 118, 16)),
               (0, 0), mask)

    # Top-edge highlight.
    hl = Image.new("L", (size, size), 0)
    ImageDraw.Draw(hl).line(w_points, fill=110, width=max(1, int(px(12))), joint="curve")
    base.paste(solid(size, (255, 255, 255)), (0, 0), hl.filter(ImageFilter.GaussianBlur(px(5))))

    # Wobble trace across the glyph.
    trace_points = curve_points(
        px(214), px(810),
        lambda t: px(492) + math.sin(t * math.tau * 3.0) * px(30) * (math.sin(t * math.pi) ** 0.6))
    trace = solid(size, (0, 0, 0))
    stamp_polyline(ImageDraw.Draw(trace), trace_points, px(12), (252, 248, 255))
    base = add(base, trace, 0.92)

    # Vignette.
    vignette = Image.new("L", (size, size), 0)
    ImageDraw.Draw(vignette).ellipse([-px(150), -px(150), px(1174), px(1174)], fill=255)
    vignette = vignette.filter(ImageFilter.GaussianBlur(px(80)))
    edge = Image.eval(vignette, lambda v: int((255 - v) * 0.50))
    base.paste(solid(size, (6, 4, 12)), (0, 0), edge)

    return base.resize((requested_size, requested_size), Image.LANCZOS)


def main() -> None:
    size = int(sys.argv[1]) if len(sys.argv) > 1 else 1024
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    icon = build(size)
    out = OUT_DIR / f"AppIcon-{size}.png"
    icon.save(out, "PNG", optimize=True)
    print(f"wrote {out} ({out.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
