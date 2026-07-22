# Color → Pixel Rename Backlog

Purpose: Rename all `color`-prefixed entities (`Color`, `ColorComponent`, `colorR`, `colorFromRGB`, etc.) to use the `pixel` prefix (`Pixel`, `PixelComponent`, `pixelR`, `pixelFromRGB`, etc.) throughout the codebase.

## Status Legend

- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Source Documents

- `src/core/Color.h` — primary type definitions (`Color`, `ColorComponent`, accessors, constructors, comparison)
- `src/core/Compat.h` — `LW_COLOR_COMPONENT_SIZE` compile-time config
- `src/LumaWave.h` — public API `using` declarations (convenience layer)
- `src/palettes/ColorMath.h` — blend math operating on `Color` values
- `src/palettes/BlendOperations.h` — quantized/CCT blend functions
- `src/palettes/Blends.h` — palette sampling with `PaletteStop::color`
- `src/palettes/Detail.h` — brightness scaling helper
- `src/palettes/Generators.h` — palette generators referencing `Color`/`ColorComponent`
- `src/palettes/Sampling.h` — `PaletteSampleIterator::value_type = lw::Color`
- `src/palettes/SamplingTransition.h` — blend transition proxy
- `src/palettes/Types.h` — `PaletteStop`, `PaletteSampleOptions`, `IPalette`, `Palette` all reference `Color`/`ColorComponent`/`ColorType`
- `src/palettes/Traits.h` — `IsPaletteLike` references `ColorType`
- `src/palettes/WrapModes.h` — boundary sampling
- `src/protocols/IProtocol.h` — `ColorType` alias, plus bug: `#include "colors/Color.h"`
- `src/protocols/Protocol.h` — `ColorType` alias
- `src/protocols/IShader.h` — shader apply signature references `lw::Color`
- `src/protocols/DotStarProtocol.h` — `InterfaceColorType`, protocol encoding
- `src/protocols/BrightnessShader.h` — `BrightnessType = lw::ColorComponent`
- `src/protocols/GammaShader.h` — gamma LUT applies to `Color`
- `src/protocols/Lpd6803Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Lpd8806Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/P9813Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/PixieProtocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Sm16716Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Sm168xProtocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Tlc59711Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Tm1814Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Tm1914Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Ws2801Protocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/Ws2812xProtocol.h` — `static_assert` on `ColorComponent`
- `src/protocols/ShaderProtocol.h` — includes `Color.h`
- `src/buses/Bus.h` — `span<Color>` pixel storage
- `src/buses/PixelBus.h` — `span<Color>` scratch/pixel arrays
- `src/buses/StackPixelBus.h` — `span<Color>` arrays
- `src/buses/ProtocolTransportPipeline.h` — includes `Color.h`
- `src/core/IPixelBus.h` — includes `Color.h`
- `src/core/OutputPipeline.h` — `write(span<const Color>)` signature
- `src/core/Fill.h` — `fillPixels` includes `Color.h`
- `src/core/KelvinToRgbStrategies.h` — includes `Color.h`, uses `ColorComponent`
- `test/busses/test_bus/test_main.cpp` — mock references `lw::Color`
- `test/contracts/test_palette_first_pass_compile/test_main.cpp` — `static_assert` on `lw::Color`
- `test/lights/test_print_light_driver/test_main.cpp` — `lw::Color` variables
- `test/protocols/test_protocol_spec_sections_1_1_to_1_4_and_1_14/test_main.cpp` — heavy `lw::Color` usage
- `test/protocols/test_protocol_spec_sections_1_5_to_1_13/test_main.cpp` — heavy `lw::Color` usage
- `include/.gitkeep` — references `core/Color.h` and `Rgb8Color`
- `examples/` — all .ino files reference `lw::Color` or `Color`
- `docs/comparison-lumawave-vs-fastled.md` — references `Color` type
- `docs/usage/bus-builder.md` — references `lw::Color`
- `docs/usage/compilation-flags.md` — references `LW_COLOR_COMPONENT_SIZE`
- `docs/usage/palette-parsing.md` — references `TColor` and `Color`
- `docs/internal/platform-transport-inventory.md` — references `Color`
- `docs/internal/wled-lumawave-compatibility.md` — references `Color`
- `docs/internal/wled-mm-pixel-data-analysis.md` — references `Color`
- `docs/backlog/plan/bus-builder-lifetime-simplification.md` — references `Color`

## Problem Statement

