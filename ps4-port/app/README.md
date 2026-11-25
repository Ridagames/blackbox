# BLACKBOX PS4 native application

Built with OpenOrbis v0.5.4, SDL2 and Dear ImGui 1.89.9.

## Interface
The native interface is implemented with Dear ImGui widgets and the SDL2 platform/renderer backends. It provides a home screen, six-column cover catalogue, title details, favorites and settings. Cover textures are loaded lazily and cached. The PS4 controller module handles DualShock 4 input through scePad.

## Modules
- `blackbox/modules/controller`: native PS4 DualShock 4 input and reconnect handling.
- `blackbox/modules/ui`: Dear ImGui interface, cover cache, catalogue pages and actions.
- `blackbox/modules/downloader`: HTTPS homebrew PKG downloader with certificate verification.
- `blackbox/modules/storage`: persistent preferences and favorites.
- `blackbox/modules/catalog`: offline PS4 catalogue reader.
- `third_party/imgui`: Dear ImGui 1.89.9 and official SDL2 backends. See upstream license headers and license file.

## Catalogue and download safety
The catalogue is bundled in `data/catalog.json`. Commercial entries remain blocked: a download is accepted only when an entry is explicitly marked non-commercial and `contentType: homebrew`. Downloaded packages are saved in the app preference directory; BlackBox does not install them automatically.

## Build
Run `make` from this directory with `OO_PS4_TOOLCHAIN` pointing to OpenOrbis v0.5.4.
