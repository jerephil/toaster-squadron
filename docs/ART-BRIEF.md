# Toaster Squadron — Claude Design sprite brief

**Claude Design** supplied the new sprite sheets for Jeremy's personal, MIT-licensed Windows screensaver. This brief records their export contract and retains the copyable commission prompt for future art revisions. The checked-in sheets are the new art; do not run `tools/generate_assets.py`, which would overwrite them with old placeholders. The original placeholder icon stays in place.

Design a playful squadron of rounded retro chrome toasters and expressive toast with small, round, featherless wings. Give the toaster two short slots, a violet circular dial, a coral side lever, and a cyan front-left glint. Give toast a slightly lopsided cloud-shaped crown and tiny square eyes. Use chunky, deliberate 8-bit pixel clusters. Everything must be original: do not copy, trace, or imitate existing screensaver characters, sprites, silhouettes, color arrangements, branding, or layouts.

## Required files and export contract

These are the **exact case-sensitive filenames read by the current loader**, relative to the repo root:

| Output file | Subject | Frames | Frame size | PNG dimensions |
| --- | --- | --- | --- | --- |
| `assets/toaster.png` | Chrome winged toaster | 4 | 48 × 48 px | 192 × 48 px |
| `assets/toast-light.png` | Winged toast, light center | 4 | 48 × 48 px | 192 × 48 px |
| `assets/toast-golden.png` | Same toast, golden center | 4 | 48 × 48 px | 192 × 48 px |
| `assets/toast-dark.png` | Same toast, dark center | 4 | 48 × 48 px | 192 × 48 px |

For **every** sheet:

- Grid: **4 columns × 1 row**, in row-major order from left to right.
- Cell/frame: exactly **48 × 48 pixels**; sheet: exactly **192 × 48 pixels**.
- Outer sheet padding: **0 px**. Gutters/spacing between cells: **0 px**. No extrusion or duplicated edge pixels.
- Keep visible pixels at least **2 px inside each cell's edges**. This transparent safety margin is part of the 48 × 48 cell, not extra padding. No wing may spill into another cell.
- Export a **static PNG, 8-bit RGBA per channel (32-bit color), with a genuinely transparent background**. Visible pixels use alpha 255; transparent pixels use alpha 0. No antialiasing, translucent edge pixels, checkerboard baked into the image, background color, labels, or guides.
- Export at native pixel dimensions, at 1×. No upscaling, interpolation, blur, resampling, trimmed frames, auto-cropping, packed atlases, or metadata needed to locate frames.
- Deliver all four downloadable PNG files, not a screenshot, mockup, vector-only design, GIF, animated PNG, or single combined atlas. Preserve exact filenames, including hyphens.

## Frame order, alignment, and timing

Coordinates are zero-based, with the origin at the top-left of each sheet. All bounds below are inclusive.

| Frame index | Atlas pixel bounds (x, y) | Wing pose | Duration |
| --- | --- | --- | --- |
| 0 | x=0…47, y=0…47 | High/upstroke apex | 140 ms |
| 1 | x=48…95, y=0…47 | Halfway down | 140 ms |
| 2 | x=96…143, y=0…47 | Low/downstroke apex | 140 ms |
| 3 | x=144…191, y=0…47 | Halfway up | 140 ms |

The renderer loops **0 → 1 → 2 → 3 → 0**, for **560 ms per full flap**. Frame indices 1 and 3 are the transition poses; small differences in wing curvature can suggest direction. Keep the body, facial details, dial, lever, and overall center anchored across all four frames; animate the wings, not the body's position. Use the same timing and registration on every sheet. Keep the three toast variants identical except for doneness shading and freckles.

In each local 48 × 48 cell:

- Toaster body target: x=13…34, y=17…35; wings can use x=3…12 and x=35…44, within the 2 px safety margin.
- Toast body target: x=15…32, y=14…35, with the crown reaching y=11; wings stay inside the same cell safety margin.
- These body bounds guide composition; the sheet dimensions, cell dimensions, safety margin, and registration are mandatory.
- Suggest travel toward the lower left through an asymmetric front-left highlight and a little three-quarter form. Do not rotate or translate the frames to create travel: the renderer supplies motion and an occasional barrel roll.
- Wings should be compact, rounded paddles, not large feathered bird wings. Keep details readable at native 1× and integer nearest-neighbor scales. No baked-in glow, shadow outside the silhouette, motion trail, or backdrop.

## Shared hex palette

Use these opaque sprite colors consistently. The background is transparent, not a palette fill.

| Use | Hex |
| --- | --- |
| Deep outline / tiny eyes | `#17182F` |
| Chrome shadow | `#667590` |
| Chrome body | `#B8CAD9` |
| Chrome highlight / wings | `#F1F5E8` |
| Cyan glint | `#55E6E0` |
| Violet dial | `#9F72E5` |
| Coral lever | `#FF807D` |
| Toast crust | `#A95735` |
| Light toast center | `#F7E4AF` |
| Golden toast center | `#E9B866` |
| Dark toast center | `#9A6541` |
| Toast freckles | `#76452F` |

The procedural scene behind the sprites uses deep blue `#090D27`, purple `#401355`, magenta `#D641A0`, cyan `#55E6E0`, orange `#FFAA60`, and pink `#FF659B`. Use these as contrast references only. The sky gradient, striped sun, scrolling grid, mountains, and stars already come from Direct2D code; do not export background graphics. Do not create audio. The existing placeholder icon stays in place; no ICO export is required for this four-sheet commission.

