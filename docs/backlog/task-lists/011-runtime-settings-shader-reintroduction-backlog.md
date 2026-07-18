# Runtime Settings & Shader Reintroduction Backlog

Purpose: Reintroduce `IShader` as a pipeline-local pixel-transform seam, convert brightness from inline code to a reference-bound `BrightnessShader`, re-introduce `AggregateShader` for runtime shader composition, and re-introduce `CurrentLimiterShader` for power budgeting. Integrate everything through `ProtocolTransportPipeline`. No auto-injection — all shaders are user-explicit.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Design written at `docs/internal/information/runtime-settings-and-shader-reintroduction-design.md`. No code started.

## Motivation

### Reference-Bound Shaders

Shaders bind to external configuration values via constructor `const&`. The user declares and owns the variable, passes a reference into the shader constructor, and mutates the variable directly. No parameter structs through the pipeline, no notification hooks, no auto-injection. The pipeline is a pure shader → encode → transmit engine with zero knowledge of brightness, gamma, or any other configuration value.

### Shader Reintroduction

Shaders were removed in backlog 003. The old integration placed shaders as a `TShader` template parameter on `Bus`, entangled `brightnessOwnership()` across three seams, and carried `AggregateShader`/`CompositeShader` template machinery. The new design places shaders inside `ProtocolTransportPipeline` as a `vector<unique_ptr<IShader>>` — no template parameter on `Bus`, no brightness entanglement, no variadic templates.

### Brightness as a Shader

Currently brightness is applied via an inline per-pixel loop in `ProtocolTransportPipeline::write()`. Converting it to a `BrightnessShader` means all frame-level pixel transforms flow through the same mechanism. The user explicitly adds a `BrightnessShader` to the shader list, binding it to a `const BrightnessType&` (typically `bus.brightnessRef()`).

### AggregateShader

A standalone `IShader` implementation that owns an ordered list of shaders and applies them in sequence. Useful for reusing a shader chain across multiple pipeline instances or building chains dynamically at runtime.

### CurrentLimiterShader

Power budgeting (limiting total current draw based on per-channel milliamps) is a common LED application concern. Reintroduced as an optional shader, binding to a `const Settings&` at construction.

## Target Shape After This Backlog

- `IShader` in `src/core/IShader.h` — `virtual void apply(span<Color>) = 0`. No configuration parameters in `apply()`. Config is bound via constructor references.
- `BrightnessShader` in `src/core/BrightnessShader.h` — binds to external `const BrightnessType&` at construction. No-op when value is max.
- `OutputPipeline::write(span<Color>)` — no brightness parameter, no params. Pure shader → encode → transmit.
- `IPixelBus` unchanged.
- `Bus` keeps `_brightness`, adds `brightnessRef()`. `setBrightness()` unchanged.
- `ProtocolTransportPipeline` owns `vector<unique_ptr<IShader>>`. No auto-injection. No inline brightness loop.
- `AggregateShader` in `src/buses/AggregateShader.h` — dynamic shader chain composition.
- `CurrentLimiterShader` in `src/core/CurrentLimiterShader.h` — reference-bound settings, power-budgeting logic.
- Zero-copy fast path: when `_shaders` is empty, pixels go directly to `protocol->update()`.
- `Protocol` and `Transport` have zero changes.

## Non-Goals

- Do not add a shader template parameter to `Bus` or `IPixelBus`.
- Do not reintroduce `brightnessOwnership()`, `UsesShaderScratch`, `PipelineParameters`, or any old shader machinery.
- Do not reintroduce `CompositeShader` (variadic template composition).
- Do not add `IShader::begin()` — deferred.
- Do not pass configuration values through `write()` or `apply()` — shaders bind at construction.
- Do not auto-inject any shaders into the pipeline.
- Do not change construction-time settings (protocol/transport settings structs).

## Task List

### Phase 1 — IShader Interface + OutputPipeline Cleanup

- [ ] **`P1a`** — Create `src/core/IShader.h` with `IShader` class: `virtual void apply(span<colors::Color> pixels) = 0`.
- [ ] **`P1b`** — Update `OutputPipeline.h`: remove `BrightnessType` from `write()`. Signature becomes `virtual void write(span<const Color> colors)`.
- [ ] **`P1c`** — Update all `OutputPipeline` implementations (all light drivers + `ProtocolTransportPipeline`): match new `write(span<const Color>)` signature. Remove any brightness-related code.
- [ ] **`P1d`** — Update `LumaWave.h`: add `#include "core/IShader.h"` and `using IShader = lw::IShader;`.
- [ ] **`P1e`** — Build and run native tests.

### Phase 2 — BrightnessShader

- [ ] **`P2a`** — Create `src/core/BrightnessShader.h` with `BrightnessShader` class implementing `IShader`:
  - Constructor takes `const BrightnessType& brightnessRef`.
  - `apply()` reads `_brightness`; if `== max`, return immediately (no-op fast path).
  - Otherwise scales R, G, B, W channels per-pixel using `lw::colors::applyBrightness()`.
- [ ] **`P2b`** — Update `LumaWave.h`: add `#include "core/BrightnessShader.h"` and `using BrightnessShader = lw::BrightnessShader;`.
- [ ] **`P2c`** — Build and verify compilation.

### Phase 3 — ProtocolTransportPipeline Shader Integration

- [ ] **`P3a`** — Add `std::vector<std::unique_ptr<IShader>> _shaders` member.
- [ ] **`P3b`** — Add second constructor accepting `std::vector<std::unique_ptr<IShader>> shaders`. No auto-injection — user provides exactly what runs.
- [ ] **`P3c`** — Rewrite `write()`:
  - If `_shaders` non-empty: copy colors → `_scratchPixel`, apply each shader sequentially, encode from `_scratchPixel`.
  - Else: encode directly from input `colors` (zero-copy fast path).
  - No inline brightness loop.
