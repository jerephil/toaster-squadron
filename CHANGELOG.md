# Changelog

## v0.1.0 — 2026-11-17

My first stable release, approved after my successful Windows 11 testing.

- Original Claude Design toaster and toast sprites with four-frame wing flaps and occasional toaster barrel rolls.
- Pixel-art sun, ridges, and scrolling synthwave grid; configurable flyer count, speed, toast ratio/doneness, backdrop, grid speed, and palette.
- Multi-monitor full screen, live Screen Saver Settings preview, per-user settings, and input exit handling.
- Standalone Windows x64 `.scr`, flat package zip, and SHA-256 checksums.

My stable executable is byte-for-byte identical to the `test-0.1.0-rc.1` executable I tested, built from `da8b84fa6bfde661ea3690c781a3213b17ecbc86` in [Windows CI](https://github.com/jerephil/toaster-squadron/actions/runs/44176772349). I updated the package documentation for stable distribution. Both of my Windows tests (`screensaver_contract` and `pixel_backdrop`) passed without MSVC compiler or linker warnings; sprite validation reported `ALL OK`.
