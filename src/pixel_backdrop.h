#pragma once
// Chunky pixel-art synthwave backdrop (sun, halo, ridges, grid floor).
// Rasterized on the CPU at the sprites' base scale, then drawn with
// nearest-neighbor scaling so it matches the 48x48 flyers pixel for pixel.
// No Windows dependencies: this file builds and tests on any C++20 compiler.
#include <cstdint>
#include <vector>

namespace ts {
struct BackdropLayout {
    int width = 0;    // base pixels
    int height = 0;   // base pixels
    int horizon = 0;  // row of the horizon line; floor starts below it
};

// width/height are in base pixels (screen size divided by the sprite scale, rounded up).
BackdropLayout MakeBackdropLayout(int width, int height);

// mode: 0 = full synthwave (sun + ridges + grid floor), 1 = sun only.
// palette: 0 = classic (cyan grid), 1 = sunset (pink grid).
// Returns width*height premultiplied pixels, value = A<<24 | R<<16 | G<<8 | B,
// which is B,G,R,A in memory on little-endian (DXGI_FORMAT_B8G8R8A8_UNORM).
std::vector<std::uint32_t> RasterizeBackdrop(const BackdropLayout& layout, int mode, int palette);

// One scrolling horizontal grid row, drawn per frame over the static layer.
struct GridRow {
    int y;              // base-pixel row
    std::uint32_t rgb;  // 0xRRGGBB
    float alpha;        // blend over what is already there
};

// Rows for a scroll phase in [0,1). Glow rows come first so the bright rows land on top.
// Refills `out` (cleared first) so the renderer can reuse one buffer with no per-frame allocation.
void ScrollingGridRows(const BackdropLayout& layout, float phase, int palette, std::vector<GridRow>& out);

std::uint32_t GridAccent(int palette);
} // namespace ts
