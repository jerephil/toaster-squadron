// I dump my backdrop layer and scroll rows for a supplied base size, mode, palette, and phase.
#include "pixel_backdrop.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv) {
    const int w = std::atoi(argv[1]), h = std::atoi(argv[2]);
    const int mode = std::atoi(argv[3]), palette = std::atoi(argv[4]);
    const float phase = static_cast<float>(std::atof(argv[5]));
    const auto layout = ts::MakeBackdropLayout(w, h);
    const auto px = ts::RasterizeBackdrop(layout, mode, palette);
    FILE* f = std::fopen(argv[6], "wb");
    std::fwrite(px.data(), 4, px.size(), f);
    std::fclose(f);
    std::printf("%d %d %d\n", layout.width, layout.height, layout.horizon);
    std::vector<ts::GridRow> rows;
    if (mode == 0) ts::ScrollingGridRows(layout, phase, palette, rows);
    for (const auto& r : rows)
            std::printf("%d %06x %.3f\n", r.y, r.rgb, r.alpha);
}
