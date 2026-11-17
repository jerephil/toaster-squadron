#include "options.h"
#include "renderer.h"
#include "resource.h"
#include "settings.h"
#include <shellapi.h>
#include <chrono>
#include <cmath>
#include <memory>
#include <string_view>
#include <vector>

namespace ts {
namespace {
using Clock = std::chrono::steady_clock;
struct Application;
struct Window {
    Application* app = nullptr;
    HWND handle = nullptr;
    std::unique_ptr<Renderer> renderer;
};
struct Application {
    bool preview = false;
    HWND parent = nullptr;
    Settings settings;
    ULONGLONG started = GetTickCount64();
    std::vector<std::unique_ptr<Window>> windows;
};
constexpr wchar_t WindowClass[] = L"ToasterSquadron.Window";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    auto* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lp);
        window = static_cast<Window*>(create->lpCreateParams);
        window->handle = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    if (!window) return DefWindowProcW(hwnd, msg, wp, lp);
    const bool preview = window->app->preview;
    switch (msg) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        BeginPaint(hwnd, &paint);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_SETCURSOR:
        if (!preview) { SetCursor(nullptr); return TRUE; }
        break;
    case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP:
    case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN: case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
        if (!preview) PostQuitMessage(0);
        return 0;
    case WM_ACTIVATEAPP:
        if (!preview && !wp && GetTickCount64() - window->app->started > 1500) PostQuitMessage(0);
        break;
    case WM_DPICHANGED:
        if (!preview) {
            MONITORINFO monitor{sizeof(monitor)};
            if (GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
                const auto& rect = monitor.rcMonitor;
                SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left,
                             rect.bottom - rect.top, SWP_NOACTIVATE);
            }
        }
        return 0;
    case WM_DISPLAYCHANGE:
        // Let Windows restart the saver with fresh monitor bounds after hotplug.
        if (!preview) PostQuitMessage(0);
        return 0;
    case WM_POWERBROADCAST:
        if (wp == PBT_APMSUSPEND) PostQuitMessage(0);
        return TRUE;
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_SCREENSAVE) return 0;
        break;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
