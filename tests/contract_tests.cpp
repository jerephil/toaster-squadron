#include "options.h"
#include "settings.h"
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Pump() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
}
struct Process {
    PROCESS_INFORMATION info{};
    Process(const std::wstring& executable, const std::wstring& args) {
        auto command = L"\"" + executable + L"\" " + args;
        STARTUPINFOW startup{sizeof(startup)};
        Check(CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &info) != FALSE, "CreateProcess failed");
    }
    ~Process() {
        if (info.hProcess) {
            if (WaitForSingleObject(info.hProcess, 0) == WAIT_TIMEOUT) {
                TerminateProcess(info.hProcess, 99);
                WaitForSingleObject(info.hProcess, 2000);
            }
            CloseHandle(info.hProcess); CloseHandle(info.hThread);
        }
    }
    void Exited(DWORD expected = 0) {
        Check(WaitForSingleObject(info.hProcess, 7000) == WAIT_OBJECT_0, "Process did not exit");
        DWORD code = 99;
        GetExitCodeProcess(info.hProcess, &code);
        Check(code == expected, "Unexpected process exit code");
    }
};
struct Search { DWORD pid; std::vector<HWND> windows; };
BOOL CALLBACK Found(HWND hwnd, LPARAM data) {
    auto& search = *reinterpret_cast<Search*>(data);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == search.pid && IsWindowVisible(hwnd)) search.windows.push_back(hwnd);
    return TRUE;
}
std::vector<HWND> Windows(Process& process, HWND parent = nullptr) {
    const ULONGLONG end = GetTickCount64() + 10000;
    do {
        Pump();
        Search search{process.info.dwProcessId, {}};
        if (parent) EnumChildWindows(parent, Found, reinterpret_cast<LPARAM>(&search));
        else EnumWindows(Found, reinterpret_cast<LPARAM>(&search));
        if (!search.windows.empty()) return search.windows;
        Check(WaitForSingleObject(process.info.hProcess, 20) == WAIT_TIMEOUT, "Application exited before creating a window");
    } while (GetTickCount64() < end);
    throw std::runtime_error("No application window appeared");
}
void ParserTests() {
    using namespace std::literals;
    using ts::Mode;
    auto parse = [](std::initializer_list<std::wstring_view> args) { return ts::ParseOptions({args.begin(), args.size()}); };
    Check(parse({}).mode == Mode::Settings, "Default mode");
    Check(parse({L"/S"sv}).mode == Mode::Fullscreen, "Case insensitive mode");
    Check(parse({L"/c"sv}).mode == Mode::Settings, "Settings mode");
    Check(parse({L"/p"sv, L"12345"sv}).parent == 12345, "Spaced HWND");
    Check(parse({L"/P:4294967296"sv}).parent == 4294967296ULL, "64-bit colon HWND");
    Check(parse({L"/c:25"sv}).mode == Mode::Settings, "Owner HWND");
    for (const auto arg : {L"/p", L"/p:0", L"/p:-5", L"/p:12abc", L"/p:18446744073709551616", L"/s:2", L"/c:", L"/surprise", L"help"})
        Check(parse({arg}).mode == Mode::Invalid, "Malformed option accepted");
    Check(parse({L"/s", L"123"}).mode == Mode::Invalid, "Unexpected full-screen argument accepted");
    Check(parse({L"/p:1", L"2"}).mode == Mode::Invalid, "Duplicate HWND accepted");
    ts::Settings settings;
    settings.flyers = 100; settings.toastRatio = -1; settings.speed = 42; settings.palette = 9;
    settings.Clamp();
    Check(settings.flyers == 40 && settings.toastRatio == 0 && settings.speed == 2 && settings.palette == 1, "Settings clamp");
}
void DialogTest(const std::wstring& executable, const std::wstring& args) {
    Process process(executable, args);
    auto windows = Windows(process);
    wchar_t name[64]{};
    GetClassNameW(windows.front(), name, 64);
    Check(std::wstring(name) == L"#32770", "Settings did not create a native dialog");
    PostMessageW(windows.front(), WM_COMMAND, IDCANCEL, 0);
    process.Exited();
}
void PreviewTest(const std::wstring& executable) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"ToasterSquadron.TestParent";
    Check(RegisterClassW(&wc) != 0, "Register test parent");
    HWND parent = CreateWindowExW(0, wc.lpszClassName, L"Preview contract test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  10, 10, 400, 260, nullptr, nullptr, wc.hInstance, nullptr);
    Check(parent != nullptr, "Create preview parent");
    {
        Process process(executable, L"/p " + std::to_wstring(reinterpret_cast<std::uintptr_t>(parent)));
        const auto child = Windows(process, parent).front();
        Check(GetParent(child) == parent, "Preview is not a child");
        Check((GetWindowLongPtrW(child, GWL_STYLE) & WS_CHILD) != 0, "Missing child style");
        PostMessageW(child, WM_KEYDOWN, VK_SPACE, 0);
        SetWindowPos(parent, nullptr, 0, 0, 540, 340, SWP_NOMOVE | SWP_NOZORDER);
        const ULONGLONG end = GetTickCount64() + 2000;
        bool resized = false;
        do {
            Pump();
            RECT outer{}, inner{};
            GetClientRect(parent, &outer); GetClientRect(child, &inner);
            resized = outer.right == inner.right && outer.bottom == inner.bottom;
            if (resized) break;
            Check(WaitForSingleObject(process.info.hProcess, 20) == WAIT_TIMEOUT, "Preview exited on keyboard input");
        } while (GetTickCount64() < end);
        Check(resized, "Preview did not follow parent size");
        Check(WaitForSingleObject(process.info.hProcess, 500) == WAIT_TIMEOUT, "Preview exited early (WIC/D2D initialization)");
        DestroyWindow(parent);
        process.Exited();
    }
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
BOOL CALLBACK CountMonitor(HMONITOR, HDC, LPRECT, LPARAM data) { ++*reinterpret_cast<int*>(data); return TRUE; }
void MouseTest(const std::wstring& executable) {
    struct RestoreCursor {
        POINT point{};
        RestoreCursor() { GetCursorPos(&point); }
        ~RestoreCursor() { SetCursorPos(point.x, point.y); }
    } restore;
    Process process(executable, L"/s");
    const auto windows = Windows(process);
    const auto bounds = [] {
        RECT rect{};
        GetWindowRect(GetDesktopWindow(), &rect);
        return rect;
    }();
    const int x = (bounds.left + bounds.right) / 2;
    const int y = (bounds.top + bounds.bottom) / 2;
    SetCursorPos(x, y);
    Check(WaitForSingleObject(process.info.hProcess, 1600) == WAIT_TIMEOUT, "Saver exited during mouse settling");
    SetCursorPos(x + 4, y + 4);
    Check(WaitForSingleObject(process.info.hProcess, 150) == WAIT_TIMEOUT, "Small mouse jitter exited saver");
    SetCursorPos(x + 24, y + 24);
    process.Exited();
}
void FullscreenTest(const std::wstring& executable, UINT input) {
    Process process(executable, L"/s");
    auto windows = Windows(process);
    int monitors = 0;
    EnumDisplayMonitors(nullptr, nullptr, CountMonitor, reinterpret_cast<LPARAM>(&monitors));
    // Window creation and GPU initialization happen one output at a time.
    const ULONGLONG end = GetTickCount64() + 10000;
    while (static_cast<int>(windows.size()) < monitors && GetTickCount64() < end) {
        Pump();
        WaitForSingleObject(process.info.hProcess, 20);
        windows = Windows(process);
    }
    Check(static_cast<int>(windows.size()) == monitors, "Window count does not match monitors");
    for (const auto window : windows) {
        Check((GetWindowLongPtrW(window, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0, "Saver is not topmost");
        Check((GetWindowLongPtrW(window, GWL_STYLE) & WS_CAPTION) == 0, "Saver is not borderless");
        MONITORINFO monitor{sizeof(monitor)};
        GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor);
        RECT rect{}; GetWindowRect(window, &rect);
        Check(EqualRect(&rect, &monitor.rcMonitor) != FALSE, "Saver does not fill its monitor");
    }
    Check(WaitForSingleObject(process.info.hProcess, 700) == WAIT_TIMEOUT, "Saver exited early (embedded sprites/render startup)");
    PostMessageW(windows.front(), input, input == WM_KEYDOWN ? VK_SPACE : 0, 0);
    process.Exited();
}
} // namespace
int wmain(int argc, wchar_t** argv) {
    try {
        Check(argc == 2, "Expected screensaver path");
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        ParserTests();
        const std::wstring executable = argv[1];
        DialogTest(executable, L"/c");
        DialogTest(executable, L"");
        PreviewTest(executable);
        FullscreenTest(executable, WM_KEYDOWN);
        FullscreenTest(executable, WM_LBUTTONDOWN);
        MouseTest(executable);
        { Process invalid(executable, L"/p:0"); invalid.Exited(2); }
        std::cout << "PASS: parser, settings bounds, /c, default settings, live /p resize/lifecycle, /s monitor coverage and key/click/mouse exit.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