The codebase currently uses the term `Color` as the central pixel value type (e.g., `lw::Color`, `lw::ColorComponent`,
`lw::colorFromRGB`, `lw::colorR`). This naming is inconsistent with the domain — a `Color` is a single pixel's
encoded channel value, not a "color" in the abstract sense. The `PixelBus` abstraction already uses "pixel"
terminology for the storage layer (`_pixels`, `_scratchPixels`, `pixels()`), creating a terminology split:
the container layer says "pixel" but the value layer says "color." Renaming everything to `Pixel` unifies the
naming around a single consistent concept: a `Pixel` is the atomic unit of LED data, with `PixelComponent`
being a single channel of that pixel.

Additionally, `src/protocols/IProtocol.h` has a latent bug: it includes `"colors/Color.h"` (non-existent path)
instead of `"core/Color.h"`. This will be fixed as part of the rename.

### Scope Summary

| Category | Current Name | New Name |
|----------|-------------|----------|
| Type | `lw::Color` | `lw::Pixel` |
| Type | `lw::ColorComponent` | `lw::PixelComponent` |
| Constant | `lw::ColorComponentBitDepth` | `lw::PixelComponentBitDepth` |
| Constant | `lw::colorBits` | `lw::pixelBits` |
| Constant | `lw::colorMask` | `lw::pixelMask` |
| Function | `colorR/G/B/W()` | `pixelR/G/B/W()` |
| Function | `setColorR/G/B/W()` | `setPixelR/G/B/W()` |
| Function | `colorFromRGB/W()` | `pixelFromRGB/W()` |
| Function | `colorComponentByTag()` | `pixelComponentByTag()` |
| Function | `setColorComponentByTag()` | (removed — replaced by `mapChannels`) |
| Function | `colorComponentByIndex()` | (removed — uncalled) |
| Function | `colorCompare()` | (removed — uncalled) |
| Function | `serializeColor()` | (removed — uncalled) |
| Function | `parseColor()` | (removed — uncalled) |
| Function | `applyBrightness()` | (moved to `BrightnessShader` as private static) |
| Macro | `LW_COLOR_COMPONENT_SIZE` | `LW_PIXEL_COMPONENT_SIZE` |
| File | `src/core/Color.h` | `src/core/Pixel.h` |
| Alias (protocols) | `ColorType` | `PixelType` |
| Alias (DotStar) | `InterfaceColorType` | `InterfacePixelType` |
| Field | `PaletteStop::color` | (kept — field name stays `color`, type changes to `lw::Pixel`) |
| Field | `PaletteSampleOptions::outOfRangeColor` | `PaletteSampleOptions::outOfRangePixel` |
| Field | `PaletteSampleOptions::brightnessScale` (keeps `ColorComponent` in type) | type becomes `PixelComponent` |
| Include path | `"colors/Color.h"` (bug) | `"core/Pixel.h"` (fix) |

### Files in `src/palettes/` that carry `Color` in their filename

| Current File | Decision |
|-------------|----------|
| `src/palettes/ColorMath.h` | **Kept** per CPR-DEC-2 — filename stays; only internal type references change |

## Open Decisions

| ID | Status | Decision | Notes |
|----|--------|----------|-------|
| CPR-DEC-1 | done | Rename `PaletteStop::color` field to `PaletteStop::pixel`? | **Keep `color`.** The palette domain is inherently about colors; `PaletteStop::color` reads naturally as "the color at this stop." The type `lw::Pixel` in the field declaration already conveys that it's a pixel value. |
| CPR-DEC-2 | done | Rename `src/palettes/ColorMath.h` to `src/palettes/PixelMath.h`? | **Keep `ColorMath.h`.** The file does math in the color domain (blends, easing); the filename accurately describes its purpose. The types it operates on changing to `Pixel` doesn't change the domain concept. |
| CPR-DEC-3 | done | Rename `src/palettes/ColorMath.h` functions like `linearBlendProgress` that take `Color` params? | **No rename needed.** These function names don't carry "color" in them — only the parameter types change from `Color` to `Pixel`. |
| CPR-DEC-4 | done | Should `color` local variable names in generators/Blends/etc. be renamed to `pixel`? | **Keep `color` for local variables.** Local variable names like `lw::Pixel color{}` are fine — the type communicates "pixel" while the variable name communicates its role (a color value). Only type references change. |
| CPR-DEC-5 | done | Do we provide backward-compat aliases (`using Color = Pixel`) during transition? | **No compat aliases.** Project is alpha with no API stability guarantee. Clean break only.

