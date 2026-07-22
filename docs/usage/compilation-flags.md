# Compilation Flags

Factory, dynamic builder, and INI parser flags have been removed from the public surface.

## Language Standard Baseline

- Native CMake builds and native tests now target C++23.
- Embedded Arduino-target builds remain on a C++17-compatible path dictated by the board/toolchain environment.
- The compilation flags documented below remain valid for both environments unless a target-specific toolchain restricts them.


## Color Compilation Flags

Color-related compile-time controls remain supported.

| Flag | Default | Controls | Allowed Values | Notes |
|------|---------|----------|----------------|-------|
| `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` | `0` | Removes high-risk combinatorial template types from the exported surface | `0`, `1` | When `1`, disables `PixelBus` exports and their public aliases. Runtime/interface-based alternatives such as `IPixelBus` and BusBuilder remain available. |
| `LW_PIXEL_COMPONENT_SIZE` | `8` | Minimum internal component bit depth for color storage | `8`, `16` | When set to `16`, `DefaultColorType` becomes `Rgbw16Color`; otherwise `Rgbw8Color`. Channel count is always 4 (RGBW). |

### Validation Constraints

- `LW_PIXEL_COMPONENT_SIZE` must be `8` or `16`.

### Example Build Defines

- Force 16-bit internal component storage:
  - `-D LW_PIXEL_COMPONENT_SIZE=16`

- Disable exported combinatorial template types:
  - `-D LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES=1`


