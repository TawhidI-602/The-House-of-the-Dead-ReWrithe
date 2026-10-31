#!/usr/bin/env python3
"""Writes src/res/hotd.ico: The House of the Dead: ReWrithe's own neutral icon (a red crosshair on a dark tile), drawn here from scratch,
not derived from any game art. Plain Python, no dependencies: python tools/make_neutral_icon.py [out.ico]"""
import math
import struct
import sys


def draw(n):
    px = []
    c = (n - 1) / 2.0
    for y in range(n):
        row = []
        for x in range(n):
            dx, dy = x - c, y - c
            r = math.hypot(dx, dy)
            half = n / 2.0
            # rounded dark tile
            ax, ay = abs(dx) - (half - n * 0.18), abs(dy) - (half - n * 0.18)
            d = math.hypot(max(ax, 0), max(ay, 0)) + min(max(ax, ay), 0) - n * 0.18
            a = 255 if d < -0.5 else (int(255 * (0.5 - d)) if d < 0.5 else 0)
            col = (28, 32, 44)
            ring = abs(r - n * 0.30) < max(n * 0.055, 0.8)
            cross = (abs(dx) < max(n * 0.035, 0.6) or abs(dy) < max(n * 0.035, 0.6)) and r < n * 0.42 and r > n * 0.12
            dot = r < n * 0.05
            if ring or cross or dot:
                col = (214, 40, 40)
            row.append((col[0], col[1], col[2], max(0, min(255, a))))
        px.append(row)
    return px


def dib(n):
    px = draw(n)
    out = struct.pack("<IiiHHIIiiII", 40, n, n * 2, 1, 32, 0, n * n * 4, 0, 0, 0, 0)
    for y in range(n - 1, -1, -1):
        for (r, g, b, a) in px[y]:
            out += bytes((b, g, r, a))
    mask_stride = ((n + 31) // 32) * 4
    out += bytes(mask_stride * n)   # AND mask: all opaque; the alpha channel does the work
    return out


def main():
    sizes = [16, 32, 48, 64]
    images = [dib(n) for n in sizes]
    head = struct.pack("<HHH", 0, 1, len(sizes))
    off = 6 + 16 * len(sizes)
    entries = b""
    for n, im in zip(sizes, images):
        entries += struct.pack("<BBBBHHII", n, n, 0, 0, 1, 32, len(im), off)
        off += len(im)
    with open(sys.argv[1] if len(sys.argv) > 1 else "src/res/hotd.ico", "wb") as f:
        f.write(head + entries + b"".join(images))


main()
