// I test my pixel-art backdrop rasterizer without Windows APIs.
#include "pixel_backdrop.h"
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::uint32_t At(const std::vector<std::uint32_t>& px, const ts::BackdropLayout& l, int x, int y) {
    return px[static_cast<size_t>(y) * static_cast<size_t>(l.width) + static_cast<size_t>(x)];
}
int Alpha(std::uint32_t p) { return static_cast<int>(p >> 24); }
std::uint32_t Rgb(std::uint32_t p) { return p & 0xffffff; }

void Premultiplied(const std::vector<std::uint32_t>& px) {
    for (auto p : px) {
        const int a = Alpha(p);
        Check(static_cast<int>((p >> 16) & 0xff) <= a && static_cast<int>((p >> 8) & 0xff) <= a &&
              static_cast<int>(p & 0xff) <= a, "pixel is not premultiplied");
    }
}

void FullScene(int palette) {
    const auto l = ts::MakeBackdropLayout(640, 360);
    Check(l.horizon == 230, "horizon should sit at 64% of the height");
    const auto px = ts::RasterizeBackdrop(l, 0, palette);
    Check(px.size() == 640u * 360u, "layer size");
    Premultiplied(px);
    for (int y = l.horizon; y < l.height; ++y)
        for (int x = 0; x < l.width; ++x) Check(Alpha(At(px, l, x, y)) == 255, "floor must be opaque");
    Check(Rgb(At(px, l, 0, l.horizon)) == (palette == 1 ? 0xffaa60u : 0xff659bu), "horizon line color");

    // I check my sun column for all three band colors and at least five dark cuts above the horizon.
    const int cx = l.width / 2;
    std::set<std::uint32_t> colors;
    int cuts = 0;
    bool inSun = false;
    for (int y = 0; y < l.horizon; ++y) {
        const auto p = At(px, l, cx, y);
        if (Alpha(p) == 255) { colors.insert(Rgb(p)); inSun = true; }
        else if (inSun && Alpha(At(px, l, cx, y - 1)) == 255) ++cuts;
    }
    Check(colors.count(0xffd385) && colors.count(0xffaa60) && colors.count(0xff659b), "sun bands");
    Check(cuts >= 5, "sun needs dark stripe cuts");
    // I check that my center grid line reaches the bottom row in the palette accent.
    const auto bottom = Rgb(At(px, l, cx, l.height - 1));
    Check(bottom != 0x090d27, "center grid line missing");
    Check(palette == 1 ? ((bottom >> 16) & 0xff) > 0xb0 : (bottom & 0xff) > 0xa0, "grid accent");
}

void SunOnly() {
    const auto l = ts::MakeBackdropLayout(640, 360);
    const auto px = ts::RasterizeBackdrop(l, 1, 0);
    Premultiplied(px);
    Check(Alpha(At(px, l, 0, l.height - 1)) == 0, "sun-only has no floor");
    Check(Alpha(At(px, l, l.width / 2, l.horizon + 5)) == 255 ||
          Alpha(At(px, l, l.width / 2, l.horizon + 6)) == 255, "sun-only keeps the full disc");
}

void Rows() {
    const auto l = ts::MakeBackdropLayout(640, 360);
    std::vector<ts::GridRow> rows;
    for (float phase : {0.0f, 0.25f, 0.5f, 0.999f}) {
        ts::ScrollingGridRows(l, phase, 0, rows);
        Check(!rows.empty() && rows.size() <= 18u * 3u, "row count");
        bool seenMain = false;
        for (const auto& r : rows) {
            Check(r.y > l.horizon && r.y < l.height, "row out of floor");
            Check(r.alpha > 0 && r.alpha <= 1, "row alpha");
            if (r.alpha > 0.3f) seenMain = true;
            else Check(!seenMain, "glow rows must come before bright rows");
        }
    }
}

void TinySizes() {
    for (int w : {1, 2, 7, 33})
        for (int h : {1, 2, 5, 19}) {
            const auto l = ts::MakeBackdropLayout(w, h);
            Check(ts::RasterizeBackdrop(l, 0, 1).size() == static_cast<size_t>(w) * static_cast<size_t>(h), "tiny size");
            std::vector<ts::GridRow> rows;
            ts::ScrollingGridRows(l, 0.5f, 1, rows);
        }
}
} // namespace

int main() {
    try {
        FullScene(0);
        FullScene(1);
        SunOnly();
        Rows();
        TinySizes();
    } catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << "\n";
        return 1;
    }
    std::cout << "pixel backdrop tests passed\n";
    return 0;
}
