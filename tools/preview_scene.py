"""Composite a full-screen preview like renderer.cpp: sky, stars, pixel layer (nearest x scale),
scrolling rows, then sprites. Usage: preview_scene.py W H MODE PALETTE PHASE OUT.png ASSETS_DIR [PREVIEW_BIN]
Build PREVIEW_BIN from tools/preview_backdrop.cpp + src/pixel_backdrop.cpp with any C++20 compiler.
"""
import random
import subprocess
import sys
import tempfile
import os
LAYER = os.path.join(tempfile.gettempdir(), "ts_layer.bin")
from PIL import Image

W, H, MODE, PAL, PHASE, OUT = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), float(sys.argv[5]), sys.argv[6]
ASSETS = sys.argv[7]
PREVIEW_BIN = sys.argv[8] if len(sys.argv) > 8 else "build-preview/preview_backdrop"
scale = max(1, min(8, min(W // 640, H // 360)))
bw, bh = -(-W // scale), -(-H // scale)


def hexc(v):
    return ((v >> 16) & 255, (v >> 8) & 255, v & 255)


stops = [(0.0, 0x090d27), (0.4, 0x401355), (0.67, 0x87315f if PAL else 0x8c246d), (1.0, 0x160d32)]
img = Image.new("RGB", (W, H))
px = img.load()
for y in range(H):
    t = y / H
    for i in range(len(stops) - 1):
        if stops[i][0] <= t <= stops[i + 1][0]:
            a, b = stops[i], stops[i + 1]
            f = (t - a[0]) / (b[0] - a[0])
            c = tuple(round(p + (q - p) * f) for p, q in zip(hexc(a[1]), hexc(b[1])))
            break
    for x in range(W):
        px[x, y] = c
if MODE == 2:
    img.paste((9, 13, 39), (0, 0, W, H))
rnd = random.Random(4)
s = max(1, scale // 2)
for _ in range(90):
    x, y, br = int(rnd.random() * W), int(rnd.random() * 0.59 * H * (1 / 0.59 if MODE == 2 else 1)), rnd.uniform(0.25, 0.85)
    for dx in range(s):
        for dy in range(s):
            if x + dx < W and y + dy < H:
                o = px[x + dx, y + dy]
                px[x + dx, y + dy] = tuple(round(br * c + (1 - br) * v) for c, v in zip((224, 217, 255), o))
img = img.convert("RGBA")
if MODE != 2:
    res = subprocess.run([PREVIEW_BIN, str(bw), str(bh), str(MODE), str(PAL), str(PHASE), LAYER],
                         capture_output=True, text=True, check=True).stdout.split("\n")
    lw, lh, hz = map(int, res[0].split())
    raw = open(LAYER, "rb").read()
    layer = Image.frombuffer("RGBA", (lw, lh), raw, "raw", "BGRA", 0, 1)
    # premultiplied -> straight for PIL compositing
    lp = layer.load()
    for y in range(lh):
        for x in range(lw):
            r, g, b, a = lp[x, y]
            if 0 < a < 255:
                lp[x, y] = (min(255, r * 255 // a), min(255, g * 255 // a), min(255, b * 255 // a), a)
    big = layer.resize((lw * scale, lh * scale), Image.NEAREST).crop((0, 0, W, H))
    img.alpha_composite(big)
    for line in res[1:]:
        if not line.strip():
            continue
        y, rgb, al = line.split()
        y = int(y)
        band = Image.new("RGBA", (W, scale), hexc(int(rgb, 16)) + (round(float(al) * 255),))
        img.alpha_composite(band, (0, y * scale))
names = ["toaster", "toast-light", "toast-golden", "toast-dark"]
sheets = [Image.open(f"{ASSETS}/{n}.png") for n in names]
size = 48 * scale
for k in range(16):
    sh = sheets[0] if k % 3 else sheets[1 + k % 3]
    fr = k % 4
    spr = sh.crop((fr * 48, 0, fr * 48 + 48, 48)).resize((size, size), Image.NEAREST)
    x, y = rnd.randrange(0, W - size), rnd.randrange(0, H - size)
    img.alpha_composite(spr, (x, y))
img.convert("RGB").save(OUT)