## Phase 1 — Core Type Rename (`src/core/`)

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-01 | done | Rename `src/core/Color.h` to `src/core/Pixel.h` and update all internal type/function/constant names | — | File renamed; `Color`→`Pixel`, `ColorComponent`→`PixelComponent`, `ColorComponentBitDepth`→`PixelComponentBitDepth`, `colorBits`→`pixelBits`, `colorMask`→`pixelMask`, `colorR/G/B/W`→`pixelR/G/B/W`, `setColorR/G/B/W`→`setPixelR/G/B/W`, `colorFromRGB/W`→`pixelFromRGB/W`, `colorComponentByTag`→`pixelComponentByTag`, `tryParseColor`→`tryParsePixel`; removed: `setPixelComponentByTag` (replaced by mapChannels), `pixelComponentByIndex`, `pixelCompare`, `serializePixel`/`parsePixel` (all uncalled), pixel-level `applyBrightness` (moved to BrightnessShader) |
| CPR-02 | done | Rename `LW_COLOR_COMPONENT_SIZE` to `LW_PIXEL_COMPONENT_SIZE` in `src/core/Compat.h` | — | Macro renamed; no remaining references to old name in Compat.h |
| CPR-03 | done | Update all `#include "core/Color.h"` to `#include "core/Pixel.h"` across `src/`, `test/`, `examples/` | CPR-01 | All 18 include directives updated; compilation succeeds |

## Phase 2 — Palette Layer (`src/palettes/`)

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-10 | done | Update `PaletteStop` field type: `ColorType = lw::Color` → `PixelType = lw::Pixel`; field name `color` stays per CPR-DEC-1 | CPR-01 | Type alias renamed; field `color` retains name, type becomes `lw::Pixel` |
| CPR-11 | done | Update `PaletteSampleOptions` field `outOfRangeColor` → `outOfRangePixel`, and type `ColorComponent` → `PixelComponent` in `Types.h` | CPR-01 | Fields renamed; type updated |
| CPR-12 | done | Update `ColorType` → `PixelType` in `PaletteStop`, `IPalette`, `Palette` in `Types.h` | CPR-01 | Alias renamed |
| CPR-13 | done | Update `IsPaletteLike` trait to reference `PixelType` in `Traits.h` | CPR-12 | Trait references new alias name |
| CPR-14 | done | Update `src/palettes/ColorMath.h` — `lw::Color` → `lw::Pixel`, `lw::ColorComponent` → `lw::PixelComponent` internally; filename stays per CPR-DEC-2 | CPR-01, CPR-DEC-2 | All type references updated; filename unchanged |
| CPR-15 | done | Update `BlendOperations.h` — `lw::Color` → `lw::Pixel`, `lw::ColorComponent` → `lw::PixelComponent`, `lw::colorComponentByTag` → `lw::pixelComponentByTag` | CPR-01, CPR-03 | All type/function references updated |
| CPR-16 | done | Update `Blends.h` — `lw::Color` → `lw::Pixel` in all signatures and locals; field access `.color` stays per CPR-DEC-1 | CPR-10 | All type references updated; `.color` field access unchanged |
| CPR-17 | done | Update `Detail.h` — `lw::Color` → `lw::Pixel`, `lw::ColorComponent` → `lw::PixelComponent` | CPR-01 | All type references updated |
| CPR-18 | done | Update `Generators.h` — `lw::Color` → `lw::Pixel`, `lw::ColorComponent` → `lw::PixelComponent`; local var `color` and field `.color` stay per CPR-DEC-1 and CPR-DEC-4 | CPR-10, CPR-01 | All type references updated; local var names and field access unchanged |
| CPR-19 | done | Update `Sampling.h` — `value_type = lw::Color` → `value_type = lw::Pixel` | CPR-01 | Iterator value_type updated |
| CPR-20 | done | Update `SamplingTransition.h` — `BlendAssignProxy::operator=(const lw::Color&)` → `const lw::Pixel&`, `value_type` | CPR-01 | All type references updated |
| CPR-21 | done | Update `WrapModes.h` — `lw::Color` → `lw::Pixel`; field access `.color` stays per CPR-DEC-1 | CPR-10, CPR-01 | All type references updated; `.color` field access unchanged |