- [ ] **`P3d`** — Build and run native tests.

### Phase 4 — Bus Brightness Ref

- [ ] **`P4a`** — Add `const BrightnessType& brightnessRef() const { return _brightness; }` to `Bus` in `src/buses/Bus.h`.
- [ ] **`P4b`** — Build and run native tests.

### Phase 5 — AggregateShader

- [ ] **`P5a`** — Create `src/buses/AggregateShader.h` with `AggregateShader` class implementing `IShader`:
  - Constructor takes `std::vector<std::unique_ptr<IShader>> shaders`.
  - `apply()` iterates and applies each shader in order.
  - Exposes `add(unique_ptr<IShader>)`, `clear()`, `size()`, `empty()`.
- [ ] **`P5b`** — Update `LumaWave.h`: add `#include "buses/AggregateShader.h"` and `using AggregateShader = lw::buses::AggregateShader;`.
- [ ] **`P5c`** — Build and verify compilation.

### Phase 6 — CurrentLimiterShader

- [ ] **`P6a`** — Create `src/core/CurrentLimiterShader.h` with `CurrentLimiterShader` class implementing `IShader`:
  - Define `CurrentLimiterShaderSettings` struct with fields: `maxMilliamps`, `milliampsPerChannel` (using `ChannelMap<Color, uint16_t>`), `controllerMilliamps`, `standbyMilliampsPerPixel`, `rgbwDerating`.
  - Constructor takes `const CurrentLimiterShaderSettings& settings` (reference-bound).
  - Port `estimateWeightedDraw()` and `scaleAll()` logic from old implementation.
  - Expose `uint32_t lastEstimatedMilliamps() const`.
- [ ] **`P6b`** — Update `LumaWave.h`: add includes and `using` declarations for `CurrentLimiterShader` and `CurrentLimiterShaderSettings`.
- [ ] **`P6c`** — Build and verify compilation.

### Phase 7 — Tests

- [ ] **`P7a`** — `test/buses/test_pipeline/`: Test `ProtocolTransportPipeline` with 0 shaders — verify zero-copy fast path (no scratch buffer allocation).
- [ ] **`P7b`** — `test/buses/test_pipeline/`: Test with 1 shader — verify shader output appears in encoded bytes.
- [ ] **`P7c`** — `test/buses/test_pipeline/`: Test with 2 shaders — verify sequential application order.
- [ ] **`P7d`** — `test/core/test_brightness_shader/`: Test `BrightnessShader` standalone with external variable at levels 0, 128, 255 — verify scaling per channel.
- [ ] **`P7e`** — `test/core/test_brightness_shader/`: Test with max brightness — verify no-op (pixels unchanged).
- [ ] **`P7f`** — `test/core/test_brightness_shader/`: Test reference tracking — mutate external variable between `apply()` calls, verify shader picks up new value.
- [ ] **`P7g`** — `test/core/test_aggregate_shader/`: Test `AggregateShader` with 0, 1, N shaders — verify ordering, `add()`, `clear()`.
- [ ] **`P7h`** — `test/core/test_current_limiter_shader/`: Test `CurrentLimiterShader` — budget enforcement, no-op within budget, RGBW derating, `lastEstimatedMilliamps()` accuracy.
- [ ] **`P7i`** — `test/buses/test_bus/`: Test `Bus::brightnessRef()` — reference tracks `setBrightness()` mutations.
- [ ] **`P7j`** — `test/buses/test_pipeline/`: Test end-to-end shader chain (CurrentLimiterShader → BrightnessShader) — verify ordering and pixel output.
- [ ] **`P7k`** — Run full suite: `ctest --test-dir build --output-on-failure`.

### Phase 8 — Documentation

- [ ] **`P8a`** — Update `.github/copilot-instructions.md`: Add `IShader` back to seam list.
- [ ] **`P8b`** — Update `docs/internal/information/object-model-contracts.md`: Add `IShader` to seam list.
- [ ] **`P8c`** — Mark this backlog as completed.

## Dependencies Between Phases

```text
Phase 1 (IShader + OutputPipeline) ──► Phase 2 (BrightnessShader)
                                               │
                                               ▼
                                        Phase 3 (PTP integration) ──► Phase 4 (Bus ref)
                                               │
                             ┌─────────────────┼─────────────────┐
                             ▼                 ▼                 ▼
                     Phase 5 (Aggregate)  Phase 6 (CurrentLim)  Phase 7 (Tests)
                             │                 │                 │
                             └─────────────────┼─────────────────┘
                                               ▼
                                        Phase 8 (Docs)
```

Phases 5 and 6 can run in parallel after Phase 3.

## Open Decisions

| Decision | Status | Notes |
|---|---|---|
| `IShader::begin()` | **Defer.** | No concrete shader needs it yet. Add later as default no-op if required. |
| BrightnessShader auto-injection | **No.** | Pipeline is pure shader engine. User explicitly adds `BrightnessShader(bus.brightnessRef())` to the shader list. |
| Zero-copy fast path | **`_shaders.empty()`.** | When no shaders are installed, input colors go directly to `protocol->update()` without scratch buffer allocation. |
| Config mechanism | **Constructor `const&`.** | Shaders bind to external variables at construction. Mutating the variable is the notification mechanism. No structs passed through `write()` or `apply()`. |
| `CompositeShader` variadic template | **Not reintroduced.** | `AggregateShader` + `vector` covers all use cases. |
| `CurrentLimiterShader` template parameter | **Removed.** | Non-templated — works on `colors::Color` directly. |
