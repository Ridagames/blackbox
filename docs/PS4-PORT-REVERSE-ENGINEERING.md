# BlackBox PS4 port — reverse-engineering findings

Branch: `ps4-port`
Source: official BlackBox 2.0.0 PS5 filesystem and `eboot.bin`
Status: analysis in progress; no PS4 binary is claimed yet.

## Confirmed original application code

- `eboot.bin` is 118,393,735 bytes.
- A readable bundled JavaScript application begins at file offset `0x5d7a680` with `(() => {`.
- The embedded bundle contains React 18.3.1 / JSX runtime, QuickJS bridge code, and original source-path comments such as `../app/index.jsx`, `../app/browse/BrowsePage.jsx`, and `../app/game/index.jsx`.
- The inspected region contains 211 distinct `../app/` module paths. This is strong evidence that the original application logic is embedded in the executable, not just a set of UI labels.
- A previous extraction attempt used an incorrect hard-coded end offset and failed its terminator check. Therefore the full standalone `app.jsx.bundle.js` is NOT yet certified. The new recovery workflow scans candidate boundaries and runs `node --check` before accepting a file.

## Original app modules identified

The bundle includes these functional areas:

- Browse/catalogue: BrowsePage, Catalog, GameRail, GenreRail, Spotlight, TopCharts, FavoritesScreen, filters, sorting and slide controls.
- Game detail: GameHero, About, MediaGallery, Trailer, PlayerReviews, DownloadButton, SourcePicker, version/choice sheets and install/download dialogs.
- Downloads: queue, throughput, progress rows, speed chart, alerts, URL and browser-capture dialogs.
- Files: file browser, storage bar, scan and file-state hooks.
- Search: on-screen keyboard, suggestions, history and matching.
- Settings/system: language, firmware, diagnostics, update, controller lightbar, presence and wallpaper.
- PS5-only integration: package conversion/helper, ShadowMount, installer/relauncher, device login, system services and console-specific storage/network operations.

## Original media references found in the bundle

The application bundle declares original named sound assets, including:

`back_01`, `error_01`, `favorite_off_01`, `favorite_on_01`, `focus_01`, `focus_02`, `launch_01`, `modal_close_01`, `modal_open_01`, `notify_01`, `download_started_01`, `open_01`, `resume_01`, `saved_01`, `select_01`, `slider_01`, `slider_02`, `slider_03`, `tab_01`, `toggle_01`, `toggle_02`, and `welcome_01`.

Other bundled asset references include `constellation.png`, `splash-wallpaper.png`, `ascii-dial.png`, CDN-card art, genre icons, language flags, Inter and JetBrains Mono fonts. These are currently confirmed as asset references/identifiers in the JS bundle; the actual WAV/PNG/TTF payload bytes still need to be located and extracted separately.

The separate catalogue-artwork extraction already recovered 6,537 original PlayStation CDN thumbnail images with zero download failures (artifact `BLACKBOX-ORIGINAL-CATALOG-ARTWORK`).

## PS5 native runtime dependencies

The JS bundle is not a browser page. It expects a custom native bridge, including `globalThis.NativeUI`, plus a PS5-REACT host runtime. The executable exposes native functionality for:

- controller/pad input, focus navigation, haptics and lightbar;
- native view/text/image/SVG nodes and animated properties;
- filesystem, app/data/cache roots, storage usage and file operations;
- network, downloads, streaming and archive handling;
- audio output and named sound playback;
- video decoding, frame presentation and image decoding;
- display mode/refresh-rate and system-service information;
- PS5 package install/ShadowMount and other console-only operations.

The native PS5 ELF files and Sony system libraries cannot be copied to PS4 as-is. The PS4 port must provide a PS4-native host/bridge and map the reusable application logic onto it.

## Porting strategy

1. Recover and syntax-validate the complete original JS bundle with exact byte boundaries.
2. Extract original non-catalogue assets (WAV, PNG, fonts, wallpapers) from the filesystem or executable; verify every output by file signature and media decoder.
3. Build a PS4 host for the app: QuickJS runtime plus a PS4 implementation of the NativeUI bridge.
4. Implement controller-first focus/navigation, rendering, images/SVG, text/fonts, animation and sound before wiring the full catalogue.
5. Map portable features (catalogue, game details, search, favourites, settings, download queue and file browser) onto PS4 services.
6. Isolate or replace PS5-exclusive operations (ShadowMount, PS5 package installation, PS5 firmware/service APIs) rather than pretending they work on PS4.
7. Produce a PS4 test build and verify controller navigation, media, downloads and stability on hardware.

## Current truth

- Original catalogue art: extracted and verified.
- Original filesystem: extracted in a prior workflow.
- Original JS application code: visibly present in `eboot.bin`; full standalone bundle extraction is being corrected/validated.
- Original UI sound payloads and wallpaper payloads: names/references identified, actual files not yet verified.
- PS4 native runtime/bridge: not implemented yet.
- PS4 build: not produced yet.