## Phase 3 — Protocol Layer (`src/protocols/`)

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-30 | done | Fix `#include "colors/Color.h"` → `#include "core/Pixel.h"` and update `ColorType` → `PixelType` in `IProtocol.h` | CPR-01, CPR-03 | Bug fixed; alias renamed; `lw::colors::Color` → `lw::Pixel` |
| CPR-31 | done | Update `Protocol.h` — `ColorType` → `PixelType`, `lw::Color` → `lw::Pixel` | CPR-01 | Alias and type references updated |
| CPR-32 | done | Update `IShader.h` — `lw::Color` → `lw::Pixel` in `apply()` signature | CPR-01 | Signature updated |
| CPR-33 | done | Update `ShaderProtocol.h` — `lw::Color` → `lw::Pixel` | CPR-01 | All type references updated |
| CPR-34 | done | Update `BrightnessShader.h` — `BrightnessType = lw::ColorComponent` → `lw::PixelComponent`, `lw::Color` → `lw::Pixel` | CPR-01 | All type references updated |
| CPR-35 | done | Update `GammaShader.h` — `lw::Color` → `lw::Pixel` | CPR-01 | All type references updated |
| CPR-36 | done | Update `DotStarProtocol.h` — `InterfaceColorType` → `InterfacePixelType`, `lw::Color` → `lw::Pixel`, `lw::ColorComponent` → `lw::PixelComponent`, `lw::colorComponentByTag` → `lw::pixelComponentByTag` | CPR-01 | All type/function references updated |
| CPR-37 | done | Update `Ws2812xProtocol.h` — `lw::ColorComponent` → `lw::PixelComponent`, `lw::Color` → `lw::Pixel`, `lw::colorComponentByTag` → `lw::pixelComponentByTag` | CPR-01 | All type/function references updated; `static_assert` message updated |
| CPR-38 | done | Update `Lpd6803Protocol.h` — `lw::ColorComponent` → `lw::PixelComponent`, `lw::Color` → `lw::Pixel`, accessor functions | CPR-01 | All type/function references updated; `static_assert` message updated |
| CPR-39 | done | Update remaining protocol files (`Lpd8806`, `P9813`, `Pixie`, `Sm16716`, `Sm168x`, `Tlc59711`, `Tm1814`, `Tm1914`, `Ws2801`) — `ColorComponent` → `PixelComponent`, `lw::Color` → `lw::Pixel` | CPR-01 | All 8 remaining protocol files updated; all `static_assert` messages updated |

## Phase 4 — Bus Layer (`src/buses/`)

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-50 | done | Update `Bus.h` — `#include "core/Color.h"` → `"core/Pixel.h"`, `span<Color>` → `span<Pixel>` | CPR-01, CPR-03 | All includes and type references updated |
| CPR-51 | done | Update `PixelBus.h` — `span<Color>` → `span<Pixel>` | CPR-01 | All type references updated |
| CPR-52 | done | Update `StackPixelBus.h` — `span<Color>` → `span<Pixel>` | CPR-01 | All type references updated |
| CPR-53 | done | Update `ProtocolTransportPipeline.h` — includes and type references | CPR-01, CPR-03 | All includes and type references updated |

## Phase 5 — Remaining Core (`src/core/`)

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-60 | done | Update `OutputPipeline.h` — `write(span<const lw::Color>)` → `write(span<const lw::Pixel>)` | CPR-01 | Signature updated |
| CPR-61 | done | Update `IPixelBus.h` — includes and type references | CPR-01, CPR-03 | All references updated |
| CPR-62 | done | Update `Fill.h` — includes and type references | CPR-01, CPR-03 | All references updated |
| CPR-63 | done | Update `KelvinToRgbStrategies.h` — `ColorComponent` → `PixelComponent` | CPR-01 | All references updated |

## Phase 6 — Public API (`src/LumaWave.h`)

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-70 | done | Update all `using` declarations: `Color` → `Pixel`, and all `using lw::color*` → `using lw::pixel*` | CPR-01 | All 16 using declarations updated; `using Pixel = lw::Pixel`; `using lw::pixelB/G/R/W`, `using lw::pixelCompare`, `using lw::pixelComponentByIndex/Tag`, `using lw::pixelFromRGB/W`, `using lw::parsePixel`, `using lw::serializePixel`, `using lw::setPixelB/G/R/W`, `using lw::setPixelComponentByTag`, `using lw::tryParsePixel` |

