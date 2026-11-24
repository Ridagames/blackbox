# BLACKBOX PS4 native application

The PS4 application shell is built with the OpenOrbis v0.5.4 toolchain and its SDL2 port.

## Modules

- `blackbox/modules/controller`: native PS4 DualShock 4 input through `scePadReadState`, reconnect handling, D-pad and both stick axes, CROSS confirm, CIRCLE/OPTIONS back, and repeat timing.
- `blackbox/modules/ui`: navigable home screen, PS4 release list, entry details, favorites and settings panels.
- `blackbox/modules/font`: compact built-in bitmap font for on-screen labels.
- `blackbox/modules/storage`: persistent user settings through SDL_GetPrefPath.
- `blackbox/modules/catalog`: offline metadata reader for the packaged PS4 catalogue.

## Catalogue

The PS4 catalogue metadata snapshot is bundled at `data/catalog.json` and packaged inside the FPKG. It contains 820 PS4 release records with title IDs, region, version, publisher, release date and size. Package URLs, filenames, providers and checksums are deliberately excluded. The catalogue is navigable with the D-pad or left stick; CROSS opens an entry and CIRCLE returns.

The controller module is adapted from the tested PS4 input implementation in [nueex/ps-mtp-explorer](https://github.com/nueex/ps-mtp-explorer) (GPL-3.0-or-later); its copyright and license are retained in the module folder. BLACKBOX uses the same native `scePad`/`sceUserService` path rather than relying on SDL game-controller events.

No game-package download or installation is implemented. The package workflow validates the resulting PS4 PKG structure before it can be attached to a versioned GitHub Release.

## Build

Run `make` from this directory with `OO_PS4_TOOLCHAIN` pointing to OpenOrbis v0.5.4 and its `bin/linux` tools on PATH. Runtime module files are staged from OpenOrbis `src/modules`.
