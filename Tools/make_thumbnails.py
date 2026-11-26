"""Composes the iOS marketing assets from the real plug-in render.

Inputs  : Assets/ui-render@2x.png  (rendered by the MKThumbnail target)
          Assets/AppIcon-1024.png  (rendered by Tools/make_app_icon.py)
Outputs : Assets/iOS-Thumbnail-1024.png          square thumbnail / store tile
          Assets/iOS-Screenshot-2732x2048.png    landscape iPad screenshot

Run: python Tools/make_thumbnails.py
"""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "Assets"

TOP = (18, 10, 38)
BOTTOM = (44, 18, 82)
ACCENT = (168, 85, 247)
ACCENT_BRIGHT = (200, 139, 251)
TEXT = (237, 233, 247)
TEXT_DIM = (150, 140, 175)

FONT_BOLD = "C:/Windows/Fonts/seguisb.ttf"
FONT_REGULAR = "C:/Windows/Fonts/segoeui.ttf"


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    path = FONT_BOLD if bold else FONT_REGULAR
    try:
        return ImageFont.truetype(path, size)
    except OSError:
        return ImageFont.load_default()


def background(width: int, height: int) -> Image.Image:
    img = Image.new("RGB", (width, height))
    draw = ImageDraw.Draw(img)
    for y in range(height):
        t = y / max(1, height - 1)
        draw.line(
            [(0, y), (width, y)],
            fill=tuple(int(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3)),
        )

    # Soft purple bloom behind the product shot.
    glow = Image.new("RGB", (width, height), (0, 0, 0))
    gd = ImageDraw.Draw(glow)
    cx, cy = width // 2, int(height * 0.56)
    radius = int(min(width, height) * 0.52)
    steps = 36
    for i in range(steps, 0, -1):
        t = i / steps
        r = radius * t
        a = 70 * (1 - t) ** 2.4
        gd.ellipse([cx - r, cy - r, cx + r, cy + r],
                   fill=(int(120 * a / 255), int(60 * a / 255), int(220 * a / 255)))
    return ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(40)))


def rounded(image: Image.Image, radius: int, border: tuple[int, int, int] | None = None,
            border_width: int = 2) -> Image.Image:
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, image.size[0] - 1, image.size[1] - 1],
                                           radius=radius, fill=255)
    out = Image.new("RGBA", image.size, (0, 0, 0, 0))
    out.paste(image.convert("RGBA"), (0, 0), mask)
    if border is not None:
        ImageDraw.Draw(out).rounded_rectangle([0, 0, image.size[0] - 1, image.size[1] - 1],
                                              radius=radius, outline=border + (255,),
                                              width=border_width)
    return out


def with_shadow(base: Image.Image, frame: Image.Image, position: tuple[int, int],
                blur: int, opacity: int) -> None:
    shadow = Image.new("RGBA", frame.size, (0, 0, 0, 0))
    shadow.paste((0, 0, 0, opacity), (0, 0), frame.split()[-1])
    shadow = shadow.filter(ImageFilter.GaussianBlur(blur))
    base.alpha_composite(shadow, (position[0] + 2, position[1] + 6))
    base.alpha_composite(frame, position)


def load_ui(target_width: int) -> Image.Image:
    ui = Image.open(ASSETS / "ui-render@2x.png").convert("RGB")
    ratio = target_width / ui.size[0]
    return ui.resize((target_width, int(ui.size[1] * ratio)), Image.LANCZOS)


def square_thumbnail(size: int = 1024) -> Image.Image:
    base = background(size, size).convert("RGBA")

    icon = Image.open(ASSETS / "AppIcon-1024.png").convert("RGB")
    icon_size = int(size * 0.13)
    icon = rounded(icon.resize((icon_size, icon_size), Image.LANCZOS), int(icon_size * 0.22),
                   border=(60, 40, 96))
    base.alpha_composite(icon, (int(size * 0.065), int(size * 0.055)))

    draw = ImageDraw.Draw(base)
    text_x = int(size * 0.065) + icon_size + int(size * 0.035)
    draw.text((text_x, int(size * 0.062)), "MK WOBBLE BASS", font=font(int(size * 0.062), True),
              fill=TEXT)
    draw.text((text_x, int(size * 0.132)), "Wobble bass synthesizer  ·  AUv3 + VST3",
              font=font(int(size * 0.031)), fill=TEXT_DIM)

    ui = load_ui(int(size * 0.9))
    frame = rounded(ui, int(size * 0.022), border=(70, 48, 112))
    position = ((size - frame.size[0]) // 2, int(size * 0.235))
    with_shadow(base, frame, position, blur=int(size * 0.02), opacity=150)

    # Feature strip.
    features = ["56 presets", "tempo-synced wobble", "FM growl", "iPad AUv3"]
    y = position[1] + frame.size[1] + int(size * 0.045)
    pill_font = font(int(size * 0.026), True)
    total = 0
    widths = []
    for label in features:
        box = draw.textbbox((0, 0), label, font=pill_font)
        widths.append(box[2] - box[0] + int(size * 0.036))
        total += widths[-1] + int(size * 0.014)
    total -= int(size * 0.014)

    x = (size - total) // 2
    for label, width in zip(features, widths):
        draw.rounded_rectangle([x, y, x + width, y + int(size * 0.052)],
                               radius=int(size * 0.026), fill=(38, 24, 64),
                               outline=(78, 54, 122))
        box = draw.textbbox((0, 0), label, font=pill_font)
        draw.text((x + (width - (box[2] - box[0])) / 2, y + int(size * 0.011)), label,
                  font=pill_font, fill=ACCENT_BRIGHT)
        x += width + int(size * 0.014)

    return base.convert("RGB")


def landscape_screenshot(width: int = 2732, height: int = 2048) -> Image.Image:
    base = background(width, height).convert("RGBA")

    icon_size = int(height * 0.075)
    icon = Image.open(ASSETS / "AppIcon-1024.png").convert("RGB")
    icon = rounded(icon.resize((icon_size, icon_size), Image.LANCZOS), int(icon_size * 0.22),
                   border=(60, 40, 96))
    margin = int(width * 0.045)
    base.alpha_composite(icon, (margin, int(height * 0.045)))

    draw = ImageDraw.Draw(base)
    text_x = margin + icon_size + int(width * 0.018)
    draw.text((text_x, int(height * 0.048)), "MK WOBBLE BASS", font=font(int(height * 0.052), True),
              fill=TEXT)
    draw.text((text_x, int(height * 0.112)),
              "Wobble, growl and neuro bass for iPad (AUv3) and desktop (VST3)",
              font=font(int(height * 0.024)), fill=TEXT_DIM)

    ui = load_ui(int(width * 0.9))
    frame = rounded(ui, int(width * 0.011), border=(70, 48, 112))
    position = ((width - frame.size[0]) // 2, int(height * 0.19))
    with_shadow(base, frame, position, blur=int(width * 0.008), opacity=150)

    return base.convert("RGB")


def main() -> None:
    if not (ASSETS / "ui-render@2x.png").exists():
        raise SystemExit("run the MKThumbnail target first (Tools/MakeThumbnail.cpp)")

    square = square_thumbnail(1024)
    square.save(ASSETS / "iOS-Thumbnail-1024.png", "PNG", optimize=True)

    shot = landscape_screenshot()
    shot.save(ASSETS / "iOS-Screenshot-2732x2048.png", "PNG", optimize=True)

    for name in ("iOS-Thumbnail-1024.png", "iOS-Screenshot-2732x2048.png"):
        path = ASSETS / name
        print(f"wrote {path} ({path.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
