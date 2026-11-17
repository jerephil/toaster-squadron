#pragma once
#include "settings.h"
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <array>
#include <random>
#include <vector>

namespace ts {
class Renderer {
public:
    Renderer(HWND window, Settings settings);
    HRESULT Initialize();
    HRESULT Render(double elapsed, float delta, bool synchronize);
private:
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    struct Flyer { float x, y, speed, phase, rollAt; bool toast; };
    struct Star { float x, y, brightness; };
    HWND window_;
    Settings settings_;
    UINT width_ = 0, height_ = 0;
    int scale_ = 1;
    float animationTime_ = 0;
    std::mt19937 random_;
    std::vector<Flyer> flyers_;
    std::vector<Star> stars_;
    ComPtr<ID3D11Device> d3d_;
    ComPtr<ID3D11DeviceContext> d3dContext_;
    ComPtr<ID2D1Factory1> factory_;
    ComPtr<ID2D1Device> device_;
    ComPtr<ID2D1DeviceContext> context_;
    ComPtr<IDXGISwapChain1> swapchain_;
    ComPtr<ID2D1Bitmap1> target_;
    ComPtr<ID2D1SolidColorBrush> brush_;
    ComPtr<ID2D1LinearGradientBrush> sky_;
    ComPtr<IWICImagingFactory> wic_;
    std::array<ComPtr<ID2D1Bitmap>, 4> sprites_;
    HRESULT Resize();
    HRESULT LoadSprite(const wchar_t* filename, int resource, ID2D1Bitmap** result);
    HRESULT Decode(IWICBitmapDecoder* decoder, ID2D1Bitmap** result);
    void ResetFlyers();
    float Random(float low, float high);
    void Backdrop(double elapsed);
    void DrawFlyers(float delta);
    void Line(float x0, float y0, float x1, float y1, D2D1_COLOR_F color, float width = 1);
    void Rect(float x0, float y0, float x1, float y1, D2D1_COLOR_F color);
};
} // namespace ts
