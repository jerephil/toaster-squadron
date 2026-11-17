"""Generate Toaster Squadron sprite sheets: 192x48, 4x1 frames of 48x48, RGBA."""
import math
import sys
from PIL import Image

H = lambda s: tuple(int(s[i:i + 2], 16) for i in (1, 3, 5)) + (255,)
OUT = H("#17182F")
CSH = H("#667590")
CBODY = H("#B8CAD9")
CHI = H("#F1F5E8")
CYAN = H("#55E6E0")
VIOLET = H("#9F72E5")
CORAL = H("#FF807D")
CRUST = H("#A95735")
FRECK = H("#76452F")
CENTERS = {"light": H("#F7E4AF"), "golden": H("#E9B866"), "dark": H("#9A6541")}

CELL = 48
# wing angle per frame (degrees above horizontal): high, half down, low, half up
WING_ANGLES = [38, 10, -30, -6]


def outline(mask):
    """Return pixels 4/8-adjacent to mask but not in it."""
    ring = set()
    for (x, y) in mask:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                p = (x + dx, y + dy)
                if p not in mask:
                    ring.add(p)
    return ring


def wing_mask(sx, sy, side, angle, a=4.4, b=2.9, reach=5.4):
    """Rounded paddle wing. side=-1 left, +1 right. Returns (mask, center, dir)."""
    th = math.radians(angle)
    dx, dy = side * math.cos(th), -math.sin(th)
    cx, cy = sx + dx * reach, sy + dy * reach
    m = set()
    for y in range(0, CELL):
        for x in range(0, CELL):
            px, py = x + 0.5 - cx, y + 0.5 - cy
            u = px * dx + py * dy
            v = -px * dy + py * dx
            if (u / a) ** 2 + (v / b) ** 2 <= 1.0:
                m.add((x, y))
    # short stem joining paddle to body
    for t in range(0, int(reach) + 1):
        x = sx + dx * t
        y = sy + dy * t
        m.add((int(math.floor(x)), int(math.floor(y))))
        m.add((int(math.floor(x)), int(math.floor(y)) + 1))
    return m, (cx, cy), (dx, dy)


def draw_wing(px, sx, sy, side, angle):
    m, (cx, cy), _ = wing_mask(sx, sy, side, angle)
    for p in outline(m):
        px[p] = OUT
    for (x, y) in m:
        # lower edge of paddle gets chrome-body shade; front-left wing brighter
        below = (x, y + 1) not in m or (x, y + 2) not in m
        if below:
            px[(x, y)] = CBODY if side < 0 else CSH
        else:
            px[(x, y)] = CHI
    # little highlight on upper-left of left wing
    if side < 0:
        hx, hy = int(cx) - 1, int(cy) - 1
        if (hx, hy) in m:
            px[(hx, hy)] = CHI


def rounded_rect(x0, y0, x1, y1, r):
    m = set()
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            ok = True
            for (cx, cy) in ((x0 + r, y0 + r), (x1 - r, y0 + r), (x0 + r, y1 - r), (x1 - r, y1 - r)):
                inx = (x < x0 + r and cx == x0 + r) or (x > x1 - r and cx == x1 - r)
                iny = (y < y0 + r and cy == y0 + r) or (y > y1 - r and cy == y1 - r)
                if inx and iny and (x - cx) ** 2 + (y - cy) ** 2 > r * r + 0.5:
                    ok = False
            if ok:
                m.add((x, y))
    return m


def disc(cx, cy, r):
    return {(x, y) for y in range(int(cy - r) - 1, int(cy + r) + 2)
            for x in range(int(cx - r) - 1, int(cx + r) + 2)
            if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= r * r}


