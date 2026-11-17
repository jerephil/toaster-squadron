#include "renderer.h"
#include "resource.h"
#include <d2d1_1helper.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace ts {
namespace {
D2D1_COLOR_F Color(UINT32 rgb, float alpha = 1.0f) { return D2D1::ColorF(rgb, alpha); }
constexpr float Pi = 3.14159265358979323846f;
}
Renderer::Renderer(HWND window, Settings settings)
    : window_(window), settings_(settings), random_(std::random_device{}()) {
    settings_.Clamp();
    stars_.reserve(90);
    for (int i = 0; i < 90; ++i) stars_.push_back({Random(0, 1), Random(0, 0.59f), Random(0.25f, 0.85f)});
}
float Renderer::Random(float low, float high) { return std::uniform_real_distribution<float>(low, high)(random_); }
HRESULT Renderer::Initialize() {
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                  nullptr, 0, D3D11_SDK_VERSION, &d3d_, nullptr, &d3dContext_);
    if (FAILED(hr)) hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                         nullptr, 0, D3D11_SDK_VERSION, &d3d_, nullptr, &d3dContext_);
    if (FAILED(hr)) return hr;
    D2D1_FACTORY_OPTIONS options{};
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options,
                          reinterpret_cast<void**>(factory_.GetAddressOf()));
    if (FAILED(hr)) return hr;
    ComPtr<IDXGIDevice> dxgi;
    hr = d3d_.As(&dxgi);
    if (FAILED(hr)) return hr;
    // I keep each output's queue short and wait for vblank only on the pacing output.
    ComPtr<IDXGIDevice1> dxgi1;
    if (SUCCEEDED(dxgi.As(&dxgi1))) dxgi1->SetMaximumFrameLatency(1);
    hr = factory_->CreateDevice(dxgi.Get(), &device_);
    if (FAILED(hr)) return hr;
    hr = device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &context_);
    if (FAILED(hr)) return hr;
    context_->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    context_->SetDpi(96, 96);
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> dxgiFactory;
    hr = dxgi->GetAdapter(&adapter);
    if (FAILED(hr)) return hr;
    hr = adapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) return hr;
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    hr = dxgiFactory->CreateSwapChainForHwnd(d3d_.Get(), window_, &desc, nullptr, nullptr, &swapchain_);
    if (FAILED(hr)) return hr;
    dxgiFactory->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER);
    hr = context_->CreateSolidColorBrush(Color(0xffffff), &brush_);
    if (FAILED(hr)) return hr;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic_));
    if (FAILED(hr)) return hr;
    constexpr const wchar_t* names[] = {L"toaster.png", L"toast-light.png", L"toast-golden.png", L"toast-dark.png"};
    constexpr int ids[] = {IDB_TOASTER, IDB_LIGHT, IDB_GOLDEN, IDB_DARK};
    for (size_t i = 0; i < sprites_.size(); ++i) {
        hr = LoadSprite(names[i], ids[i], sprites_[i].GetAddressOf());
        if (FAILED(hr)) return hr;
    }
    return Resize();
}
HRESULT Renderer::Decode(IWICBitmapDecoder* decoder, ID2D1Bitmap** result) {
    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return hr;
    UINT width = 0, height = 0;
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr) || width != 192 || height != 48) return E_INVALIDARG;
    ComPtr<IWICFormatConverter> converter;
    hr = wic_->CreateFormatConverter(&converter);
    if (FAILED(hr)) return hr;
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
                               nullptr, 0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return hr;
    const auto properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                                                  D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
    return static_cast<ID2D1RenderTarget*>(context_.Get())->CreateBitmapFromWicBitmap(converter.Get(), &properties, result);
}
HRESULT Renderer::LoadSprite(const wchar_t* filename, int resource, ID2D1Bitmap** result) {
    wchar_t module[32768]{};
    const DWORD length = GetModuleFileNameW(nullptr, module, static_cast<DWORD>(std::size(module)));
    ComPtr<IWICBitmapDecoder> decoder;
    if (length > 0 && length < std::size(module)) {
        const auto path = std::filesystem::path(module).parent_path() / L"assets" / filename;
        if (SUCCEEDED(wic_->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnLoad, &decoder)) &&
            SUCCEEDED(Decode(decoder.Get(), result))) return S_OK;
        decoder.Reset();
    }
    const auto instance = GetModuleHandleW(nullptr);
    const auto info = FindResourceW(instance, MAKEINTRESOURCEW(resource), RT_RCDATA);
    if (!info) return HRESULT_FROM_WIN32(GetLastError());
    const auto loaded = LoadResource(instance, info);
    auto* bytes = static_cast<BYTE*>(LockResource(loaded));
    const DWORD size = SizeofResource(instance, info);
    if (!bytes || !size) return E_FAIL;
    ComPtr<IWICStream> stream;
    HRESULT hr = wic_->CreateStream(&stream);
    if (FAILED(hr)) return hr;
    hr = stream->InitializeFromMemory(bytes, size);
    if (FAILED(hr)) return hr;
    hr = wic_->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) return hr;
    return Decode(decoder.Get(), result);
}
HRESULT Renderer::Resize() {
    RECT client{};
    GetClientRect(window_, &client);
    const UINT w = static_cast<UINT>(std::max<LONG>(1, client.right));
    const UINT h = static_cast<UINT>(std::max<LONG>(1, client.bottom));
    if (w == width_ && h == height_ && target_) return S_OK;
    context_->SetTarget(nullptr);
    target_.Reset();
    HRESULT hr = swapchain_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return hr;
    ComPtr<IDXGISurface> surface;
    hr = swapchain_->GetBuffer(0, IID_PPV_ARGS(&surface));
    if (FAILED(hr)) return hr;
    const auto properties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96, 96);
    hr = context_->CreateBitmapFromDxgiSurface(surface.Get(), &properties, &target_);
    if (FAILED(hr)) return hr;
    context_->SetTarget(target_.Get());
    width_ = w; height_ = h;
    scale_ = std::clamp(static_cast<int>(std::min(w / 640, h / 360)), 1, 8);
    const bool sunset = settings_.palette == 1;
    const D2D1_GRADIENT_STOP stops[] = {
        {0.0f, Color(0x090d27)}, {0.4f, Color(0x401355)},
        {0.67f, Color(sunset ? 0x87315f : 0x8c246d)}, {1.0f, Color(0x160d32)}
    };
    ComPtr<ID2D1GradientStopCollection> collection;
    hr = static_cast<ID2D1RenderTarget*>(context_.Get())->CreateGradientStopCollection(stops, static_cast<UINT32>(std::size(stops)), &collection);
    if (FAILED(hr)) return hr;
    sky_.Reset();
    hr = context_->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(
        D2D1::Point2F(0, 0), D2D1::Point2F(0, static_cast<float>(h))), collection.Get(), &sky_);
    if (FAILED(hr)) return hr;
    hr = BuildBackdrop();
    if (FAILED(hr)) return hr;
    ResetFlyers();
    return S_OK;
}
HRESULT Renderer::BuildBackdrop() {
    backdropLayer_.Reset();
    if (settings_.backdrop == 2) return S_OK;
    // I use the same base scale as my 48x48 sprites so backdrop and sprite pixels match in size.
    const auto pixelScale = static_cast<UINT>(scale_);
    const int baseW = static_cast<int>((width_ + pixelScale - 1) / pixelScale);
    const int baseH = static_cast<int>((height_ + pixelScale - 1) / pixelScale);
    layout_ = MakeBackdropLayout(baseW, baseH);
    gridRows_.reserve(18 * 3);
    const auto pixels = RasterizeBackdrop(layout_, settings_.backdrop, settings_.palette);
    const auto properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                                                  D2D1_ALPHA_MODE_PREMULTIPLIED), 96, 96);
    return static_cast<ID2D1RenderTarget*>(context_.Get())->CreateBitmap(
        D2D1::SizeU(static_cast<UINT32>(layout_.width), static_cast<UINT32>(layout_.height)), pixels.data(),
        static_cast<UINT32>(layout_.width) * 4, properties, &backdropLayer_);
}
void Renderer::ResetFlyers() {
    flyers_.clear();
    flyers_.reserve(static_cast<size_t>(settings_.flyers));
    // I round the toast population to the nearest integer and keep it fixed between resets.
    const int toastCount = (settings_.flyers * settings_.toastRatio + 50) / 100;
    const float w = static_cast<float>(width_), h = static_cast<float>(height_);
    for (int i = 0; i < settings_.flyers; ++i) {
        flyers_.push_back({Random(-48.0f, w), Random(-48.0f, h), Random(0.8f, 1.25f),
                           Random(0, 0.56f), animationTime_ + Random(8, 80), i < toastCount});
    }
    std::shuffle(flyers_.begin(), flyers_.end(), random_);
}
void Renderer::Rect(float x0, float y0, float x1, float y1, D2D1_COLOR_F color) {
    brush_->SetColor(color);
    context_->FillRectangle(D2D1::RectF(x0, y0, x1, y1), brush_.Get());
}
void Renderer::Line(float x0, float y0, float x1, float y1, D2D1_COLOR_F color, float width) {
    brush_->SetColor(color);
    context_->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush_.Get(), width);
}
void Renderer::Backdrop(double elapsed) {
    const float w = static_cast<float>(width_), h = static_cast<float>(height_);
    context_->SetTransform(D2D1::Matrix3x2F::Identity());
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    if (settings_.backdrop == 2) context_->Clear(Color(0x090d27));
    else context_->FillRectangle(D2D1::RectF(0, 0, w, h), sky_.Get());
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    for (const auto& star : stars_) {
        const float x = std::floor(star.x * w), y = std::floor(star.y * h * (settings_.backdrop == 2 ? 1.0f / 0.59f : 1.0f));
        const float size = static_cast<float>(std::max(1, scale_ / 2));
        Rect(x, y, x + size, y + size, Color(0xe0d9ff, star.brightness));
    }
    if (settings_.backdrop == 2 || !backdropLayer_) return;
    // I upscale the pixel-art sun, halo, ridges, floor, and converging grid lines without smoothing.
    const float s = static_cast<float>(scale_);
    const auto dest = D2D1::RectF(0, 0, static_cast<float>(layout_.width) * s, static_cast<float>(layout_.height) * s);
    static_cast<ID2D1RenderTarget*>(context_.Get())->DrawBitmap(backdropLayer_.Get(), &dest, 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    if (settings_.backdrop == 1) return;
    // I snap scrolling grid rows to whole base pixels so my grid and sprite pixels move consistently.
    const double gridRate = settings_.gridSpeed == 0 ? 0 : settings_.gridSpeed == 1 ? 0.045 : 0.12;
    const float phase = static_cast<float>(std::fmod(elapsed * gridRate, 1.0));
    ScrollingGridRows(layout_, phase, settings_.palette, gridRows_);
    for (const auto& row : gridRows_) {
        Rect(0, static_cast<float>(row.y) * s, w, static_cast<float>(row.y + 1) * s, Color(row.rgb, row.alpha));
    }
}
void Renderer::DrawFlyers(float delta) {
    animationTime_ += delta;
    const float w = static_cast<float>(width_), h = static_cast<float>(height_);
    const float size = static_cast<float>(48 * scale_);
    const float speed = (settings_.speed == 0 ? 22.0f : settings_.speed == 1 ? 43.0f : 76.0f) * scale_;
    context_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    for (auto& flyer : flyers_) {
        flyer.x -= speed * flyer.speed * delta;
        flyer.y += speed * flyer.speed * 0.48f * delta;
        if (flyer.x < -size || flyer.y > h + size) {
            // I weight incoming flyers by edge length and velocity.
            if (Random(0, 1) < h / (h + w * 0.48f)) {
                flyer.x = w + size * 0.1f;
                flyer.y = Random(-size, h);
            } else {
                flyer.x = Random(-size, w);
                flyer.y = -size;
            }
        }
        float rotation = 0;
        if (!flyer.toast && animationTime_ >= flyer.rollAt) {
            const float progress = static_cast<float>((animationTime_ - flyer.rollAt) / 1.4);
            if (progress < 1) rotation = 360 * (progress - std::sin(progress * 2 * Pi) / (2 * Pi));
            else flyer.rollAt = animationTime_ + Random(35, 100);
        }
        const float x = std::floor(flyer.x), y = std::floor(flyer.y);
        const auto center = D2D1::Point2F(x + size / 2, y + size / 2);
        context_->SetTransform(D2D1::Matrix3x2F::Rotation(rotation, center));
        const int frame = static_cast<int>(std::fmod((animationTime_ + flyer.phase) / 0.14, 4.0));
        const int sheet = flyer.toast ? settings_.doneness + 1 : 0;
        const auto source = D2D1::RectF(static_cast<float>(frame * 48), 0, static_cast<float>((frame + 1) * 48), 48);
        const auto dest = D2D1::RectF(x, y, x + size, y + size);
        static_cast<ID2D1RenderTarget*>(context_.Get())->DrawBitmap(sprites_[sheet].Get(), &dest, 1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &source);
    }
    context_->SetTransform(D2D1::Matrix3x2F::Identity());
}
HRESULT Renderer::Render(double elapsed, float delta, bool synchronize) {
    HRESULT hr = Resize();
    if (FAILED(hr)) return hr;
    context_->BeginDraw();
    Backdrop(elapsed);
    DrawFlyers(delta);
    hr = context_->EndDraw();
    if (FAILED(hr)) return hr;
    return swapchain_->Present(synchronize ? 1 : 0, 0);
}
} // namespace ts
