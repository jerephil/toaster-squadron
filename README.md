# Toaster Squadron

My personal, free Windows 11 screensaver: original chunky pixel-art winged toasters and toast drift diagonally across a neon sunset and scrolling perspective grid. C++20, Win32, Direct2D; no audio or external runtime to install.

**v0.1.0 is stable.** My Windows 11 testing passed. [Download the release](https://github.com/jerephil/toaster-squadron/releases/tag/v0.1.0) or read the [changelog](CHANGELOG.md).

![My running Toaster Squadron build with Claude Design sprites over the pixel-art synthwave backdrop](docs/screenshot.png)

Screenshot from my running CI build on a GitHub-hosted Windows desktop, using the Claude Design sprite sheets and pixel-art sun, ridges, and grid.

Toaster Squadron is an independent, original project and is not affiliated with or endorsed by Berkeley Systems or the makers of After Dark.

## Install and test

Download [ToasterSquadron.scr](https://github.com/jerephil/toaster-squadron/releases/download/v0.1.0/ToasterSquadron.scr) directly, or download and extract the [Windows x64 package](https://github.com/jerephil/toaster-squadron/releases/download/v0.1.0/ToasterSquadron-windows-x64.zip). The extracted folder contains `ToasterSquadron.scr` at its top level, beside `assets/`, `docs/`, `README.md`, `LICENSE`, and `SHA256SUMS.txt`. From that folder, the exact Windows path is `.\ToasterSquadron.scr`; there is no `dist/` folder or second zip to open. Right-click it → **Install**, then use Windows Screen Saver Settings to preview and configure it. Alternatively copy it to `C:\Windows\System32` (administrator permission required). An unsigned download may trigger SmartScreen; inspect and build the source yourself if preferred.

- `ToasterSquadron.scr /s`: full screen on every monitor; exits on keyboard, click, or deliberate mouse movement.
- `ToasterSquadron.scr /p <HWND>`: live child preview for Windows Screen Saver Settings.
- `ToasterSquadron.scr /c` or no arguments: settings.

My stable release uses the same screensaver executable that passed my Windows 11 testing. [SHA256SUMS.txt](https://github.com/jerephil/toaster-squadron/releases/download/v0.1.0/SHA256SUMS.txt) contains hashes for the standalone saver and package zip. Development builds are available from [GitHub Actions](https://github.com/jerephil/toaster-squadron/actions/workflows/build.yml) (artifact downloads require GitHub sign-in).

## Settings

Flyers (5–40, default 15), slow/medium/fast speed, toast ratio (0–100%, default 30%), light/golden/dark toast, full synthwave/sun only/dark starfield, grid speed (off/slow/fast), and classic synthwave/sunset palette. Settings are per-user under `HKCU\Software\ToasterSquadron`.

## Build

Windows 11, Windows SDK, CMake 3.24+, and the Visual Studio 2022 C++ desktop toolchain:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The output is `build\Release\ToasterSquadron.scr`. Windows SDK libraries are the only application dependencies; Release uses the static MSVC runtime. CI uses public GitHub-hosted Windows runners only. If `windows-latest` has moved to a newer Visual Studio, the workflow installs the VS 2022 build tools from Microsoft before configuring.

## Validation

The Windows CI job builds with MSVC and exercises the settings dialog, live child preview and resizing, and full-screen monitor coverage with key/click/mouse exit and a mouse-jitter threshold. My [PC test checklist](docs/TESTING.md) covers appearance, mixed-DPI monitors, mouse jitter, and longer runs. My Windows 11 testing is complete for v0.1.0; I retain the checklist for regression testing future versions. Release executables are unsigned.

## Art and license

My new sprite sheets came from Claude Design, following the [art brief](docs/ART-BRIEF.md). The sun, ridges, and grid now use matching chunky pixels. The four PNG sheets can be replaced in `assets/` without code changes. Do not run `tools/generate_assets.py`: it would overwrite the new art with the old placeholders. MIT licensed, copyright Jeremy W. Phillips; see [LICENSE](LICENSE).
