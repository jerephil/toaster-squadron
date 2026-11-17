# Handoff: pixel-art sprites and synthwave backdrop

> Historical Claude Design handoff, retained for provenance. The Windows build and tests subsequently passed, the screenshot was refreshed, and Jeremy approved stable v0.1.0 after successful Windows 11 testing on November 17, 2026. The pending checks and release hold below describe the original handoff; see [TESTING.md](TESTING.md) for current release status.

Owner: Jeremy Phillips. Repo: `jerephil/toaster-squadron`. Branch: `pixel-art-backdrop`, based on `main`.

This handoff covers two changes:

1. Final sprite art replaces the placeholder sheets.
2. The sun, ridges and grid floor now draw as chunky pixel art at the same pixel size as the flyers. They used to be smooth antialiased vector shapes.

Everything is in one commit, delivered as `0001-pixel-art-sprites-and-synthwave-backdrop.patch`.

## Apply

```powershell
git checkout main
git pull
git checkout -b pixel-art-backdrop
git am --3way 0001-pixel-art-sprites-and-synthwave-backdrop.patch
```

The patch includes binary PNG diffs. Use `git am`, not `git apply` on a hand-edited copy.

## What changed

| Path | Change |
|---|---|
| `assets/toaster.png` | New art. Chrome toaster, 4 wing-flap frames. |
| `assets/toast-light.png` | New art. Light toast, 4 frames. |
| `assets/toast-golden.png` | New art. Golden toast, 4 frames. |
| `assets/toast-dark.png` | New art. Dark toast, 4 frames. |
| `src/pixel_backdrop.h/.cpp` | New. Portable CPU rasterizer for the sun, halo, ridges, floor, converging grid lines and horizon. Also computes the scrolling grid rows. No Windows APIs. |
| `src/renderer.h/.cpp` | `BuildBackdrop()` rasterizes the layer on resize and uploads it as an `ID2D1Bitmap`. `Backdrop()` draws the sky and stars as before, then the layer with nearest-neighbor scaling, then the scrolling rows as snapped rects. The old procedural sun, ridge and grid drawing is removed. |
| `tests/backdrop_tests.cpp` | New portable test. Registered in CTest as `pixel_backdrop`. |
| `CMakeLists.txt` | Adds `src/pixel_backdrop.cpp` to the saver and the `backdrop_tests` target. |
| `tools/sprite_art.py` | Generator for the four sprite sheets. Needs Pillow. |
| `tools/check_sprites.py` | Spec checker for the sheets. Also writes a contrast preview. |
| `tools/preview_backdrop.cpp`, `tools/preview_scene.py` | Off-Windows preview of the full scene. |
| `docs/TESTING.md` | Step 5 and the architecture notes describe the pixel backdrop. |

`resources/toaster.rc` embeds the four PNGs from `assets/`, so a rebuild picks up the new art with no resource changes.

## Sprite spec (must stay true)

- 4 files, exact names above. Each is 192 x 48, 8-bit RGBA, static PNG.
- 4 x 1 grid of 48 x 48 frames. No padding or gutters. Frame origins are x = 0, 48, 96, 144.
- Frame order: 0 wings high, 1 halfway down, 2 wings low, 3 halfway up. Playback is 140 ms per frame. `renderer.cpp` already does this.
- Every visible pixel is at least 2 px inside its cell.
- Only the wings move. Body, eyes, dial, lever and toast crown are identical across frames.
- The three toast sheets share shape and motion. Only the center color and freckle count change: light 2, golden 5, dark 8.
- Opaque pixels use only the 12 brief colors: `#17182F #667590 #B8CAD9 #F1F5E8 #55E6E0 #9F72E5 #FF807D #A95735 #F7E4AF #E9B866 #9A6541 #76452F`. Alpha is only 0 or 255.
- `Renderer::Decode` rejects any sheet that is not 192 x 48. That falls back to the embedded copy.

To change sprite art, edit `tools/sprite_art.py` and run:

```bash
pip install pillow
python tools/sprite_art.py assets
python tools/check_sprites.py assets sprite-preview.png
```

`check_sprites.py` must print `ALL OK`. Its preview shows each sheet on the dark sky and the orange sun.

Main sprite knobs in `sprite_art.py`:

- `WING_ANGLES` sets the four flap poses.
- `wing_mask(a, b, reach)` sets paddle size and distance from the body. Larger values can push frames 1 and 3 past the 2 px margin. Run the checker after any change.
- `toaster_frame()` sets the toaster body box, slots, glint, eyes, dial and lever positions.
- `toast_shape()` sets the crown lobes. `FRECKLES` sets the freckle pixels per doneness.

Do not run `tools/generate_assets.py`. It writes the old placeholders over the real art.

## Backdrop design

