# Eliminate Shader Concept Backlog

Purpose: Remove the `IShader` abstraction and all shader integration from the library. The shader concept is too implementation-specific for a general-purpose LED library — pixel transformations (gamma, current limiting, white balance) are either protocol-level concerns or application-level utilities that shouldn't be baked into the bus pipeline.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Not started.

## Motivation

The library defines four architectural seams: `IPixelBus`, `IProtocol`, `ITransport`, and `IShader`. The shader seam is the weakest:

| Issue | Details |
|---|---|
| **Too implementation-specific** | Gamma correction, current limiting, and white balance are application concerns; the bus shouldn't know about pixel transforms at all. |
| **Pipeline complexity** | `show()` has `UsesShaderScratch`, `brightnessOwnership()`, `applyBrightness()` — 3 branching paths just for optional transforms. |
| **Runtime overhead** | Scratch buffer allocation, copy-on-write, virtual dispatch through `AggregateShader` — all for features most users don't need. |
| **Brightness entanglement** | `brightnessOwnership()` creates a three-way tug-of-war between bus, shader, and protocol for who controls brightness. |
| **Low usage** | Out of 30+ protocol aliases, only 2 examples use shaders. The majority of users just want `Strip<Protocol>.show()`. |
| **Maintenance tax** | 7 source files, 15+ test files, 3 examples, public namespace aliases — all for a feature with questionable API stability. |

## Target Shape After This Backlog

- `PixelBus<TProtocol, TTransport>` — no `TShader` parameter
- `LightBus<TColor, TDriver>` — no `TShader` parameter
- `ReferenceLightBus<TColor>` — no `unique_ptr<IShader<TColor>>`
- Bus `show()` copies root pixels directly to protocol input — no scratch buffer, no shader pipeline
- Bus always owns brightness — `setBrightness()` scales directly in `show()`
- Gamma correction absorbed into protocol layer (per-chip characteristic) or removed
- Current limiting / white balance / other transforms are standalone utilities — users call them on pixel data before `show()` if desired
- Removed: `IShader<TColor>`, `NilShader<TColor>`, `GammaShader<TColor>`, `CurrentLimiterShader<TColor>`, `AutoWhiteBalanceShader<TColor>`, `CCTWhiteBalanceShader<TColor>`, `AggregateShader<TColor>`, `CompositeShader<TColor, ...>`
- Removed: `shaders::BrightnessOwnership`, `shaders::detail::ShaderBrightnessTraits`
- Removed: `namespace Shader { ... }` public alias block

## Non-Goals

