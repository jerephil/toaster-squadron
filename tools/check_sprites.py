import sys
from PIL import Image

PAL = {"17182F", "667590", "B8CAD9", "F1F5E8", "55E6E0", "9F72E5", "FF807D",
       "A95735", "F7E4AF", "E9B866", "9A6541", "76452F"}
BG = [(9, 13, 39), (255, 170, 96), (214, 65, 160)]
d = sys.argv[1]
names = ["toaster", "toast-light", "toast-golden", "toast-dark"]
preview = Image.new("RGBA", (192 * 6, 48 * 6 * 4 * 2), (0, 0, 0, 255))
ok = True
for r, n in enumerate(names):
    im = Image.open(f"{d}/{n}.png")
    print(n, im.size, im.mode, end=" ")
    assert im.size == (192, 48) and im.mode == "RGBA"
    alphas, bad = set(), set()
    for y in range(48):
        for x in range(192):
            c = im.getpixel((x, y))
            alphas.add(c[3])
            if c[3] == 255 and "%02X%02X%02X" % c[:3] not in PAL:
                bad.add(c)
    print("alphas", sorted(alphas), "offpalette", bad, end=" ")
    for i in range(4):
        bb = im.crop((i * 48, 0, i * 48 + 48, 48)).getbbox()
        inside = bb[0] >= 2 and bb[1] >= 2 and bb[2] <= 46 and bb[3] <= 46
        ok &= inside and not bad
        print(f"f{i}{bb}{'ok' if inside else 'EDGE'}", end=" ")
    print()
    for j, bg in enumerate([BG[0], BG[1]]):
        tile = Image.new("RGBA", (192, 48), bg + (255,))
        tile.alpha_composite(im)
        preview.paste(tile.resize((192 * 6, 48 * 6), Image.NEAREST), (0, (r * 2 + j) * 48 * 6))
preview.save(f"{sys.argv[2]}")
print("ALL OK" if ok else "PROBLEMS")
