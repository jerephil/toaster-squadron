#pragma once
// I rasterize my chunky synthwave sun, halo, ridges, and grid floor on the CPU
// at my sprites' base scale. I draw the result with nearest-neighbor scaling
// to match my 48x48 flyers pixel for pixel. I keep this code independent of
// Windows so I can build and test it with any C++20 compiler.
#include <cstdint>
#include <vector>

namespace ts {
struct BackdropLayout {
    int width = 0;    // I store this dimension in base pixels.
    int height = 0;   // I store this dimension in base pixels.
    int horizon = 0;  // I store the horizon row, with the floor below it.
};

// I round screen dimensions divided by my sprite scale up to obtain width/height in base pixels.
BackdropLayout MakeBackdropLayout(int width, int height);

// I select full synthwave with mode 0 or sun only with mode 1.
// I select classic cyan with palette 0 or sunset pink with palette 1.
// I return width*height premultiplied pixels as A<<24 | R<<16 | G<<8 | B,
// stored as B,G,R,A on little-endian systems (DXGI_FORMAT_B8G8R8A8_UNORM).
std::vector<std::uint32_t> RasterizeBackdrop(const BackdropLayout& layout, int mode, int palette);

// I draw each scrolling horizontal grid row over my static layer every frame.
struct GridRow {
    int y;              // I store this row in base pixels.
    std::uint32_t rgb;  // I store this color as 0xRRGGBB.
    float alpha;        // I blend with this opacity over existing pixels.
};

// I emit glow rows before bright rows for a scroll phase in [0,1).
// I clear and refill `out` to reuse my buffer without allocating every frame.
void ScrollingGridRows(const BackdropLayout& layout, float phase, int palette, std::vector<GridRow>& out);

std::uint32_t GridAccent(int palette);
} // namespace ts
