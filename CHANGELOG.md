# Changelog

All notable changes to dlssg-to-fsr3 are documented in this file.

## [0.130] - Latest Release
- Added support for newer Streamline plugin interposer paths.
- Added logging for Vulkan present metering availability.

## [0.123]
- Added a warning prompt because Monster Hunter Wilds crashes without REFramework installed. Once again, multiplayer games aren't supported.

## [0.122]
- Removed a workaround for Indiana Jones and the Great Circle as it is no longer necessary.

## [0.121]
- Added additional error logging.
- Added future proofing for RTX Remix-based games.
- Fixed issues reading configuration settings when supplied through environment variables.

## [0.120]
- Added support for intercepting/hooking over-the-air Streamline plugin updates with the Universal edition.
- Added support for games that pass in bidirectional distortion field resources.
- Added workarounds to support Indiana Jones and the Great Circle.
- Added workarounds to better support RTX Remix-based games.
- Added workarounds for games providing incorrect camera far, near, and field of view values.
- Migrated to the latest AMD FidelityFX SDK (v1.1 -> v1.1.3).
- Fixed occasional blurry rectangle (interpolation rect) issues when switching upscalers or changing output resolutions.

## [0.110]
- Added native Vulkan support for FSR 3.1.

## [0.100]
- Tentative support for FSR 3.1 frame generation.
- Added extremely experimental support for Vulkan. Expect artifacts and disocclusion issues.
- Implemented even more aggressive hooking in the universal variants due to recent DLSS SDK changes.
- Revised a number of debug log prints.

## [0.90]
- Added a Universal zip archive for maximum game support. Separate READ ME.txts are included within each folder. Registry key tweaks are not required.
- Universal DLLs now automatically disable the EGS overlay due to hooking conflicts.
- Universal DLLs now bypass GPU architecture checks for stubborn games (Dying Light 2, Returnal).
- HDR luminance values are now queried from the active monitor, falling back to defaults when necessary.
- Fixed GPU driver crashes in Dying Light 2 with universal DLLs.
- Hardware accelerated GPU scheduling status is now logged.

## [0.81]
- Fixed GPU hangs in certain games with major scene transitions (e.g. The Witcher 3).
- Miscellaneous smaller stability fixes and error checking.
- Added the ability to rename nvngx.dll to version.dll, winhttp.dll, or dbghelp.dll to avoid the registry key signature override requirement.

## [0.80]
- Hopefully fixed all texture format conversion crashes (e.g. Hogwarts Legacy).
- Improved error logging, again.

## [0.70]
- Error checking code rewritten.
- Logging code rewritten.
- Added better support for texture dimensions/formats changing at runtime.
- Added a developer config option to show only interpolated frames.
- Improved nvngx wrapper dll compatibility.

## [0.60]
- The nag prompt at startup has been removed.
- Added a log file ("dlssg_to_fsr3.log") in the game directory.
- Added support for developer options and debug overlay ("dlssg_to_fsr3.ini").
- More stability fixes.

## [0.50]
- Experimental format conversion support. This mainly includes HDR-enabled games along with mismatched UI render target formats.

## [0.41]
- Fixed accidental inclusion of debug overlay.

## [0.40]
- Replaced dbghelp.dll with nvngx.dll for better game compatibility. Please delete the old dbghelp.dll version from earlier releases.
- DisableNvidiaSignatureChecks.reg is now required for usage in games.
- Various stability fixes.

## [0.30]
- Fixed numerous game crashes (e.g. Starfield).

## [0.21]
- Fixed a typo in DLL path determination.
- Added explicit binary license.

## [0.20]
- First working build.

## [0.10]
- Initial test release.

## [1.2.0-wealdly1] - 2026-03-15
- Migrated from FidelityFX SDK v1.1.3 to v1.1.4.
- Adapted to SDK API changes: `FFX_FRAMEINTERPOLATION_ENABLE_PREDILATED_MOTION_VECTORS` flag removed (SDK now handles predilated motion vectors internally).
- Exposed internal `BackendContext_DX12` struct for custom resource allocation compatibility with SDK 1.1.4 private headers.
- Added automatic game engine detection based on executable name and loaded DLLs (Unreal Engine, Unity, REDengine, Frostbite).
- Introduced a per-game quirk table with wildcard matching for applying engine-specific fixes:
  - **DepthInverted**: Force inverted depth for engines that use it (e.g. Unreal Engine).
  - **ForceTAAJitter**: Assume TAA jitter is present when the game doesn't report it.
  - **DisablePredilatedMVs**: Disable predilated motion vectors for problematic titles (e.g. Baldur's Gate 3, Unity games).
  - **MotionVectorScaleOverride**: Override motion vector scale per-game.
  - **HUDlessBufferFix**: Apply HUDless buffer workaround (e.g. Cyberpunk 2077, The Witcher 3).
- All active quirks are logged on detection for easier debugging.
- Added GPU-based automatic letterbox/pillarbox detection for games running in non-native aspect ratios (e.g. 21:9 content on a 16:9 display).
- Copies narrow pixel strips from the backbuffer center for analysis—no full-frame readback, minimal GPU overhead.
- Requires consecutive stable detections across multiple frames before applying (stability check).
- Auto-detected bars constrain the interpolation rect so FSR 3 skips black bar regions.
- Can be enabled via INI: `[GenerationRect] EnableAutoLetterboxDetection=1`.
- Added manual interpolation region support via INI (`[GenerationRect]` section: `Left`, `Top`, `Width`, `Height`).
- Allows users to confine frame interpolation to a specific sub-region of the screen, reducing GPU work on static/black areas.
- Auto letterbox detection results are overridden when manual rect values are set.
- Added new `[Tuning]` INI section with the following options (all also settable via `DLSSGTOFSR3_` environment variables):
  - `OpticalFlowBlockSize` — Optical flow block size (default: 8).
  - `HDRLuminanceMin` / `HDRLuminanceMax` — HDR luminance range fallback.
  - `DefaultFOV` — Default vertical FOV in degrees when the game provides 0 (default: 90.0).
  - `EnableAsyncCompute` — Enable async compute for frame interpolation (default: 1).
  - `MinBaseFPS` — Minimum base FPS threshold; disables frame interpolation when base FPS drops below this value (default: 0 = disabled).
- Frame interpolation is now automatically disabled when the base FPS drops below the configurable `MinBaseFPS` threshold, and re-enabled once FPS recovers. Prevents interpolation from worsening already-low framerates.
- Frame interpolation dispatch now increments and passes a proper `frameID` to the FSR 3 context instead of always sending 0.
- Added `FFX_FRAMEINTERPOLATION_ENABLE_ASYNC_SUPPORT` flag to the frame interpolation context when `EnableAsyncCompute` is enabled.
- Added overloaded `Util::GetSetting()` for `int`, `float`, and section-scoped `bool`/`int` types.
- Settings can be read from both the INI file and environment variables.
- Added release-mode compiler optimizations for MSVC across all wrapper targets: `/O2`, `/Oi`, `/Ot`, `/fp:fast`, `/arch:AVX2`.
- INI config file (`dlssg_to_fsr3.ini`) is now included in the install/distribution output.
- Debug overlay is now disabled by default in the shipped INI (`EnableDebugOverlay=0`).
