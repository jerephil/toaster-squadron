# Toaster Squadron

My personal, free Windows 11 screensaver: original chunky pixel-art winged toasters and toast drift diagonally across a neon sunset and scrolling perspective grid. C++20, Win32, Direct2D; no audio or external runtime to install.

**In development.** [The art brief](docs/ART-BRIEF.md) is ready for my art workflow; the first Windows build is coming next. Screenshots/GIF: TODO — capture the running Windows build after PC testing.

Toaster Squadron is an independent, original project and is not affiliated with or endorsed by Berkeley Systems or the makers of After Dark.

## Install and test

Download `ToasterSquadron.scr` from a green GitHub Actions build. Right-click it → **Install**, then use Windows Screen Saver Settings to preview and configure it. Alternatively copy it to `C:\Windows\System32` (administrator permission required). An unsigned download may trigger SmartScreen; inspect and build the source yourself if preferred.

- `ToasterSquadron.scr /s`: full screen on every monitor; exits on keyboard, click, or deliberate mouse movement.
- `ToasterSquadron.scr /p <HWND>`: live child preview for Windows Screen Saver Settings.
- `ToasterSquadron.scr /c` or no arguments: settings.

No version tag or Release will be created until I try the CI artifact on my Windows 11 PC and approve it.

## Planned settings

Flyers (5–40, default 15), slow/medium/fast speed, toast ratio (0–100%, default 30%), light/golden/dark toast, full synthwave/sun only/dark starfield, grid speed (off/slow/fast), and classic synthwave/sunset palette. Settings are per-user under `HKCU\Software\ToasterSquadron`.

## Build

Windows 11, Windows SDK, CMake 3.24+, and the Visual Studio 2022 C++ desktop toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The planned output is `build\Release\ToasterSquadron.scr`. Windows SDK libraries are the only application dependencies; Release uses the static MSVC runtime. CI uses public GitHub-hosted Windows runners only.

## Art and license

See [docs/ART-BRIEF.md](docs/ART-BRIEF.md) for the swappable PNG contract. All art is original; the initial sprites are code-generated placeholders. MIT licensed, copyright Jeremy W. Phillips; see [LICENSE](LICENSE).
