#include "settings.h"
#include "resource.h"
#include <commctrl.h>
#include <initializer_list>
#include <string>

namespace ts {
namespace {
constexpr wchar_t Key[] = L"Software\\ToasterSquadron";
struct Field { const wchar_t* name; int Settings::* member; int maximum; };
constexpr Field Fields[] = {
    {L"Flyers", &Settings::flyers, 40}, {L"Speed", &Settings::speed, 2},
    {L"ToastRatio", &Settings::toastRatio, 100}, {L"Doneness", &Settings::doneness, 2},
    {L"Backdrop", &Settings::backdrop, 2}, {L"GridSpeed", &Settings::gridSpeed, 2},
    {L"Palette", &Settings::palette, 1}
};
void Combo(HWND dialog, int id, std::initializer_list<const wchar_t*> items, int selected) {
    for (auto item : items) SendDlgItemMessageW(dialog, id, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    SendDlgItemMessageW(dialog, id, CB_SETCURSEL, selected, 0);
}
int Selected(HWND dialog, int id) { return static_cast<int>(SendDlgItemMessageW(dialog, id, CB_GETCURSEL, 0, 0)); }
int Slider(HWND dialog, int id) { return static_cast<int>(SendDlgItemMessageW(dialog, id, TBM_GETPOS, 0, 0)); }
void UpdateLabels(HWND dialog) {
    const auto flyers = std::to_wstring(Slider(dialog, IDC_FLYERS));
    const auto ratio = std::to_wstring(Slider(dialog, IDC_RATIO)) + L"%";
    SetDlgItemTextW(dialog, IDC_FLYERS_VALUE, flyers.c_str());
    SetDlgItemTextW(dialog, IDC_RATIO_VALUE, ratio.c_str());
}
INT_PTR CALLBACK DialogProc(HWND dialog, UINT msg, WPARAM wp, LPARAM) {
    if (msg == WM_INITDIALOG) {
        const auto settings = LoadSettings();
        SendMessageW(dialog, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_TOASTER))));
        SendDlgItemMessageW(dialog, IDC_FLYERS, TBM_SETRANGE, TRUE, MAKELPARAM(5, 40));
        SendDlgItemMessageW(dialog, IDC_FLYERS, TBM_SETTICFREQ, 5, 0);
        SendDlgItemMessageW(dialog, IDC_FLYERS, TBM_SETPOS, TRUE, settings.flyers);
        SendDlgItemMessageW(dialog, IDC_RATIO, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendDlgItemMessageW(dialog, IDC_RATIO, TBM_SETTICFREQ, 10, 0);
        SendDlgItemMessageW(dialog, IDC_RATIO, TBM_SETPOS, TRUE, settings.toastRatio);
        Combo(dialog, IDC_SPEED, {L"Slow", L"Medium", L"Fast"}, settings.speed);
        Combo(dialog, IDC_DONENESS, {L"Light", L"Golden", L"Dark"}, settings.doneness);
        Combo(dialog, IDC_BACKDROP, {L"Full synthwave (grid + sun)", L"Sun only", L"Plain dark starfield"}, settings.backdrop);
        Combo(dialog, IDC_GRID, {L"Off", L"Slow", L"Fast"}, settings.gridSpeed);
        Combo(dialog, IDC_PALETTE, {L"Classic synthwave", L"Sunset"}, settings.palette);
        UpdateLabels(dialog);
        return TRUE;
    }
    if (msg == WM_HSCROLL) { UpdateLabels(dialog); return TRUE; }
    if (msg == WM_COMMAND) {
        if (LOWORD(wp) == IDOK) {
            Settings settings;
            settings.flyers = Slider(dialog, IDC_FLYERS);
            settings.toastRatio = Slider(dialog, IDC_RATIO);
            settings.speed = Selected(dialog, IDC_SPEED);
            settings.doneness = Selected(dialog, IDC_DONENESS);
            settings.backdrop = Selected(dialog, IDC_BACKDROP);
            settings.gridSpeed = Selected(dialog, IDC_GRID);
            settings.palette = Selected(dialog, IDC_PALETTE);
            settings.Clamp();
            if (!SaveSettings(settings)) {
                MessageBoxW(dialog, L"Settings could not be saved to your user registry. Please try again.", L"Toaster Squadron", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wp) == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    }
    if (msg == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    return FALSE;
}
} // namespace
Settings LoadSettings() {
    Settings settings;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, Key, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
        for (const auto& field : Fields) {
            DWORD value = 0;
            DWORD size = sizeof(value);
            if (RegGetValueW(key, nullptr, field.name, RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS &&
                value <= static_cast<DWORD>(field.maximum)) settings.*(field.member) = static_cast<int>(value);
        }
        RegCloseKey(key);
    }
    settings.Clamp();
    return settings;
}
bool SaveSettings(const Settings& original) {
    auto settings = original;
    settings.Clamp();
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, Key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    bool ok = true;
    for (const auto& field : Fields) {
        const DWORD value = static_cast<DWORD>(settings.*(field.member));
        if (RegSetValueExW(key, field.name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value)) != ERROR_SUCCESS) ok = false;
    }
    RegCloseKey(key);
    return ok;
}
INT_PTR ShowSettings(HINSTANCE instance, HWND owner) {
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_BAR_CLASSES};
    InitCommonControlsEx(&controls);
    return DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_SETTINGS), owner, DialogProc, 0);
}
} // namespace ts
