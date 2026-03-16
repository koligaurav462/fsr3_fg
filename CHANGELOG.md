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
