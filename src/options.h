#pragma once
#include <cstdint>
#include <cwctype>
#include <limits>
#include <span>
#include <string_view>

namespace ts {
enum class Mode { Settings, Fullscreen, Preview, Invalid };
struct Options { Mode mode = Mode::Settings; std::uintptr_t parent = 0; };

inline Options ParseOptions(std::span<const std::wstring_view> args) {
    if (args.empty()) return {};
    const auto arg = args.front();
    if (arg.size() < 2 || (arg[0] != L'/' && arg[0] != L'-')) return {Mode::Invalid};
    const wchar_t flag = static_cast<wchar_t>(std::towlower(arg[1]));
    const Mode mode = flag == L's' ? Mode::Fullscreen : flag == L'p' ? Mode::Preview :
                      flag == L'c' ? Mode::Settings : Mode::Invalid;
    if (mode == Mode::Invalid || args.size() > 2) return {Mode::Invalid};
    std::wstring_view handle;
    if (arg.size() > 2) {
        if (arg[2] != L':' || args.size() != 1) return {Mode::Invalid};
        handle = arg.substr(3);
        if (handle.empty()) return {Mode::Invalid};
    } else if (args.size() == 2) {
        handle = args[1];
    }
    if (mode == Mode::Fullscreen) return handle.empty() ? Options{mode} : Options{Mode::Invalid};
    if (handle.empty()) return mode == Mode::Settings ? Options{} : Options{Mode::Invalid};
    std::uintptr_t value = 0;
    for (const wchar_t c : handle) {
        if (c < L'0' || c > L'9') return {Mode::Invalid};
        const auto digit = static_cast<std::uintptr_t>(c - L'0');
        if (value > (std::numeric_limits<std::uintptr_t>::max() - digit) / 10) return {Mode::Invalid};
        value = value * 10 + digit;
    }
    if (value == 0) return {Mode::Invalid};
    return {mode, value};
}
} // namespace ts