## Phase 7 — Tests

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-80 | done | Update `test/busses/test_bus/test_main.cpp` — all `lw::Color` → `lw::Pixel`, `lw::colorFromRGB` → `lw::pixelFromRGB`, `lw::colorR/G/B` → `lw::pixelR/G/B`, local `color`/`pixel` var names | CPR-01 | All type/function references updated; tests compile and pass |
| CPR-81 | done | Update `test/contracts/test_palette_first_pass_compile/test_main.cpp` — `lw::Color` → `lw::Pixel`, `colorFromRGB` → `pixelFromRGB`; field `.color` stays per CPR-DEC-1 | CPR-10, CPR-01 | All references updated; contract tests pass |
| CPR-82 | done | Update `test/lights/test_print_light_driver/test_main.cpp` — `lw::Color` → `lw::Pixel`, `colorFromRGB` → `pixelFromRGB`, local var `color` → `pixel` | CPR-01 | All references updated; tests compile and pass |
| CPR-83 | done | Update `test/protocols/test_protocol_spec_sections_1_1_to_1_4_and_1_14/test_main.cpp` — all `lw::Color` → `lw::Pixel`, `colorFromRGB` → `pixelFromRGB`, local `colors` vars → `pixels` | CPR-01, CPR-39 | All references updated; tests compile and pass |
| CPR-84 | done | Update `test/protocols/test_protocol_spec_sections_1_5_to_1_13/test_main.cpp` — all `lw::Color` → `lw::Pixel`, `colorFromRGB` → `pixelFromRGB`, local `colors` vars → `pixels` | CPR-01, CPR-39 | All references updated; tests compile and pass |
| CPR-85 | done | Run full test suite (`ctest`) and confirm all tests pass | CPR-80, CPR-81, CPR-82, CPR-83, CPR-84 | `ctest --test-dir build -C Debug --output-on-failure` exits 0 with no failures |

## Phase 8 — Examples

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-90 | done | Update all `examples/` .ino and .cpp files: `lw::Color` → `lw::Pixel`, `Color` → `Pixel`, `colorFromRGB` → `pixelFromRGB`, `Protocol::ColorType` → `Protocol::PixelType`, local vars | CPR-01, CPR-31, CPR-70 | All examples updated; `lw::Color` no longer appears in examples/ |
| CPR-91 | done | Update `include/.gitkeep` — `core/Color.h` → `core/Pixel.h`, `Rgb8Color` → `Rgb8Pixel` | CPR-01 | include stub updated |

## Phase 9 — Documentation

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-100 | done | Update `docs/comparison-lumawave-vs-fastled.md` — `Color`/`span<Color>` → `Pixel`/`span<Pixel>` | CPR-01 | All doc references updated |
| CPR-101 | done | Update `docs/usage/bus-builder.md` — `lw::Color` → `lw::Pixel` | CPR-01 | All doc references updated |
| CPR-102 | done | Update `docs/usage/compilation-flags.md` — `LW_COLOR_COMPONENT_SIZE` → `LW_PIXEL_COMPONENT_SIZE` | CPR-02 | All doc references updated |
| CPR-103 | done | Update `docs/usage/palette-parsing.md` — `TColor` → `TPixel`, `Color` → `Pixel` | CPR-01 | All doc references updated |
| CPR-104 | done | Update `docs/internal/` docs — `Color` → `Pixel`, `span<Color>` → `span<Pixel>` | CPR-01 | All 4 internal docs updated |
| CPR-105 | done | Update `docs/backlog/plan/bus-builder-lifetime-simplification.md` — `Color` → `Pixel` | CPR-01 | Plan doc references updated |

## Phase 10 — Validation & Cleanup

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|---------------------|
| CPR-110 | done | Full-text search for remaining `lw::Color` (not in comments/docs about the rename) | All prior phases | `grep` for `lw::Color\b` across `src/`, `test/`, `examples/` returns zero code hits |
| CPR-111 | done | Full-text search for remaining `ColorComponent` (not in comments/docs about the rename) | All prior phases | `grep` for `ColorComponent\b` across `src/`, `test/`, `examples/` returns zero code hits |
| CPR-112 | done | Full-text search for remaining `colorFromRGB\|colorR\|colorG\|colorB\|colorW` function calls | All prior phases | `grep` returns zero code hits |
| CPR-113 | done | Search for remaining `#include.*Color\.h"` include directives | All prior phases | `grep` returns zero hits |
| CPR-114 | done | CMake Configure + Build (native test target) | CPR-110, CPR-111, CPR-112, CPR-113 | `cmake --build build --config Debug` succeeds with no errors |
| CPR-115 | done | Run clang-format across all changed files | CPR-114 | `clang-format -i --style=file` on all modified files with no unexpected changes |
| CPR-116 | done | Resolve all open decisions (CPR-DEC-1 through CPR-DEC-5) and update this document | All prior phases | All decision rows marked `done` with resolution notes |
