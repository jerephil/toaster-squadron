#!/usr/bin/env python3
"""Generate original placeholder PNG sheets and an ICO, using no packages."""
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
PALETTE = {key: tuple(bytes.fromhex(value)) + (255,) for key, value in {
    'outline': '17182F', 'shadow': '667590', 'chrome': 'B8CAD9',
    'wing': 'F1F5E8', 'cyan': '55E6E0', 'dial': '9F72E5',
    'lever': 'FF807D', 'crust': 'A95735', 'light': 'F7E4AF',
    'golden': 'E9B866', 'dark': '9A6541', 'freckle': '76452F',
}.items()}


def canvas(w, h):
    return [[(0, 0, 0, 0) for _ in range(w)] for _ in range(h)]


def rect(img, x0, y0, x1, y1, color):
    c = PALETTE.get(color, color)
    for y in range(max(0, y0), min(len(img), y1 + 1)):
        for x in range(max(0, x0), min(len(img[0]), x1 + 1)):
            img[y][x] = c


def ellipse(img, x0, y0, x1, y1, color):
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    rx, ry = (x1 - x0 + 1) / 2, (y1 - y0 + 1) / 2
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1:
                rect(img, x, y, x, y, color)


def frame(kind, pose):
    img = canvas(48, 48)
    top, bottom = [(9, 23), (19, 28), (27, 38), (19, 28)][pose]
    for left, right in [(3, 16), (31, 44)]:
        ellipse(img, left, top, right, bottom, 'outline')
        ellipse(img, left + 2, top + 2, right - 2, bottom - 2, 'wing')
        rect(img, left + 4, (top + bottom) // 2, right - 4,
             (top + bottom) // 2 + 1, 'chrome')
    if kind == 'toaster':
        rect(img, 16, 16, 31, 36, 'outline')
        rect(img, 13, 19, 34, 33, 'outline')
        rect(img, 15, 19, 32, 33, 'chrome')
        rect(img, 17, 18, 30, 34, 'chrome')
        rect(img, 16, 20, 18, 31, 'wing')
        rect(img, 20, 18, 29, 20, 'shadow')
        rect(img, 21, 18, 28, 18, 'outline')
        rect(img, 24, 22, 31, 24, 'wing')
        rect(img, 19, 33, 31, 34, 'shadow')
        ellipse(img, 25, 27, 30, 32, 'outline')
        ellipse(img, 26, 28, 29, 31, 'dial')
        rect(img, 33, 23, 35, 24, 'outline')
        rect(img, 34, 24, 36, 26, 'lever')
        rect(img, 17, 36, 20, 37, 'outline')
        rect(img, 28, 36, 31, 37, 'outline')
        rect(img, 14, 27, 15, 28, 'cyan')
    else:
        rect(img, 15, 17, 32, 35, 'outline')
        ellipse(img, 13, 11, 33, 24, 'outline')
        rect(img, 16, 17, 31, 34, 'crust')
        ellipse(img, 14, 12, 32, 23, 'crust')
        rect(img, 18, 19, 29, 32, kind)
        ellipse(img, 17, 14, 30, 24, kind)
        rect(img, 20, 24, 21, 25, 'outline')
        rect(img, 27, 24, 28, 25, 'outline')
        rect(img, 23, 28, 25, 28, 'freckle')
        rect(img, 21, 19, 21, 20, 'freckle')
        rect(img, 28, 30, 28, 30, 'freckle')
        rect(img, 15, 27, 16, 28, 'cyan')
    return img


def png(img):
    h, w = len(img), len(img[0])
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))
    raw = b''.join(b'\0' + bytes(c for pixel in row for c in pixel) for row in img)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>2I5B', w, h, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def main():
    (ROOT / 'assets').mkdir(exist_ok=True)
    for kind in ['toaster', 'light', 'golden', 'dark']:
        frames = [frame(kind, i) for i in range(4)]
        sheet = [sum((f[y] for f in frames), []) for y in range(48)]
        name = 'toaster' if kind == 'toaster' else f'toast-{kind}'
        (ROOT / 'assets' / f'{name}.png').write_bytes(png(sheet))
    base = frame('toaster', 0)
    entries, images, offset = [], [], 6 + 4 * 16
    for size in [16, 32, 48, 256]:
        img = [[base[y * 48 // size][x * 48 // size]
                if base[y * 48 // size][x * 48 // size][3] else PALETTE['outline']
                for x in range(size)] for y in range(size)]
        data = png(img)
        entries.append(struct.pack('<4B2H2I', size % 256, size % 256, 0, 0, 1, 32, len(data), offset))
        images.append(data)
        offset += len(data)
    (ROOT / 'resources' / 'toaster.ico').write_bytes(struct.pack('<3H', 0, 1, 4) + b''.join(entries + images))
    print('Generated four original 192x48 sheets and a four-size icon.')


if __name__ == '__main__':
    main()