The layer is rasterized at base resolution: `ceil(screen / scale_)`, where `scale_` is the existing sprite scale (`min(w/640, h/360)`, clamped 1..8). At 1080p that is 640 x 360 drawn at 3x. One backdrop pixel equals one sprite pixel on screen.

Layout and colors in base pixels:

- Horizon row: `round(0.64 * height)`.
- Sun radius: `min(0.19 * width, 0.23 * height)`. Center x is the screen middle. Center y is `horizon - 0.72 * radius`. These match the old geometry.
- Sun color bands: 12 bands. Bands 0-3 are `#FFD385`, 4-7 are `#FFAA60`, 8-11 are `#FF659B`. A 2-row checkerboard dither marks each color change.
- Sun cuts: transparent rows at the bottom of bands 4-11, 1,1,1,2,2,2,3,3 rows tall. The sky shows through.
- Halo: three hard-edged `#FF659B` discs at radius +3, +6, +10, alpha 0.11, 0.08, 0.06.
- Ridges: Bresenham wireframe peaks in `#D641A0`, same positions as before. Alpha 0.55 for the outer lines, 0.3 for the inner line.
- Floor: solid `#090D27` below the horizon.
- Converging lines: 21 lines, accent mixed 70% over the floor color.
- Horizon line: `#FF659B` in classic, `#FFAA60` in sunset.
- Scrolling rows: 18 rows, same speed constants as before (0.045 slow, 0.12 fast). Each row snaps to a whole base pixel. Brightness steps through alpha 0.35, 0.6 and 0.85 by depth. Near rows get a one-pixel glow above and below at alpha 0.18.
- Accent: `#55E6E0` classic, `#FF659B` sunset.

Modes follow `settings_.backdrop`:

- 0 is full synthwave.
- 1 is sun only. It shows the full disc with no floor, as before.
- 2 is the starfield. No layer is built.

The sky gradient and stars are unchanged. The sky is still a smooth gradient.

Performance: the layer is built once per resize. Per frame, the cost is one bitmap draw plus up to 54 rect fills. `gridRows_` is reused, so normal frames do no heap allocation.

## Verification done

- `backdrop_tests` passes under GCC with AddressSanitizer and UBSan. It covers layout, opaque floor, horizon color, all sun bands, at least 5 stripe cuts, grid accent per palette, sun-only mode, row bounds and ordering, premultiplied alpha, and tiny sizes down to 1 x 1.
- `src/renderer.cpp` and `src/pixel_backdrop.cpp` pass a MinGW-w64 syntax check against Windows SDK headers with `-Wall -Wextra`. No errors or warnings.
- Scene previews were rendered with `tools/preview_scene.py` at 1280 x 720 and 1920 x 1080, in both palettes and sun-only mode.
- `check_sprites.py` prints `ALL OK` for all four sheets.

## Not verified yet (T3 to do)

1. **MSVC build and CTest.** This work was done off Windows. Push the branch and confirm the GitHub Actions Windows build is green. Both `screensaver_contract` and `pixel_backdrop` must pass. If MSVC reports a `/W4` warning in the new code, fix it. Do not weaken the warning level.
2. **Real Windows run.** Work through `docs/TESTING.md`, focusing on step 5. Check every backdrop mode, palette and grid speed. Check that the backdrop pixel size matches the flyers at 100%, 150% and 200% DPI, and on a 4K monitor (scale 6).
3. **Odd resolutions.** On a size that is not a multiple of `scale_` (1366 x 768, ultrawide), the layer overhangs the right and bottom edges by less than one base pixel. That is expected. Confirm there is no seam or gap.
4. **Device loss and resize.** Code review shows this is covered. `main.cpp` (around line 212) rebuilds the whole `Renderer` on device loss. `Initialize()` then calls `Resize()`, which calls `BuildBackdrop()`. A size change also rebuilds the layer. Confirm this on hardware with lock/unlock, sleep/wake and a GPU driver reset if practical.
5. **Screenshot.** `docs/screenshot.png` still shows the old vector backdrop and placeholder sprites. Let the CI capture step regenerate it, or capture one on Windows 11.
6. **Sprite review on hardware.** At 1x scale (screens under 1280 x 720, and the Screen Saver Settings preview box), confirm the flyers still read clearly. The `#17182F` outline blends into the dark sky, so the fills carry the silhouette there.

## Optional follow-ups (only if Jeremy asks)

- Band the sky into 6 to 8 hard color steps with dither so it matches the pixel art. This would be a change to `Resize()` and the sky brush, or done by drawing the sky into the layer.
- Make the star size follow the base pixel. Today it is `max(1, scale_ / 2)` screen pixels.
- Remove `Renderer::Line`. It is now unused, but it is harmless.

## Rules carried from the project

- Keep all art original. Do not imitate existing screensaver characters, sprites, branding or layouts.
- No audio.
- Do not create a tag or a GitHub Release. Jeremy approves `v0.1.0` after the Windows checklist in `docs/TESTING.md`.
- The license stays MIT.
