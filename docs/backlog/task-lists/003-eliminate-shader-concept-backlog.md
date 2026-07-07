# Eliminate Shader Concept Backlog

Purpose: Remove the `IShader` abstraction and all shader integration from the library. The shader concept is too implementation-specific for a general-purpose LED library — gamma correction, white balance, and CCT balance are eliminated entirely (to be re-added later if needed), and current limiting is a transport/bus-level concern. None belong in the bus pipeline as a separate seam.

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
- Gamma correction eliminated entirely (re-add later if needed)
- White balance / CCT balance eliminated entirely (re-add later if needed)
- Current limiting absorbed into transport/bus layer (per-transport power budget)
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

- [ ] **`P2a`** — Delete `src/colors/GammaShader.h`. *(Eliminated entirely — re-add later if needed.)*
- [ ] **`P2b`** — Delete `src/colors/CurrentLimiterShader.h`. *(Current limiting moves to transport/bus layer — see Phase 5.)*
- [ ] **`P2c`** — Delete `src/colors/AutoWhiteBalanceShader.h`. *(Eliminated entirely — re-add later if needed.)*
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

### Phase 5 — Transport/bus-layer feature (current limiting)

Current limiting is a transport/bus-level concern — the bus knows pixel count and protocol settings, and the transport knows the hardware power budget. Integrating current limiting into the bus pipeline avoids the need for a separate shader seam.

- [ ] **`P5a`** — Add optional `maxMilliamps`, `milliampsPerChannel`, `controllerMilliamps`, and `standbyMilliampsPerPixel` to bus-level settings (either in `PixelBus` constructor or a new `BusSettings` struct).
- [ ] **`P5b`** — Implement current estimation and per-frame scaling directly in `PixelBus::show()` (and `LightBus::show()` if applicable), gated by `maxMilliamps > 0`:
  - Estimate weighted draw from current pixel data.
  - If estimated draw exceeds budget, scale all channels proportionally before protocol encoding.
  - Retain the per-channel mA mapping from the old `CurrentLimiterShader`.
- [ ] **`P5c`** — Add a `currentLimiterEnabled()` / `lastEstimatedMilliamps()` accessor on the bus so users can query power state.
- [ ] **`P5d`** — Update `LightBus` analogously if single-light current limiting is meaningful (likely not needed — mark as `deferred`).

### Phase 6 — Tests

- [ ] **`P6a`** — Remove entire `test/shaders/` directory tree.
- [ ] **`P6b`** — `test/busses/test_static_bus_driver_pixel_bus/test_main.cpp`: Remove `IncrementRedShader`, `BrightnessOwnerShader`, and all shader integration test cases. Bus tests should verify pixel storage, show(), and brightness without shaders.
- [ ] **`P6c`** — `test/contracts/test_disable_template_combinatorial_types_compile/test_main.cpp`: Remove `NoOpShader` and any `AggregateShader` compile tests.
- [ ] **`P6d`** — `test/protocols/`: Remove or update any protocol tests that reference shaders or shader scratch buffers.
- [ ] **`P6e`** — `test/busses/`: Remove or update any bus tests that construct buses with shader parameters.
- [ ] **`P6f`** — `test/CMakeLists.txt`: Remove shader test subdirectories and update test registration.
- [ ] **`P6g`** — Search and update any remaining test files that reference `IShader`, `NilShader`, `Shader::*`, `_shader`, `_shaderScratch`, `UsesShaderScratch`, `shader()` accessor, or `shaderScratch()` accessor.

### Phase 7 — Examples

- [ ] **`P7a`** — Remove entire `examples/shaders/` directory tree.
- [ ] **`P7b`** — Review remaining `examples/` for any shader references or shader template arguments in bus/strip construction. Remove them.
- [ ] **`P7c`** — If new bus-layer features were added (Phase 5), consider adding a minimal current limiting example (e.g., `examples/features/current-limiter/`).

### Phase 8 — Documentation

