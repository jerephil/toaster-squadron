#pragma once
#include <windows.h>
#include <algorithm>

namespace ts {
struct Settings {
    int flyers = 15;
    int speed = 1;
    int toastRatio = 30;
    int doneness = 1;
    int backdrop = 0;
    int gridSpeed = 1;
    int palette = 0;
    void Clamp() {
        flyers = std::clamp(flyers, 5, 40);
        speed = std::clamp(speed, 0, 2);
        toastRatio = std::clamp(toastRatio, 0, 100);
        doneness = std::clamp(doneness, 0, 2);
        backdrop = std::clamp(backdrop, 0, 2);
        gridSpeed = std::clamp(gridSpeed, 0, 2);
        palette = std::clamp(palette, 0, 1);
    }
};
Settings LoadSettings();
bool SaveSettings(const Settings& settings);
INT_PTR ShowSettings(HINSTANCE instance, HWND owner);
} // namespace ts
