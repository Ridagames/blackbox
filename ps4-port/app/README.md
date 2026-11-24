# BLACKBOX PS4 native application

The PS4 application shell is built with the OpenOrbis v0.5.4 toolchain and its SDL2 port.

## Modules

- `blackbox/modules/controller`: native PS4 DualShock 4 input through `scePadReadState`, reconnect handling, D-pad and both stick axes, CROSS confirm, CIRCLE/OPTIONS back, and repeat timing. Keyboard input remains available for development.
- `blackbox/modules/ui`: SDL2 rendering for the initial catalogue, favorites and settings panels.
- `blackbox/modules/font`: compact bitmap font for readable on-screen labels without external font assets.
- `blackbox/modules/storage`: persistent user settings through SDL_GetPrefPath.

The controller module is adapted from the tested PS4 input implementation in `nueex/ps-mtp-explorer` (GPL-3.0-or-later); its copyright and license are retained in the module folder. BLACKBOX uses the same native `scePad`/`sceUserService` path rather than relying on SDL game-controller events. The app currently provides the native shell and navigation foundation. Catalogue parsing and filtering are maintained in the portable core; connecting that data to the PS4 screen and adding HTTPS catalogue refresh remain subsequent integration tasks.

No game-package download or installation is implemented. The package workflow verifies the resulting PS4 PKG structure before it can be attached to a versioned GitHub Release.

## Build

Run `make` from this directory with `OO_PS4_TOOLCHAIN` pointing to OpenOrbis v0.5.4 and its `bin/linux` tools on PATH.

Runtime module files are staged from OpenOrbis `src/modules` (the location used by the pinned v0.5.4 release archive).

The Makefile includes the OpenOrbis SDL2 header directory explicitly.

CI installs the legacy OpenSSL 1.1 compatibility runtime needed by the bundled LibOrbisPkg publisher.