- [ ] **`P8a`** — `docs/comparison-lumawave-vs-fastled.md`: Remove or rewrite all shader-related comparison points (section 1.2 "Color transforms — Shader pipeline", section 3.x if it references shaders).
- [ ] **`P8b`** — `docs/internal/information/object-model-contracts.md`: Remove `IShader` from the seam list (section 1.4). Update to reflect that brightness is always bus-owned.
- [ ] **`P8c`** — `docs/internal/information/cpp23-modules-reamortization-design.md`: Remove `lw.shader` module plans.
- [ ] **`P8d`** — `docs/internal/information/arduino-optional-plan.md`: Update references to `IShader` as an architectural seam.
- [ ] **`P8e`** — `docs/usage/compilation-flags.md`: Remove any shader-related compilation flags (e.g., `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` may still be relevant for `CompositeShader` — update docs or remove flag if no longer needed).
- [ ] **`P8f`** — `.github/copilot-instructions.md`: Update architecture seam descriptions — remove `IShader` from the seam list.
- [ ] **`P8g`** — `docs/backlog/task-lists/001-platform-library-split-backlog.md`: Update references to `lumawave_shader` library target, or mark as obsolete.
- [ ] **`P8h`** — Update `ReadMe.md` if it references shaders.

### Phase 9 — Build and configuration

- [ ] **`P9a`** — `CMakeLists.txt`: Remove `shader` source files from build. Remove any `lumawave_shader` library target if it exists.
- [ ] **`P9b`** — Review `src/CMakeLists.txt` or equivalent for shader file listings.
- [ ] **`P9c`** — Update `keywords.txt` to remove shader-related keywords.
- [ ] **`P9d`** — If `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` is no longer needed (only used for `CompositeShader`), consider removing the flag entirely.

### Phase 10 — Validation

- [ ] **`P10a`** — Configure and build native tests: `cmake -S . -B build && cmake --build build`
- [ ] **`P10b`** — Run full test suite: `ctest --test-dir build --output-on-failure`
- [ ] **`P10c`** — Verify no remaining references to `IShader`, `NilShader`, `Shader::`, `_shader`, `_shaderScratch`, `UsesShaderScratch`, `shaders::`, `BrightnessOwnership`, `applyBrightness`, `brightnessOwnership`, `GammaShader`, `CurrentLimiterShader`, `AutoWhiteBalanceShader`, `CCTWhiteBalanceShader`, `AggregateShader`, or `CompositeShader` in active source/test paths.
- [ ] **`P10d`** — Build any remaining example sketches to confirm they compile without shader references.

## Dependencies Between Phases

```text
Phase 1 (interface removal) ──► Phase 2 (concrete shaders) ──► Phase 3 (bus surgery)
                                                                     │
                     ┌───────────────────────────────────────────────┘
                     ▼
               Phase 4 (public surface)
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
     Phase 5                Phase 6
    (transport/             (tests)
     bus feat.)
          │                     │
          └──────────┬──────────┘
                     ▼
               Phase 7 (examples)
                     │
                     ▼
               Phase 8 (docs)
                     │
                     ▼
                Phase 9 (build)
                     │
                     ▼
              Phase 10 (validation)
```
               ▼
         Phase 10 (validation)
```

Phase 6 (tests) can run in parallel with Phase 5. Phases 7–9 must wait for the code changes to settle.

## Open Decisions

All resolved. See relevant phases for implementation details.

| Decision | Resolution | Rationale |
|---|---|---|
| Gamma correction future | **Eliminated entirely** — re-add later if desired | Too implementation-specific; protocols shouldn't carry gamma either. |
| White balance / CCT balance future | **Eliminated entirely** — re-add later if desired | Same rationale as gamma; Kelvin-to-RGB correction is an app concern. |
| Current limiting future | **Transport/bus layer** — integrated directly into `PixelBus::show()` pipeline | Bus knows pixel count + protocol; transport knows power budget. No separate seam needed. |
| `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` | Still open — keep until `CompositeShader` is the last remaining user | Affects P9d; resolve after Phase 2 if all shader implementations are gone. |
