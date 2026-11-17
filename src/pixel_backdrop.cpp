#include "pixel_backdrop.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace ts {
namespace {
constexpr std::uint32_t Floor = 0x090d27;
constexpr std::uint32_t SunTop = 0xffd385;
constexpr std::uint32_t SunMid = 0xffaa60;
constexpr std::uint32_t SunLow = 0xff659b;
constexpr std::uint32_t Halo = 0xff659b;
constexpr std::uint32_t Ridge = 0xd641a0;
constexpr int SunBands = 12;

int Channel(std::uint32_t c, int shift) { return static_cast<int>((c >> shift) & 0xff); }

std::uint32_t Mix(std::uint32_t over, std::uint32_t under, float alpha) {
    std::uint32_t out = 0;
    for (int shift = 0; shift <= 16; shift += 8) {
        const float v = Channel(over, shift) * alpha + Channel(under, shift) * (1 - alpha);
        out |= static_cast<std::uint32_t>(std::clamp(static_cast<int>(std::lround(v)), 0, 255)) << shift;
    }
    return out;
}

class Canvas {
public:
    Canvas(int w, int h) : w_(w), h_(h), px_(static_cast<size_t>(w) * static_cast<size_t>(h), 0) {}
    void Set(int x, int y, std::uint32_t rgb) {
        if (Inside(x, y)) px_[Index(x, y)] = 0xff000000u | rgb;
    }
    // Source-over blend of a straight-alpha color onto premultiplied pixels.
    void Blend(int x, int y, std::uint32_t rgb, float alpha) {
        if (!Inside(x, y)) return;
        std::uint32_t& d = px_[Index(x, y)];
        const float inv = 1 - alpha;
        std::uint32_t out = 0;
        for (int shift = 0; shift <= 16; shift += 8) {
            const float v = Channel(rgb, shift) * alpha + Channel(d, shift) * inv;
            out |= static_cast<std::uint32_t>(std::clamp(static_cast<int>(std::lround(v)), 0, 255)) << shift;
        }
        const float a = 255 * alpha + Channel(d, 24) * inv;
        out |= static_cast<std::uint32_t>(std::clamp(static_cast<int>(std::lround(a)), 0, 255)) << 24;
        d = out;
    }
    void Span(int x0, int x1, int y, std::uint32_t rgb) {  // x1 exclusive
        for (int x = std::max(0, x0); x < std::min(w_, x1); ++x) Set(x, y, rgb);
    }
    void Disc(float cx, float cy, float r, std::uint32_t rgb, float alpha, int maxRow) {
        const int top = static_cast<int>(std::floor(cy - r));
        const int bottom = std::min(maxRow, static_cast<int>(std::ceil(cy + r)));
        for (int y = top; y < bottom; ++y) {
            const float dy = y + 0.5f - cy;
            if (std::fabs(dy) > r) continue;
            const int half = static_cast<int>(std::lround(std::sqrt(r * r - dy * dy)));
            const int c = static_cast<int>(std::lround(cx));
            for (int x = c - half; x < c + half; ++x) Blend(x, y, rgb, alpha);
        }
    }
    void Line(int x0, int y0, int x1, int y1, std::uint32_t rgb, float alpha) {  // Bresenham
        const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        for (;;) {
            Blend(x0, y0, rgb, alpha);
            if (x0 == x1 && y0 == y1) break;
            const int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
    std::vector<std::uint32_t> Take() { return std::move(px_); }
private:
    bool Inside(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }
    size_t Index(int x, int y) const { return static_cast<size_t>(y) * static_cast<size_t>(w_) + static_cast<size_t>(x); }
    int w_, h_;
    std::vector<std::uint32_t> px_;
};

std::uint32_t BandColor(int band) { return band < 4 ? SunTop : band < 8 ? SunMid : SunLow; }

void DrawSun(Canvas& canvas, const BackdropLayout& l, bool clipAtHorizon) {
    const float r = std::min(l.width * 0.19f, l.height * 0.23f);
    const float cx = std::floor(l.width * 0.5f);
    const float cy = std::floor(l.horizon - r * 0.72f) + 0.5f;
    const int maxRow = clipAtHorizon ? l.horizon : l.height;

    // Stepped halo: hard-edged rings, no smooth glow.
    canvas.Disc(cx, cy, r + 10, Halo, 0.06f, maxRow);
    canvas.Disc(cx, cy, r + 6, Halo, 0.08f, maxRow);
    canvas.Disc(cx, cy, r + 3, Halo, 0.11f, maxRow);

    const int top = static_cast<int>(std::floor(cy - r));
    const int bottom = static_cast<int>(std::ceil(cy + r));
    const int rows = bottom - top;
    std::vector<int> band(static_cast<size_t>(rows));
    for (int i = 0; i < rows; ++i) {
        const float t = (top + i + 0.5f - (cy - r)) / (2 * r);
        band[static_cast<size_t>(i)] = std::clamp(static_cast<int>(t * SunBands), 0, SunBands - 1);
    }
    for (int i = 0; i < rows; ++i) {
        const int y = top + i;
        if (y >= maxRow) break;
        const float dy = y + 0.5f - cy;
        if (std::fabs(dy) > r) continue;
        const int b = band[static_cast<size_t>(i)];
        // Dark cuts widen toward the base: 1,1,1,2,2,2,3,3 rows for bands 4..11.
        if (b >= 4) {
            int left = 0;
            while (i + left + 1 < rows && band[static_cast<size_t>(i + left + 1)] == b) ++left;
            const int gap = 1 + (b - 4) / 3;
            if (left < gap) continue;
        }
        const int half = static_cast<int>(std::lround(std::sqrt(r * r - dy * dy)));
        const int c = static_cast<int>(cx);
        const int next = i + 1 < rows ? band[static_cast<size_t>(i + 1)] : b;
        const int prev = i > 0 ? band[static_cast<size_t>(i - 1)] : b;
        for (int x = c - half; x < c + half; ++x) {
            std::uint32_t col = BandColor(b);
            // Two-row checkerboard dither where the colors change (yellow->orange, orange->pink).
            if (((x + y) & 1) != 0) {
                if (BandColor(next) != col) col = BandColor(next);
                else if (BandColor(prev) != col) col = BandColor(prev);
            }
            canvas.Set(x, y, col);
        }
    }
}

void DrawRidges(Canvas& canvas, const BackdropLayout& l) {
    const float w = static_cast<float>(l.width), h = static_cast<float>(l.height);
    for (int i = -1; i < 11; ++i) {
        const float x = w * i / 10, next = x + w / 10;
        const float s = std::sin(i * 2.7f);
        const int peakX = static_cast<int>(std::lround(x + w * 0.046f));
        const int peakY = static_cast<int>(std::lround(l.horizon - h * (0.035f + 0.025f * s * s)));
        canvas.Line(static_cast<int>(std::lround(x)), l.horizon, peakX, peakY, Ridge, 0.55f);
        canvas.Line(peakX, peakY, static_cast<int>(std::lround(next)), l.horizon, Ridge, 0.55f);
        canvas.Line(peakX, peakY, static_cast<int>(std::lround(x + w * 0.065f)), l.horizon, Ridge, 0.3f);
    }
}

void DrawFloor(Canvas& canvas, const BackdropLayout& l, int palette) {
    for (int y = l.horizon; y < l.height; ++y) canvas.Span(0, l.width, y, Floor);
    const std::uint32_t line = Mix(GridAccent(palette), Floor, 0.7f);
    const float cx = std::floor(l.width * 0.5f);
    const float depth = static_cast<float>(l.height - l.horizon);
    for (int i = -10; i <= 10; ++i) {
        const float bottomX = cx + i * l.width * 0.16f;
        for (int y = l.horizon + 1; y < l.height; ++y) {
            const float a = cx + (bottomX - cx) * (y - l.horizon) / depth;
            const float b = cx + (bottomX - cx) * (y + 1 - l.horizon) / depth;
            const int x0 = static_cast<int>(std::floor(std::min(a, b)));
            const int x1 = std::max(x0 + 1, static_cast<int>(std::floor(std::max(a, b))));
            canvas.Span(x0, x1, y, line);
        }
    }
    // Horizon line.
    canvas.Span(0, l.width, l.horizon, palette == 1 ? SunMid : SunLow);
}
} // namespace

std::uint32_t GridAccent(int palette) { return palette == 1 ? 0xff659b : 0x55e6e0; }

BackdropLayout MakeBackdropLayout(int width, int height) {
    BackdropLayout l;
    l.width = std::max(1, width);
    l.height = std::max(1, height);
    l.horizon = std::clamp(static_cast<int>(std::lround(l.height * 0.64f)), 0, l.height - 1);
    return l;
}

std::vector<std::uint32_t> RasterizeBackdrop(const BackdropLayout& layout, int mode, int palette) {
    Canvas canvas(layout.width, layout.height);
    const bool full = mode == 0;
    DrawSun(canvas, layout, full);
    if (full) {
        DrawRidges(canvas, layout);
        DrawFloor(canvas, layout, palette);
    }
    return canvas.Take();
}

void ScrollingGridRows(const BackdropLayout& l, float phase, int palette, std::vector<GridRow>& out) {
    constexpr int Count = 18;
    const std::uint32_t accent = GridAccent(palette);
    out.clear();
    // Pass 0 emits glow rows, pass 1 the bright rows, so bright rows draw last.
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < Count; ++i) {
            const float depth = std::fmod(static_cast<float>(i) / Count + phase, 1.0f);
            const int y = l.horizon + static_cast<int>(std::floor((l.height - l.horizon) * std::pow(depth, 2.8f)));
            if (y <= l.horizon || y >= l.height) continue;
            if (pass == 1) {
                // Three stepped brightness levels instead of a smooth fade.
                const float alpha = depth < 0.35f ? 0.35f : depth < 0.7f ? 0.6f : 0.85f;
                out.push_back({y, accent, alpha});
            } else if (depth >= 0.7f) {
                if (y - 1 > l.horizon) out.push_back({y - 1, accent, 0.18f});
                if (y + 1 < l.height) out.push_back({y + 1, accent, 0.18f});
            }
        }
    }
}
} // namespace ts
