# Changelog

## v0.1.0 — 2026-11-17

First stable release, approved after Jeremy's successful Windows 11 testing.

- Original Claude Design toaster and toast sprites with four-frame wing flaps and occasional toaster barrel rolls.
- Pixel-art sun, ridges, and scrolling synthwave grid; configurable flyer count, speed, toast ratio/doneness, backdrop, grid speed, and palette.
- Multi-monitor full screen, live Screen Saver Settings preview, per-user settings, and input exit handling.
- Standalone Windows x64 `.scr`, flat package zip, and SHA-256 checksums.

The stable executable is byte-for-byte identical to the accepted `test-0.1.0-rc.1` executable, built from `da8b84fa6bfde661ea3690c781a3213b17ecbc86` in [Windows CI](https://github.com/jerephil/toaster-squadron/actions/runs/44176772349). Package documentation has been updated for stable distribution. Both Windows tests (`screensaver_contract` and `pixel_backdrop`) passed without MSVC compiler or linker warnings; sprite validation reported `ALL OK`.
