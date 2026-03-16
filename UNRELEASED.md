# Unreleased Changes

Changes in development, not yet included in a tagged release.

## SDK Upgrade
- Migrated from FidelityFX SDK v1.1.3 to v1.1.4.
- Adapted to SDK API changes: `FFX_FRAMEINTERPOLATION_ENABLE_PREDILATED_MOTION_VECTORS` flag removed (SDK now handles predilated motion vectors internally).
- Exposed internal `BackendContext_DX12` struct for custom resource allocation compatibility with SDK 1.1.4 private headers.

## Game Engine Detection & Quirks System
- Added automatic game engine detection based on executable name and loaded DLLs (Unreal Engine, Unity, REDengine, Frostbite).
- Introduced a per-game quirk table with wildcard matching for applying engine-specific fixes:
  - **DepthInverted**: Force inverted depth for engines that use it (e.g. Unreal Engine).
  - **ForceTAAJitter**: Assume TAA jitter is present when the game doesn't report it.
  - **DisablePredilatedMVs**: Disable predilated motion vectors for problematic titles (e.g. Baldur's Gate 3, Unity games).
  - **MotionVectorScaleOverride**: Override motion vector scale per-game.
  - **HUDlessBufferFix**: Apply HUDless buffer workaround (e.g. Cyberpunk 2077, The Witcher 3).
- All active quirks are logged on detection for easier debugging.

## Automatic Letterbox Detection (DX12)
- Added GPU-based automatic letterbox/pillarbox detection for games running in non-native aspect ratios (e.g. 21:9 content on a 16:9 display).
- Copies narrow pixel strips from the backbuffer center for analysis—no full-frame readback, minimal GPU overhead.
- Requires consecutive stable detections across multiple frames before applying (stability check).
- Auto-detected bars constrain the interpolation rect so FSR 3 skips black bar regions.
- Can be enabled via INI: `[GenerationRect] EnableAutoLetterboxDetection=1`.

## Custom Interpolation Region (Generation Rect)
- Added manual interpolation region support via INI (`[GenerationRect]` section: `Left`, `Top`, `Width`, `Height`).
- Allows users to confine frame interpolation to a specific sub-region of the screen, reducing GPU work on static/black areas.
- Auto letterbox detection results are overridden when manual rect values are set.

## Configurable Tuning Parameters
- Added new `[Tuning]` INI section with the following options (all also settable via `DLSSGTOFSR3_` environment variables):
  - `OpticalFlowBlockSize` — Optical flow block size (default: 8).
  - `HDRLuminanceMin` / `HDRLuminanceMax` — HDR luminance range fallback.
  - `DefaultFOV` — Default vertical FOV in degrees when the game provides 0 (default: 90.0).
  - `EnableAsyncCompute` — Enable async compute for frame interpolation (default: 1).
  - `MinBaseFPS` — Minimum base FPS threshold; disables frame interpolation when base FPS drops below this value (default: 0 = disabled).

## Performance Safety
- Frame interpolation is now automatically disabled when the base FPS drops below the configurable `MinBaseFPS` threshold, and re-enabled once FPS recovers. Prevents interpolation from worsening already-low framerates.

## Frame ID Tracking
- Frame interpolation dispatch now increments and passes a proper `frameID` to the FSR 3 context instead of always sending 0.

## Async Compute Support
- Added `FFX_FRAMEINTERPOLATION_ENABLE_ASYNC_SUPPORT` flag to the frame interpolation context when `EnableAsyncCompute` is enabled.

## Extended Util API
- Added overloaded `Util::GetSetting()` for `int`, `float`, and section-scoped `bool`/`int` types.
- Settings can be read from both the INI file and environment variables.

## Build System
- Added release-mode compiler optimizations for MSVC across all wrapper targets: `/O2`, `/Oi`, `/Ot`, `/fp:fast`, `/arch:AVX2`.
- INI config file (`dlssg_to_fsr3.ini`) is now included in the install/distribution output.

## Config Defaults
- Debug overlay is now disabled by default in the shipped INI (`EnableDebugOverlay=0`).
