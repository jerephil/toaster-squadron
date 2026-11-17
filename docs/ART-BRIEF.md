# Toaster Squadron — original sprite art brief

This is Jeremy W. Phillips's personal, MIT-licensed Windows screensaver. Art can begin from this document while the renderer is built. Everything must be original. Draw a friendly, rounded chrome toaster with compact round featherless wings, a violet circular temperature dial, a coral side lever, and a little cyan undercarriage. Give toast a lopsided cloud-shaped crown, tiny square eyes, and short rounded wings. Do not reference or trace existing screensaver sprites, color arrangements, silhouettes, or compositions.

## Delivery contract

Deliver true-color RGBA PNGs with transparent backgrounds, no color-key transparency, no padding between frames, and no interpolation. Every cell is **48 × 48 pixels**. Sheets are **192 × 48 pixels**, four frames in one horizontal row. Keep all opaque pixels inside their cell, including wings. Pixel coordinates begin at the upper left. No text, branding, shadows outside cells, or audio assets. The renderer draws at integer scales with nearest-neighbor sampling.

| Filename in `assets/` | Cells left to right | Frames | Frame duration | Loop |
| --- | --- | --- | --- | --- |
| `toaster.png` | wings high; wings halfway down; wings low; wings halfway up | 4 | 140 ms each | 560 ms |
| `toast-light.png` | same wing poses, pale center | 4 | 140 ms each | 560 ms |
| `toast-golden.png` | same wing poses, golden center | 4 | 140 ms each | 560 ms |
| `toast-dark.png` | same wing poses, dark center | 4 | 140 ms each | 560 ms |

The toaster's body is approximately x=13…34, y=17…35, with wings occupying x=3…12 and x=35…44. Keep the body anchored across frames. Its two short top slots and the coral lever should remain readable at 1×. Toast uses approximately x=15…32, y=14…35 with its crown reaching y=11. Both point toward the lower left, with a distinctive two-pixel cyan glint on their front-left edges. Wings are deliberately small, rounded paddles rather than large bird wings. No baked-in motion trails. Flap frames are evenly spaced in time; each flyer starts at a random phase.

## Palette

Use this shared palette (transparent pixels have alpha 0; all visible pixels alpha 255):

| Purpose | Hex |
| --- | --- |
| Deep outline | `#17182F` |
| Chrome shadow | `#667590` |
| Chrome body | `#B8CAD9` |
| Chrome highlight / wings | `#F1F5E8` |
| Cyan glint | `#55E6E0` |
| Violet dial | `#9F72E5` |
| Coral lever | `#FF807D` |
| Toast crust | `#A95735` |
| Light center | `#F7E4AF` |
| Golden center | `#E9B866` |
| Dark center | `#9A6541` |
| Toast freckles | `#76452F` |

The procedural backdrop uses deep blue `#090D27`, purple `#401355`, magenta `#D641A0`, and cyan `#55E6E0`. The sunset variant uses orange `#FFAA60` and pink `#FF659B`. Background colors are guidance for contrast, not additional sprite-sheet requirements.

## Icon

`resources/toaster.ico`: original toaster from the first frame, centered on a dark violet square. Supply 16, 32, 48, and 256 pixel sizes in one ICO. Preserve hard pixel edges when scaling. The initial icon and sheets are generated deterministically by `tools/generate_assets.py` using only Python's standard library. These are original temporary production assets; Jeremy can replace them at any time.

## Integration and acceptance

- Replace the four PNGs at the specified paths and rebuild to embed the new art inside the standalone `.scr`. No C++ change is needed.
- For immediate local art iteration, place replacement PNGs in an `assets` folder beside `ToasterSquadron.scr`. Valid external sheets override embedded art, one sheet at a time; invalid/missing sheets fall back to embedded art.
- Keep exact sheet dimensions and filenames. Test all doneness choices, 1× preview, and larger full-screen rendering; there should be no frame bleed or halos.
- Chrome, wings, and toast must be readable against both dark sky and bright sun. Do not use semitransparent edge smoothing.
- A barrel roll is a renderer rotation of the existing frames, so no additional art is required. Toast ejection and additional cosmetic animations are optional future work, not extra required sheets.
- The gradient sky, striped sun, scrolling perspective grid, pixel stars, and distant mountains are procedural Direct2D drawing. Do not deliver backdrop images.

Jeremy: replace these placeholder sheets when ready and test the Windows CI artifact. A release/tag waits for your explicit approval after that test.