# ---------------- toaster ----------------
def toaster_frame(angle):
    img = Image.new("RGBA", (CELL, CELL), (0, 0, 0, 0))
    px = {}
    # wings behind body
    draw_wing(px, 13, 23, -1, angle)
    draw_wing(px, 34, 23, +1, angle)

    body = rounded_rect(14, 18, 33, 34, 4)  # outline lands on 13..34, 17..35
    for p in outline(body):
        px[p] = OUT
    for (x, y) in body:
        c = CBODY
        if x >= 31 or y >= 33:  # right side panel + base: shadow (3/4 form)
            c = CSH
        if x <= 15 and y <= 31:  # front-left highlight strip
            c = CHI
        if y <= 21:  # top face catches light
            c = CHI if x < 31 else CBODY
        px[(x, y)] = c
    # side-panel seam
    for y in range(20, 33):
        px[(30, y)] = CSH if y > 21 else CBODY
    # two short slots on top face
    for x in range(17, 22):
        px[(x, 20)] = OUT
    for x in range(24, 29):
        px[(x, 20)] = OUT
    for x in range(14, 31):
        px[(x, 22)] = CBODY if x > 15 else CHI
    # cyan glint, front-left
    for p in [(16, 21), (16, 22), (16, 23), (17, 21)]:
        px[p] = CYAN
    px[(16, 25)] = CYAN
    # eyes (2x2) and a small smile
    for (ex, ey) in [(19, 24), (25, 24)]:
        for dx in (0, 1):
            for dy in (0, 1):
                px[(ex + dx, ey + dy)] = OUT
    px[(19, 24)] = OUT
    px[(20, 24)] = CHI  # eye sparkle
    px[(26, 24)] = CHI
    for x in range(21, 25):
        px[(x, 28)] = OUT
    px[(20, 27)] = OUT
    px[(25, 27)] = OUT
    # violet dial, lower front-left
    d = disc(27.5, 31.0, 2.2)
    for p in outline(d):
        if p in body:
            px[p] = OUT
    for p in d:
        px[p] = VIOLET
    px[(27, 30)] = CHI
    # coral lever on right side panel
    lever = {(x, y) for x in range(31, 34) for y in range(22, 25)}
    for p in outline(lever):
        if p[0] <= 34 and p in body | outline(body):
            px[p] = OUT
    for p in lever:
        px[p] = CORAL
    px[(31, 22)] = CHI
    # lever slot below
    for y in range(26, 31):
        px[(32, y)] = OUT

    for p, c in px.items():
        img.putpixel(p, c)
    return img


# ---------------- toast ----------------
def toast_shape():
    m = rounded_rect(16, 21, 31, 34, 2)
    # lopsided three-lobe cloud crown: biggest, highest lobe on the left
    m |= disc(19.8, 16.0, 4.5)
    m |= disc(26.3, 15.9, 3.5)
    m |= disc(29.8, 19.2, 2.5)
    m -= {(23, 12), (23, 13), (24, 12), (29, 15), (30, 15), (30, 16)}
    m |= {(x, y) for x in range(16, 32) for y in range(19, 22)}
    m = {p for p in m if 16 <= p[0] <= 31 and 12 <= p[1] <= 34}
    return m


def toast_frame(angle, center, freckles):
    img = Image.new("RGBA", (CELL, CELL), (0, 0, 0, 0))
    px = {}
    draw_wing(px, 15, 25, -1, angle)
    draw_wing(px, 32, 25, +1, angle)

    shape = toast_shape()
    ring = outline(shape)
    for p in ring:
        px[p] = OUT

    def dist_to_edge(x, y, dx, dy):
        n = 0
        while (x + dx * (n + 1), y + dy * (n + 1)) in shape:
            n += 1
        return n

    for (x, y) in shape:
        left = dist_to_edge(x, y, -1, 0)
        right = dist_to_edge(x, y, 1, 0)
        up = dist_to_edge(x, y, 0, -1)
        down = dist_to_edge(x, y, 0, 1)
        # thicker crust on right/bottom suggests 3/4 depth
        if left < 1 or up < 1 or right < 3 or down < 3:
            px[(x, y)] = CRUST
        else:
            px[(x, y)] = center
    # front-left highlights on crust
    for p in [(18, 14), (19, 13), (20, 13), (17, 15), (17, 17)]:
        if p in shape:
            px[p] = CHI
    for p in [(17, 22), (17, 23), (17, 24)]:
        px[p] = CHI
    # eyes: tiny squares
    for (ex, ey) in [(20, 24), (25, 24)]:
        for dx in (0, 1):
            for dy in (0, 1):
                px[(ex + dx, ey + dy)] = OUT
    px[(20, 24)] = CHI if center != CENTERS["light"] else OUT
    # small open smile
    for x in range(22, 25):
        px[(x, 28)] = OUT
    px[(21, 27)] = OUT
    px[(25, 27)] = OUT
    for p in freckles:
        px[p] = FRECK

    for p, c in px.items():
        img.putpixel(p, c)
    return img


FRECKLES = {
    "light": [(19, 27), (27, 27)],
    "golden": [(19, 27), (27, 27), (18, 28), (28, 26), (23, 21)],
    "dark": [(19, 27), (27, 27), (18, 28), (28, 26), (23, 21), (20, 31), (26, 31), (21, 19)],
}


def sheet(frames):
    s = Image.new("RGBA", (CELL * 4, CELL), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        s.paste(f, (i * CELL, 0))
    return s


def main(outdir):
    sheets = {"toaster": sheet([toaster_frame(a) for a in WING_ANGLES])}
    for k in ("light", "golden", "dark"):
        sheets[f"toast-{k}"] = sheet([toast_frame(a, CENTERS[k], FRECKLES[k]) for a in WING_ANGLES])
    for name, s in sheets.items():
        s.save(f"{outdir}/{name}.png", optimize=True)


if __name__ == "__main__":
    main(sys.argv[1])
