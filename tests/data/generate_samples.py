#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 LingmoOS Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Makes the sample pictures the image viewer's tests and captures use."""

import math
import os
import random

from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))


def lerp(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3))


def vertical_gradient(size, top, bottom, height_ratio=1.0):
    width, height = size
    image = Image.new("RGB", (width, max(1, round(height * height_ratio))))
    draw = ImageDraw.Draw(image)
    for y in range(image.height):
        draw.line([(0, y), (width, y)], fill=lerp(top, bottom, y / max(1, image.height - 1)))
    return image


def landscape():
    """A dusk landscape: the reference picture in the screenshots."""
    width, height = 1024, 640
    horizon = 384

    sky = vertical_gradient((width, horizon), (12, 16, 44), (86, 59, 92))
    image = Image.new("RGB", (width, height), (8, 10, 24))
    image.paste(sky, (0, 0))

    # Stars, thinning out towards the horizon.
    rng = random.Random(7)
    draw = ImageDraw.Draw(image, "RGBA")
    for _ in range(140):
        x = rng.uniform(0, width)
        y = rng.uniform(0, horizon * 0.82)
        fade = int(150 * (1 - y / (horizon * 0.9)))
        radius = rng.choice([0.6, 0.6, 0.8, 1.0])
        draw.ellipse([x - radius, y - radius, x + radius, y + radius],
                     fill=(255, 250, 235, fade))

    # The sun and its glow.
    sun_x, sun_y, sun_r = 700, horizon - 96, 58
    for i in range(28, 0, -1):
        radius = sun_r * (1 + i * 0.13)
        alpha = int(30 * (1 - i / 29) + 4)
        draw.ellipse([sun_x - radius, sun_y - radius, sun_x + radius, sun_y + radius],
                     fill=(255, 186, 116, alpha))
    draw.ellipse([sun_x - sun_r, sun_y - sun_r, sun_x + sun_r, sun_y + sun_r],
                 fill=(255, 236, 196, 255))
    draw.ellipse([sun_x - sun_r + 10, sun_y - sun_r + 8, sun_x + sun_r - 10, sun_y + sun_r - 8],
                 fill=(255, 246, 222, 255))

    # Three mountain ridges, farther ones hazier.
    ridges = [
        ([(0, 330), (110, 250), (215, 300), (330, 214), (455, 296), (560, 236),
          (690, 302), (815, 244), (935, 296), (1024, 268), (1024, 384), (0, 384)], (74, 62, 104)),
        ([(0, 352), (140, 292), (250, 338), (395, 268), (520, 336), (650, 286),
          (790, 344), (900, 306), (1024, 344), (1024, 384), (0, 384)], (49, 44, 80)),
        ([(0, 372), (180, 334), (330, 366), (470, 326), (640, 372), (800, 340),
          (940, 370), (1024, 356), (1024, 384), (0, 384)], (29, 28, 54)),
    ]
    for points, color in ridges:
        draw.polygon(points, fill=color + (255,))

    # The water: the sky reflected, with the sun's column of light.
    water = vertical_gradient((width, height - horizon), (34, 38, 70), (9, 11, 26))
    image.paste(water, (0, horizon))
    draw = ImageDraw.Draw(image, "RGBA")
    for i in range(24):
        t = i / 23
        half = 120 * (1 - t) + 18
        y = horizon + 8 + i * 10
        alpha = int(70 * (1 - t) ** 1.6)
        draw.ellipse([sun_x - half, y - 4, sun_x + half, y + 4], fill=(255, 190, 130, alpha))
    for row in range(0, height - horizon, 6):
        y = horizon + row + 3
        band = Image.new("RGBA", (width, 2), (0, 0, 0, 0))
        band_draw = ImageDraw.Draw(band)
        rng2 = random.Random(1000 + row)
        x = 0
        while x < width:
            length = rng2.randint(12, 70)
            alpha = rng2.randint(4, 26)
            band_draw.line([(x, 0), (x + length, 0)], fill=(210, 220, 255, alpha))
            x += length + rng2.randint(20, 80)
        image.paste(band, (0, y), band)

    # Birds over the water.
    draw = ImageDraw.Draw(image, "RGBA")
    for bx, by, size in [(212, 196, 9), (258, 224, 7), (166, 240, 6)]:
        draw.arc([bx - size, by - size // 2, bx, by + size // 2], 200, 340, fill=(22, 20, 34, 220), width=2)
        draw.arc([bx, by - size // 2, bx + size, by + size // 2], 200, 340, fill=(22, 20, 34, 220), width=2)

    image = image.filter(ImageFilter.SMOOTH)
    return image


def chart():
    """A colour chart for the second picture in the folder: zoom, sharpness."""
    width, height = 720, 480
    image = Image.new("RGB", (width, height), (18, 20, 28))
    draw = ImageDraw.Draw(image, "RGBA")

    bars = [(233, 84, 84), (235, 158, 75), (232, 210, 82), (108, 199, 110),
            (82, 196, 205), (88, 132, 220), (160, 108, 220), (220, 98, 168)]
    bar_width = width // len(bars)
    for i, color in enumerate(bars):
        draw.rectangle([i * bar_width, 0, (i + 1) * bar_width - 1, 150], fill=color + (255,))

    for x in range(width):
        value = round(255 * x / (width - 1))
        draw.line([(x, 158), (x, 214)], fill=(value, value, value))

    # Rings and a checkerboard, for looking at filter quality.
    for i in range(9):
        radius = 16 + i * 15
        draw.ellipse([110 - radius, 320 - radius, 110 + radius, 320 + radius],
                     outline=(230, 236, 248, 200 - i * 14), width=2)
    cell = 20
    for row in range(4):
        for column in range(6):
            if (row + column) % 2 == 0:
                draw.rectangle([250 + column * cell, 270 + row * cell,
                                250 + (column + 1) * cell - 1, 270 + (row + 1) * cell - 1],
                               fill=(220, 226, 240, 190))

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 40)
        small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20)
    except OSError:
        font = ImageFont.load_default()
        small = font
    draw.text((420, 270), "Lingmo", font=font, fill=(240, 244, 255, 255))
    draw.text((422, 318), "Image Viewer", font=small, fill=(168, 190, 230, 255))
    draw.text((422, 348), "720 x 480 test chart", font=small, fill=(120, 136, 168, 255))
    return image


def main():
    landscape().save(os.path.join(HERE, "lingmo-sample.png"), optimize=True)
    chart().save(os.path.join(HERE, "lingmo-sample-2.png"), optimize=True)
    with open(os.path.join(HERE, "not-an-image.png"), "wb") as broken:
        broken.write(b"This is not a PNG file, it only has the extension.\n")
    print("wrote", sorted(os.listdir(HERE)))


if __name__ == "__main__":
    main()