- Do not change the `IProtocol` or `ITransport` seams.
- Do not change color types, channel infrastructure, or palette code.
- Do not remove the brightness feature from the bus — bus-owned brightness stays.
- Do not change the factory or descriptor system (that's a separate concern).
- Do not add compatibility shims for removed shader APIs.

## Task List

### Phase 1 — Interface and NilShader removal

- [ ] **`P1a`** — Delete `src/colors/IShader.h` entirely.
- [ ] **`P1b`** — Delete `src/colors/NilShader.h` entirely.
- [ ] **`P1c`** — Remove `#include "colors/IShader.h"` and `#include "colors/NilShader.h"` from `Colors.h`.
- [ ] **`P1d`** — Remove all `shaders::` namespace declarations from `src/core/Compat.h` or any other core headers that reference them.
- [ ] **`P1e`** — Remove the `lw::IShader<TColor>` / `lw::NilShader<TColor>` namespace-level using-declarations.

### Phase 2 — Concrete shader implementations

- [ ] **`P2a`** — Delete `src/colors/GammaShader.h`. *(Gamma correction should be either absorbed into protocol settings or removed — decide in Phase 5.)*
- [ ] **`P2b`** — Delete `src/colors/CurrentLimiterShader.h`. *(Current limiting becomes a standalone utility — see Phase 5 utilities track.)*
- [ ] **`P2c`** — Delete `src/colors/AutoWhiteBalanceShader.h`. *(White balance becomes a standalone utility — see Phase 5 utilities track.)*
- [ ] **`P2d`** — Delete `src/colors/CCTWhiteBalanceShader.h`. *(Already scheduled for removal in backlog 002 — expedite.)*
- [ ] **`P2e`** — Delete `src/colors/AggregateShader.h` (includes `AggregateShader`, `CompositeShader`, `AggregateShaderSettings`).
- [ ] **`P2f`** — Remove all shader `#include` lines from `src/colors/Colors.h`.

### Phase 3 — Bus surgery

#### 3a — `PixelBus.h`

- [ ] **`P3a1`** — Remove `#include "colors/IShader.h"` and `#include "colors/NilShader.h"` from `PixelBus.h`.
- [ ] **`P3a2`** — Remove `TShader` template parameter from `PixelBus` class template. Signature becomes `template <typename TProtocol, typename TTransport = PlatformDefaultTransport>`.
- [ ] **`P3a3`** — Remove `ShaderType` typedef, `UsesShaderScratch` constexpr, `_shader` member, `_shaderScratch` member.
- [ ] **`P3a4`** — Remove all `ShaderType`-related static_asserts.
- [ ] **`P3a5`** — Remove all constructors that accept `ShaderType shaderInstance`. Keep only the shader-less overloads (renumber as default constructors).
- [ ] **`P3a6`** — Simplify `show()`: remove the `UsesShaderScratch` branch, remove `shaderOwnership`/`brightnessAppliedUpstream` tracking. Protocol input is always `_rootPixels` directly. Bus-level brightness always applies if not max.
- [ ] **`P3a7`** — Remove `shaderScratch()` accessor methods (if any remain).

#### 3b — `LightBus.h`

- [ ] **`P3b1`** — Remove `#include "colors/IShader.h"` and `#include "colors/NilShader.h"` from `LightBus.h`.
- [ ] **`P3b2`** — Remove `TShader` template parameter. Signature becomes `template <typename TColor = DefaultColorType, typename TDriver = PlatformDefaultLightDriver<TColor>>`.
- [ ] **`P3b3`** — Remove `ShaderType` typedef, `UsesShaderScratch`, `_shader` member, `_shaderScratch` member.
- [ ] **`P3b4`** — Remove shader-related static_asserts.
- [ ] **`P3b5`** — Remove constructor overloads accepting `ShaderType shader`.
- [ ] **`P3b6`** — Simplify `show()`: remove scratch copy, shader apply, brightness ownership check. Output pixel is always `_rootPixel[0]`.
- [ ] **`P3b7`** — Remove `shader()` accessor and `shaderScratch()` accessor.

#### 3c — `ReferenceLightBus.h`

- [ ] **`P3c1`** — Remove `#include "colors/IShader.h"` from `ReferenceLightBus.h`.
- [ ] **`P3c2`** — Remove `unique_ptr<IShader<TColor>> _shader` member and `unique_ptr<TColor> _shaderBuffer` member.
- [ ] **`P3c3`** — Remove shader parameter from constructor.
- [ ] **`P3c4`** — Simplify `show()`: remove shader branching, copy, and `applyBrightness` logic. Output is always `*_rootBuffer`.
- [ ] **`P3c5`** — Remove `shader()` accessor and `shaderBuffer()` accessor.

### Phase 4 — Public surface (`LumaWave.h`)

- [ ] **`P4a`** — Remove the entire `namespace Shader { ... }` block (all aliases for `IShader`, `NilShader`, `AggregateShader`, `CompositeShader`, `GammaShader`, `CurrentLimiterShader`, `AutoWhiteBalanceShader`, `CCTWhiteBalanceShader`, settings types, Kelvin strategies, `CCTInterlock`).
- [ ] **`P4b`** — Remove `TShader` default template argument from `Strip` alias. Signature becomes `template <typename TProtocol, typename TTransport = PlatformDefaultTransport> using Strip = ...`.
- [ ] **`P4c`** — Remove `TShader` default template argument from `Light` alias. Signature becomes `template <typename TColor = DefaultColorType, typename TDriver = PlatformDefaultLightDriver<TColor>> using Light = ...`.
- [ ] **`P4d`** — Remove `ReferenceLight` shader support (no change needed — it takes no shader template param, just remove runtime shader support in Phase 3).
- [ ] **`P4e`** — Clean up any remaining comments or references to shaders in `LumaWave.h`.

### Phase 5 — Absorb gamma into protocols (if desired)

- [ ] **`P5a`** — **Decision**: Determine whether gamma correction should be absorbed into the protocol layer. Options:
  - **Option A** (recommended): Add optional `gamma` field to protocol settings structs for protocols where gamma is a known chip characteristic (e.g., WS2812, SK6812). Protocol applies gamma during `update()`.
  - **Option B**: Gamma is purely application-level — remove from library entirely and let users implement it as a pixel pre-processing step.
  - **Option C**: Gamma becomes a standalone `applyGamma(span<TColor>, float)` free function in `ColorMath.h`.
- [ ] **`P5b`** — If Option A: Add `GammaSettings` to relevant protocol settings (likely `Ws2812xProtocolSettings`, `DotStarProtocolSettings`, etc.). Apply gamma LUT during `IProtocol::update()`.
- [ ] **`P5c`** — If Option A: Update protocol static_asserts and `requiredBufferSizeBytes()` if gamma changes encoding requirements.
- [ ] **`P5d`** — If Option C: Add `applyGamma()` / `applyGammaInPlace()` free functions to `ColorMath.h` or a new `Gamma.h`.

### Phase 6 — Standalone utility track

- [ ] **`P6a`** — **Decision**: Determine fate of `CurrentLimiterShader` functionality. Options:
  - **Option A** (recommended): Move to a standalone free function `estimateCurrent(span<TColor>, ChannelMilliampsMap) -> uint32_t` in a utility header. Users call before `show()` and scale pixels themselves.
  - **Option B**: Remove entirely — current limiting is out of scope for this library.
- [ ] **`P6b`** — **Decision**: Determine fate of `AutoWhiteBalanceShader` functionality. Options:
  - **Option A** (recommended): Move to a standalone free function `applyWhiteBalance(span<TColor>, uint16_t kelvin)` in a utility header.
  - **Option B**: Remove entirely — white balance is application-specific.
- [ ] **`P6c`** — If elected, create `src/utilities/CurrentLimiter.h` with standalone functions.
- [ ] **`P6d`** — If elected, create `src/utilities/WhiteBalance.h` with standalone functions.
- [ ] **`P6e`** — If elected, create `src/utilities/Gamma.h` with standalone functions (if Option C from P5a).
- [ ] **`P6f`** — Add `#include` aggregation in a new or existing utility header umbrella.
- [ ] **`P6g`** — Update `src/LumaWave.h` to expose utility functions if they're part of the public surface.

### Phase 7 — Tests

- [ ] **`P7a`** — Remove entire `test/shaders/` directory tree.
- [ ] **`P7b`** — `test/busses/test_static_bus_driver_pixel_bus/test_main.cpp`: Remove `IncrementRedShader`, `BrightnessOwnerShader`, and all shader integration test cases. Bus tests should verify pixel storage, show(), and brightness without shaders.
- [ ] **`P7c`** — `test/contracts/test_disable_template_combinatorial_types_compile/test_main.cpp`: Remove `NoOpShader` and any `AggregateShader` compile tests.
- [ ] **`P7d`** — `test/protocols/`: Remove or update any protocol tests that reference shaders or shader scratch buffers.
- [ ] **`P7e`** — `test/busses/`: Remove or update any bus tests that construct buses with shader parameters.
- [ ] **`P7f`** — `test/CMakeLists.txt`: Remove shader test subdirectories and update test registration.
- [ ] **`P7g`** — Search and update any remaining test files that reference `IShader`, `NilShader`, `Shader::*`, `_shader`, `_shaderScratch`, `UsesShaderScratch`, `shader()` accessor, or `shaderScratch()` accessor.

### Phase 8 — Examples

- [ ] **`P8a`** — Remove entire `examples/shaders/` directory tree.
- [ ] **`P8b`** — Review remaining `examples/` for any shader references or shader template arguments in bus/strip construction. Remove them.
- [ ] **`P8c`** — If standalone utility functions were created (Phase 6), consider adding minimal utility examples to `examples/utilities/` or similar.

### Phase 9 — Documentation

- [ ] **`P9a`** — `docs/comparison-lumawave-vs-fastled.md`: Remove or rewrite all shader-related comparison points (section 1.2 "Color transforms — Shader pipeline", section 3.x if it references shaders).
- [ ] **`P9b`** — `docs/internal/information/object-model-contracts.md`: Remove `IShader` from the seam list (section 1.4). Update to reflect that brightness is always bus-owned.
- [ ] **`P9c`** — `docs/internal/information/cpp23-modules-reamortization-design.md`: Remove `lw.shader` module plans.
- [ ] **`P9d`** — `docs/internal/information/arduino-optional-plan.md`: Update references to `IShader` as an architectural seam.
- [ ] **`P9e`** — `docs/usage/compilation-flags.md`: Remove any shader-related compilation flags (e.g., `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` may still be relevant for `CompositeShader` — update docs or remove flag if no longer needed).
- [ ] **`P9f`** — `.github/copilot-instructions.md`: Update architecture seam descriptions — remove `IShader` from the seam list.
- [ ] **`P9g`** — `docs/backlog/task-lists/001-platform-library-split-backlog.md`: Update references to `lumawave_shader` library target, or mark as obsolete.
- [ ] **`P9h`** — Update `ReadMe.md` if it references shaders.

### Phase 10 — Build and configuration

- [ ] **`P10a`** — `CMakeLists.txt`: Remove `shader` source files from build. Remove any `lumawave_shader` library target if it exists.
- [ ] **`P10b`** — Review `src/CMakeLists.txt` or equivalent for shader file listings.
- [ ] **`P10c`** — Update `keywords.txt` to remove shader-related keywords.
- [ ] **`P10d`** — If `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` is no longer needed (only used for `CompositeShader`), consider removing the flag entirely.

### Phase 11 — Validation

- [ ] **`P11a`** — Configure and build native tests: `cmake -S . -B build && cmake --build build`
- [ ] **`P11b`** — Run full test suite: `ctest --test-dir build --output-on-failure`
- [ ] **`P11c`** — Verify no remaining references to `IShader`, `NilShader`, `Shader::`, `_shader`, `_shaderScratch`, `UsesShaderScratch`, `shaders::`, `BrightnessOwnership`, `applyBrightness`, `brightnessOwnership`, `GammaShader`, `CurrentLimiterShader`, `AutoWhiteBalanceShader`, `CCTWhiteBalanceShader`, `AggregateShader`, or `CompositeShader` in active source/test paths.
- [ ] **`P11d`** — Build any remaining example sketches to confirm they compile without shader references.

## Dependencies Between Phases

```text
Phase 1 (interface removal) ──► Phase 2 (concrete shaders) ──► Phase 3 (bus surgery)
                                                                    │
                    ┌───────────────────────────────────────────────┘
                    ▼
              Phase 4 (public surface)
                    │
                    ▼
         ┌──────────┼──────────┐
         ▼          ▼          ▼
    Phase 5     Phase 6     Phase 7
   (protocol   (utilities)  (tests)
    gamma)
         │          │          │
         └──────────┼──────────┘
                    ▼
              Phase 8 (examples)
                    │
                    ▼
              Phase 9 (docs)
                    │
                    ▼
             Phase 10 (build)
                    │
                    ▼
             Phase 11 (validation)
```

Phases 5–6 are optional (decision points). Phase 7 (tests) can run in parallel with 5–6. Phases 8–10 must wait for the code changes to settle.

## Open Decisions

These need to be resolved before or during Phase 5–6:

| Decision | Options | Impact |
|---|---|---|
| Gamma correction future | Protocol layer / Standalone utility / Remove entirely | Affects P2a, P5a-d, P6e |
| Current limiting future | Standalone utility / Remove | Affects P2b, P6a, P6c |
| White balance future | Standalone utility / Remove | Affects P2c, P6b, P6d |
| `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` | Keep / Remove | Affects P10d (only relevant if `CompositeShader` is the last user) |