## Drop-in integration and checks

Replace the four files in the repo's `assets/` directory and rebuild to embed the Claude Design art in the standalone `.scr`. **No C++ or resource-file change is needed.** For immediate testing without rebuilding, replace the four matching PNGs in the `assets/` folder beside `ToasterSquadron.scr` in the extracted artifact. The loader reads that folder relative to the executable, not the current working directory; valid external sheets override embedded art one at a time. Missing, unreadable, or incorrectly sized sheets fall back to the embedded placeholders.

Before delivery, verify all four PNGs are 192 × 48 RGBA images, all 16 frame cells have the intended pose and anchored bodies, and every transparent edge stays crisp. Test against both the dark sky and bright sun, including small preview rendering and all toast doneness settings. Do not run the placeholder generator over the replacement art. No tag or Release is authorized by this art handoff.

## Paste into Claude Design

```text
Create four original, production-ready pixel-art PNG sprite sheets for my personal MIT-licensed Windows screensaver, Toaster Squadron. I want Claude Design to make the final graphics. Existing code-generated graphics are placeholders only. Create the downloadable image files themselves, not just a design description or mockup.

ART DIRECTION
Playful 8-bit pixel art: a friendly rounded retro chrome toaster and expressive toast, both with compact round featherless paddle wings. The toaster has two short slots, a violet circular temperature dial, a coral side lever, and a cyan front-left glint. Toast has a slightly lopsided cloud-shaped crown, tiny square eyes, and freckles. Suggest lower-left travel with asymmetric front-left highlights and subtle three-quarter form; do not bake in travel, rotation, backgrounds, glow, or trails. All characters must be original. Do not copy, trace, or imitate existing screensaver characters, sprites, silhouettes, branding, color arrangements, or layouts. I intend to distribute the resulting art with the project's MIT license.

EXACT FILES
assets/toaster.png — chrome toaster, 4 wing-flap frames.
assets/toast-light.png — light toast, 4 wing-flap frames.
assets/toast-golden.png — golden toast, 4 wing-flap frames.
assets/toast-dark.png — dark toast, 4 wing-flap frames.
Use these exact lowercase filenames and hyphens. Each file must be a separate downloadable PNG that I can put in assets/ without changing any code.

EXACT LAYOUT FOR EVERY FILE
Sheet dimensions: 192 pixels wide × 48 pixels high.
Grid: 4 columns × 1 row.
Each frame/cell: 48 × 48 pixels.
Outer sheet padding: 0 pixels. Inter-frame gutters: 0 pixels. Edge extrusion: 0 pixels.
Keep every visible pixel at least 2 pixels inside its own cell's edges; that transparent safety margin is INSIDE the 48 × 48 cell. Wings must not cross cell boundaries. Do not trim, crop, pack, scale, or add metadata-dependent frame layouts.
Frame origins: (0,0), (48,0), (96,0), (144,0).
Frame 0: wings high/upstroke apex.
Frame 1: wings halfway down.
Frame 2: wings low/downstroke apex.
Frame 3: wings halfway up.
Playback is 0,1,2,3,repeat at 140 milliseconds per frame, a 560-millisecond loop. Keep bodies, eyes, dial, lever, and character centers registered identically across frames. Transition frames 1 and 3 can vary slightly in wing curvature. Keep the three toast variants aligned and identical in shape and motion; change only doneness shading and freckles.
Within each local cell, target a toaster body at x=13..34, y=17..35 with wings extending toward x=3..12 and x=35..44. Target toast at x=15..32, y=14..35 with its crown reaching y=11. These body bounds guide composition; exact cell dimensions, safety margin, and frame registration are mandatory.

OPAQUE HEX PALETTE
#17182F deep outline and eyes
#667590 chrome shadow
#B8CAD9 chrome body
#F1F5E8 chrome highlights and wings
#55E6E0 cyan glint
#9F72E5 violet dial
#FF807D coral lever
#A95735 toast crust
#F7E4AF light toast center
#E9B866 golden toast center
#9A6541 dark toast center
#76452F toast freckles
The background is transparent, not a solid palette color. For contrast reference only, the procedural scene behind the sprites uses #090D27 deep blue, #401355 purple, #D641A0 magenta, #55E6E0 cyan, #FFAA60 orange, and #FF659B pink. Keep silhouettes readable over both the dark sky and bright sun.

EXPORT AND DELIVERY
Export each sheet at native 1× size as a static 192 × 48 PNG, 8-bit RGBA per channel (32-bit color), with genuine transparency. Opaque art pixels must have alpha 255; transparent pixels alpha 0. Use crisp, deliberate pixel clusters and nearest-neighbor treatment only. No antialiasing, translucent edge smoothing, blur, interpolation, checkerboard baked into the image, labels, guides, background, or upscaled preview substituted for the source file.
Deliver exactly the four PNG sheets, each containing all four frames in the specified order. No combined atlas, GIF, animated PNG, vector-only file, or screenshot instead of the exports. No backdrop graphics, icon, audio, or code are needed. The renderer already supplies the sky, sun, grid, stars, mountains, movement, and barrel rolls. Confirm the filenames, 192 × 48 sheet dimensions, 4 × 1 grid, 48 × 48 cells, zero sheet padding/gutters, and transparent RGBA export before delivering.
```