struct Monitor { RECT bounds; bool primary; };
BOOL CALLBACK CollectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    MONITORINFO info{sizeof(info)};
    if (GetMonitorInfoW(monitor, &info)) {
        auto& monitors = *reinterpret_cast<std::vector<Monitor>*>(data);
        Monitor entry{info.rcMonitor, (info.dwFlags & MONITORINFOF_PRIMARY) != 0};
        if (entry.primary) monitors.insert(monitors.begin(), entry);
        else monitors.push_back(entry);
    }
    return TRUE;
}
bool AddWindow(Application& app, HINSTANCE instance, RECT rect) {
    auto window = std::make_unique<Window>();
    window->app = &app;
    const DWORD style = app.preview ? WS_CHILD | WS_VISIBLE : WS_POPUP | WS_VISIBLE;
    const DWORD exStyle = app.preview ? WS_EX_NOACTIVATE : WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    const auto hwnd = CreateWindowExW(exStyle, WindowClass, L"Toaster Squadron", style,
        rect.left, rect.top, std::max<LONG>(1, rect.right - rect.left), std::max<LONG>(1, rect.bottom - rect.top),
        app.parent, nullptr, instance, window.get());
    if (!hwnd) return false;
    window->renderer = std::make_unique<Renderer>(hwnd, app.settings);
    if (FAILED(window->renderer->Initialize())) {
        DestroyWindow(hwnd);
        return false;
    }
    app.windows.push_back(std::move(window));
    return true;
}
struct CursorGuard {
    int hides = 0;
    explicit CursorGuard(bool hide) {
        if (hide) { do { ++hides; } while (ShowCursor(FALSE) >= 0); }
    }
    ~CursorGuard() { while (hides-- > 0) ShowCursor(TRUE); }
};
int Run(HINSTANCE instance, Options options) {
    Application app;
    app.preview = options.mode == Mode::Preview;
    app.parent = reinterpret_cast<HWND>(options.parent);
    app.settings = LoadSettings();
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_TOASTER));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = WindowClass;
    if (!RegisterClassExW(&wc)) return 2;
    bool created = true;
    if (app.preview) {
        RECT rect{};
        if (!IsWindow(app.parent) || !GetClientRect(app.parent, &rect)) return 2;
        created = AddWindow(app, instance, rect);
    } else {
        std::vector<Monitor> monitors;
        EnumDisplayMonitors(nullptr, nullptr, CollectMonitor, reinterpret_cast<LPARAM>(&monitors));
        if (monitors.empty()) return 2;
        for (const auto& monitor : monitors) {
            if (!AddWindow(app, instance, monitor.bounds)) { created = false; break; }
        }
    }
    if (!created) {
        for (const auto& window : app.windows) if (IsWindow(window->handle)) DestroyWindow(window->handle);
        return 2;
    }
    CursorGuard cursor(!app.preview);
    if (!app.preview) SetForegroundWindow(app.windows.front()->handle);
    // A one-shot waitable timer caps high-refresh screens at 60 fps, while
    // DXGI Present(1) synchronizes the primary output. Other windows use the
    // composited desktop without serial waits for each monitor's vblank.
    HANDLE timer = CreateWaitableTimerW(nullptr, FALSE, nullptr);
    if (!timer) {
        for (const auto& window : app.windows) DestroyWindow(window->handle);
        return 2;
    }
    const auto start = Clock::now();
    auto previous = start;
    auto nextFrame = start;
    POINT anchor{};
    bool anchored = false;
    bool running = true;
    int exitCode = 0;
    while (running) {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!running) break;
        if (app.preview) {
            RECT rect{};
            if (!IsWindow(app.parent) || !GetClientRect(app.parent, &rect)) break;
            RECT current{};
            GetClientRect(app.windows.front()->handle, &current);
            if (rect.right != current.right || rect.bottom != current.bottom) {
                SetWindowPos(app.windows.front()->handle, nullptr, 0, 0, std::max<LONG>(1, rect.right),
                             std::max<LONG>(1, rect.bottom), SWP_NOACTIVATE | SWP_NOZORDER);
            }
        } else if (GetTickCount64() - app.started > 1200) {
            POINT position{};
            if (GetCursorPos(&position)) {
                if (!anchored) { anchor = position; anchored = true; }
                if (std::abs(position.x - anchor.x) > 8 || std::abs(position.y - anchor.y) > 8) break;
            }
        }
        const auto now = Clock::now();
        if (now < nextFrame) {
            const auto wait = std::chrono::duration_cast<std::chrono::nanoseconds>(nextFrame - now).count();
            LARGE_INTEGER due{};
            due.QuadPart = -std::max<LONGLONG>(1, wait / 100);
            SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE);
            MsgWaitForMultipleObjectsEx(1, &timer, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            continue;
        }
        const double elapsed = std::chrono::duration<double>(now - start).count();
        const float delta = std::clamp(std::chrono::duration<float>(now - previous).count(), 0.0f, 0.1f);
        previous = now;
        nextFrame = now + std::chrono::microseconds(16667);
        // Draw secondary monitors first, then the output that supplies pacing.
        for (size_t n = 0; n < app.windows.size(); ++n) {
            const size_t index = (n + 1) % app.windows.size();
            auto& window = app.windows[index];
            if (IsIconic(window->handle)) continue;
            const HRESULT hr = window->renderer->Render(elapsed, delta, index == 0);
            if (FAILED(hr)) {
                // Release every resource owned by the removed device before recreation.
                window->renderer.reset();
                window->renderer = std::make_unique<Renderer>(window->handle, app.settings);
                if (FAILED(window->renderer->Initialize())) { exitCode = 2; running = false; break; }
            }
        }
    }
    CancelWaitableTimer(timer);
    CloseHandle(timer);
    for (const auto& window : app.windows) {
        window->renderer.reset();
        if (IsWindow(window->handle)) DestroyWindow(window->handle);
    }
    UnregisterClassW(WindowClass, instance);
    return exitCode;
}
} // namespace
} // namespace ts

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    // The embedded PerMonitorV2 manifest establishes awareness before any HWND.
    int argc = 0;
    auto** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return 2;
    std::vector<std::wstring_view> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    const auto options = ts::ParseOptions(args);
    LocalFree(argv);
    if (options.mode == ts::Mode::Invalid) return 2;
    if (options.parent && !IsWindow(reinterpret_cast<HWND>(options.parent))) return 2;
    if (options.mode == ts::Mode::Settings) {
        return ts::ShowSettings(instance, reinterpret_cast<HWND>(options.parent)) == -1 ? 2 : 0;
    }
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(com)) return 2;
    int result = 2;
    try { result = ts::Run(instance, options); }
    catch (...) { OutputDebugStringW(L"Toaster Squadron: initialization or rendering failed.\n"); }
    CoUninitialize();
    return result;
}
