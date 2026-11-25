# BLACKBOX PS4 native application

The PS4 application shell is built with the OpenOrbis v0.5.4 toolchain and its SDL2 port.

## Modules

- `blackbox/modules/controller`: native PS4 DualShock 4 input through `scePadReadState`, reconnect handling, D-pad and both stick axes, CROSS confirm, CIRCLE/OPTIONS back, and repeat timing.
- `blackbox/modules/ui`: image-backed SDL2 interface with the BlackBox wallpaper/logo, cached PS4 cover artwork, catalogue preview, details, favorites, settings and controller navigation.
- `blackbox/modules/downloader`: native PS4 HTTPS downloader using SceNet/SceSsl/SceHttp. It accepts only `.pkg` files from explicitly curated homebrew entries and keeps certificate verification enabled.
- `blackbox/modules/font`: compact built-in bitmap font for on-screen labels.
- `blackbox/modules/storage`: persistent user settings through SDL_GetPrefPath.
- `blackbox/modules/catalog`: offline metadata reader for the packaged PS4 catalogue.

## Catalogue

The catalogue is bundled at `data/catalog.json` and packaged inside the FPKG. It contains 820 PS4 release records plus a curated Apollo Save Tool homebrew entry. Commercial entries are blocked by default; common commercial PS4 title-ID prefixes (CUSA/PPSA/PCAS/PLAS and related IDs) are always treated as commercial. A download is allowed only when an entry is explicitly marked `commercial: false` and `contentType: homebrew`. The DOWNLOAD action remains visible on every details page and commercial entries display the requested cancellation message.

The Apollo Save Tool entry points to its official v2.4.0 GitHub release. Downloaded packages are saved under the app's SDL preference directory in `downloads/`; BlackBox does not install them automatically.

The controller module is adapted from the tested PS4 input implementation in [nueex/ps-mtp-explorer](https://github.com/nueex/ps-mtp-explorer) (GPL-3.0-or-later); its copyright and license are retained in the module folder. BLACKBOX uses the same native `scePad`/`sceUserService` path rather than relying on SDL game-controller events.

During the build, `tools/prepare_artwork.py` downloads only catalogue cover-image URLs and converts them to small BMP textures for SDL2. It never downloads package URLs. The build workflow packages these textures with the app and validates the resulting FPKG structure.

## Build

Run `make` from this directory with `OO_PS4_TOOLCHAIN` pointing to OpenOrbis v0.5.4 and its `bin/linux` tools on PATH. Runtime module files are staged from OpenOrbis `src/modules`.
