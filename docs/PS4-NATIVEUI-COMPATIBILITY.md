# BlackBox PS4 NativeUI compatibility layer

This document defines the first PS4-side implementation boundary. It is an implementation contract, not a claim that the PS4 host is already built.

## Design

Keep the recovered BlackBox JavaScript application as the presentation/application layer wherever its APIs are portable. Run it in a QuickJS-compatible runtime on PS4. Provide a PS4-native host object named `NativeUI` matching the calls used by the original app. Do not attempt to load PS5 PRX/ELF files on PS4.

Split the host into replaceable providers:

- `input`: DualShock 4 / DualSense buttons, axes, focus and repeat.
- `view`: retained UI tree, layout, text, image, SVG and clipping.
- `animation`: timed property transitions and cancellation.
- `audio`: named UI sound playback, volume and channel management.
- `assets`: packaged images, fonts, sound effects and wallpapers.
- `storage`: app data, cache, free-space and safe file operations.
- `network`: HTTP(S), download queue, progress, cancellation and retry.
- `platform`: firmware, display and device capabilities with explicit unsupported values.

## Compatibility rules

1. Preserve original JS method names and argument shapes only after they have been confirmed from the recovered bundle.
2. Return promises for asynchronous operations where the original app expects promises.
3. All platform-specific failures must be explicit error objects; never report a fake successful install or download.
4. Unknown PS5-only calls must return a stable `UNSUPPORTED_ON_PS4` error until a real PS4 implementation exists.
5. The UI must remain navigable with the controller even when network access is unavailable.
6. Catalogue artwork should be cached locally and loaded from the extracted asset manifest; no dependency on Sony CDN at runtime.
7. Do not expose shell execution or arbitrary filesystem access to the JS layer.

## PS4 implementation order

### Phase A — boot and input
- Start the JS runtime and load the extracted app bundle.
- Implement controller polling, focus movement, confirm, back, shoulder navigation and repeat.
- Provide deterministic frame/update scheduling.

### Phase B — rendering
- Implement view creation/update/removal.
- Implement text measurement and font loading.
- Implement image decode, SVG subset, clipping and z-order.
- Match the original focus, modal and transition behavior.

### Phase C — original media
- Extract and validate original audio and image payloads.
- Add a name-to-file asset registry for sounds and visual assets.
- Verify every sound identifier resolves or explicitly reports missing.

### Phase D — portable app services
- Catalogue data, details, search, favourites and settings.
- Download queue with progress, cancellation and resume where supported.
- File browser restricted to app-managed storage.

### Phase E — console-specific features
- Replace PS5 package installer, ShadowMount and PS5 firmware/service APIs with PS4 equivalents only where a documented PS4 API exists.
- Otherwise disable those controls with a clear UI state.

## Acceptance criteria

A PS4 port is not considered ready until:
- app starts repeatedly without a crash;
- every visible screen can be reached and exited using the controller;
- focus is always visible and never becomes trapped;
- all included original media decodes on-device;
- no PS5 PRX/ELF dependency is loaded;
- unsupported PS5 actions cannot falsely report success;
- catalogue, search and settings work offline;
- download/file operations report truthful progress and errors.

## Current blockers

- The exact end boundary of the embedded JavaScript bundle is not yet independently verified; the first hard-coded extraction check failed.
- Original audio/image payload files have not yet been individually validated from the executable.
- The exact NativeUI call signatures must be extracted from the validated bundle before implementing the ABI.
- No PS4 SDK build or hardware test has been completed.
