# BLACKBOX PS4 native app

First native PS4 application shell built against OpenOrbis and SDL2.

Modules:
- blackbox/modules/controller: SDL controller and keyboard event adapter.
- blackbox/modules/ui: SDL renderer and initial three-panel layout.
- blackbox/modules/storage: persistent per-user settings via SDL_GetPrefPath.

The shell is deliberately metadata-only. No package download or install function is implemented. Network transport will be integrated only after catalogue policy and endpoint validation are settled.

Build requires the OpenOrbis PS4 Toolchain (v0.5.4+) and its SDL2 port. Run make from this directory with OO_PS4_TOOLCHAIN set.
