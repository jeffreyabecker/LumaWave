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
| `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` | `0` | Removes high-risk combinatorial template types from the exported surface | `0`, `1` | When `1`, disables `PixelBus`, `CompositeBus`, and `CompositeShader` exports and their public aliases. Runtime/interface-based alternatives such as `IPixelBus`, `AggregateBus`, `AggregateShader`, and `LightBus` remain available. |
| `LW_COLOR_MINIMUM_COMPONENT_COUNT` | `4` | Minimum internal channel count for color storage (`DefaultColorType`/internal color padding) | `3`, `4`, `5` | Global memory/compatibility trade-off; `4` defaults to RGBW-style internal storage. |
| `LW_COLOR_MINIMUM_COMPONENT_SIZE` | `8` | Minimum internal component bit depth for color storage | `8`, `16` | May widen internal storage component type to `uint16_t` when set to `16`. |

### Validation Constraints

- `LW_COLOR_MINIMUM_COMPONENT_COUNT` must be in the range `[3, 5]`.
- `LW_COLOR_MINIMUM_COMPONENT_SIZE` must be `8` or `16`.

### Example Build Defines

- Memory-lean RGB internal storage:
  - `-D LW_COLOR_MINIMUM_COMPONENT_COUNT=3`
  - `-D LW_COLOR_MINIMUM_COMPONENT_SIZE=8`

- Force 16-bit internal component storage:
  - `-D LW_COLOR_MINIMUM_COMPONENT_SIZE=16`

- Disable exported combinatorial template types:
  - `-D LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES=1`


