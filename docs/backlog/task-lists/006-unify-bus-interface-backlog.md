# Unify Bus Types & Move Brightness Lower — Backlog

Purpose: unify all bus types (`PixelBus`, `LightBus`, `ReferenceBus`, `ReferenceLightBus`, `CompositeBus`) into a single non-templated `Bus` class backed by a new `IOutputPipeline` seam. Move brightness application out of the bus layer and into the output pipeline, applied on-the-fly during protocol transformation without a separate scratch-buffer pre-pass.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

**Superseded.** Merged into [Backlog 007](007-combined-bus-unification-backlog.md) — combined bus unification, PixelView elimination, and brightness relocation.

## Motivation

### Problem 1: Four parallel bus class hierarchies for one job

| Bus Class | Template Params | Pixel Count | Output Mechanism | Ownership |
|---|---|---|---|---|
| `PixelBus<TProtocol, TTransport>` | 2 (protocol + transport) | N | protocol.encode → transport.transmit | value |
| `ReferenceBus` | 0 | N | protocol.encode → transport.transmit | unique_ptr |
| `LightBus<TDriver>` | 1 (driver) | 1 | driver.write(color, brightness) | value |
| `ReferenceLightBus` | 0 | 1 | driver.write(color, brightness) | unique_ptr |
| `CompositeBus<TBuses...>` | variadic | Σ | delegates to children | value |
| `AggregateBus` | 0 | Σ | delegates to children | unique_ptr |

All six classes implement the same `IPixelBus` interface. All six track dirtiness. All six delegate `show()` to something below. The only real differences are (a) what sits below the bus and (b) whether it's owned by value or by pointer. That's two axes of variation generating six classes.

### Problem 2: Brightness lives at the wrong layer

In `PixelBus` and `ReferenceBus`, brightness is applied **at the bus level** via an identical 14-line per-channel loop over a scratch buffer before protocol encoding:

```cpp
// Duplicated in PixelBus::show() AND ReferenceBus::show()
if (_brightness != max && !protocolInput.empty()) {
    for (size_t i = 0; i < count; ++i) {
        for (char channel : {'R', 'G', 'B', 'W'}) {
            dst[channel] = applyBrightness(src[channel], _brightness);
        }
    }
    protocolInput = scratch; // swap to dimmed copy
}
```

This is an output concern, not a bus concern. The bus should own pixels, track dirtiness, and delegate output. It should not know about per-channel brightness scaling.

In `LightBus` and `ReferenceLightBus`, brightness is already at the right level — passed to `driver.write(color, brightness)` where each driver applies it at PWM hardware level. This asymmetry means the two pipelines handle brightness differently for no good reason.

### Problem 3: Template combinatorial concerns

`PixelBus<TProtocol, TTransport>` and `CompositeBus<TBuses...>` are gated by `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` because every protocol×transport combination produces a distinct template instantiation. `LightBus` is exempt from this gate but adds its own template axis. A type-erased design eliminates this entire category of concern.

### Problem 4: `TransportBrightness` is vestigial

`TransportBrightness` exists on `ITransport::transmitBytes()` but is always called with `upstreamApplied=false` and ignored by every real transport. It's dead code that exists only because brightness was applied at the bus layer and the transport might theoretically need to know. Moving brightness into the pipeline eliminates the need for this struct entirely.

## Target Shape After This Backlog

```
IPixelBus (non-templated)
    │
    ├── Bus  (NEW — single non-templated class)
    │       │
    │       └── std::unique_ptr<IOutputPipeline>  (NEW seam)
    │               ├── LightOutputPipeline        (wraps ILightDriver)
    │               └── ProtocolTransportPipeline  (wraps IProtocol + ITransport)
    │
    └── AggregateBus  (kept — composes multiple IPixelBus*)
```

### `IOutputPipeline` — the new seam

```cpp
class IOutputPipeline {
public:
    virtual ~IOutputPipeline() = default;
    virtual void begin() = 0;
    virtual bool isReadyToUpdate() const = 0;
    virtual void write(span<const Color> colors, BrightnessType brightness) = 0;
    virtual bool alwaysUpdate() const { return false; }
};
```

Replaces the dual role of `ILightDriver` (for single-pixel) and `IProtocol`+`ITransport` (for strips) as the thing the bus delegates to.

### `Bus` — the only bus class

- Non-templated. Owns `std::unique_ptr<IOutputPipeline>`.
- Accepts `span<Color>` for pixel storage in constructor (caller provides the buffer).
- `show()` delegates entirely to `_pipeline->write(pixels, _brightness)`.
- No brightness logic. No scratch buffer. No protocol/transport/driver knowledge.
- ~50 lines total.

### Brightness: on-the-fly during protocol transformation

- `ProtocolTransportPipeline::write()` applies brightness **inline** during the color→bytes transformation — no separate pre-pass, no full-frame scratch buffer.
- `LightOutputPipeline::write()` delegates to `ILightDriver::write(color, brightness)` — brightness applied at PWM hardware level as today.
- `TransportBrightness` struct removed from `ITransport` — no longer needed.

### What gets deleted

| File | Lines | Reason |
|---|---|---|
| `src/buses/PixelBus.h` | ~358 | Replaced by `Bus` + `ProtocolTransportPipeline` |
| `src/buses/LightBus.h` | ~120 | Replaced by `Bus` + `LightOutputPipeline` |
| `src/buses/ReferenceBus.h` | ~160 | Replaced by `Bus` (already type-erased) |
| `src/buses/ReferenceLightBus.h` | ~110 | Replaced by `Bus` (already type-erased) |
| `src/buses/CompositeBus.h` | ~130 | Replaced by `AggregateBus` (or non-templated equivalent) |
| `TransportBrightness` (in `ITransport.h`) | ~15 | Brightness no longer flows through transport |

**Total deleted: ~893 lines.** Plus elimination of ~30 lines of duplicate brightness logic.

### What gets created

| File | Purpose | ~Lines |
|---|---|---|
| `src/buses/IOutputPipeline.h` | New seam interface | ~25 |
| `src/buses/LightOutputPipeline.h` | Wraps `ILightDriver` into `IOutputPipeline` | ~50 |
| `src/buses/ProtocolTransportPipeline.h` | Wraps `IProtocol`+`ITransport`, applies brightness on-the-fly | ~100 |
| `src/buses/Bus.h` | Single unified bus class | ~60 |

**Total created: ~235 lines.** Net reduction: ~660 lines.

### Updated public aliases in `LumaWave.h`

```cpp
// Before (post-005)                          // After
Light<TDriver>                                Bus + LightOutputPipeline (factory function)
Strip<TProtocol, TTransport>                  Bus + ProtocolTransportPipeline (factory function)
ReferenceLight                                Bus (direct construction)
ReferenceStrip                                Bus (direct construction)
CompositeStrip<TBuses...>                     AggregateBus (already non-templated)
Driver::PlatformDefault                       Bus factory default
```

The `Light<>` and `Strip<>` aliases are replaced by factory functions (see Phase 6) or direct `Bus` construction with the appropriate pipeline. `ReferenceLight` and `ReferenceStrip` aliases currently have dead `TColor` default parameters that are silently ignored — these get cleaned up as part of the alias removal.

## Non-Goals

- Do not change `ILightDriver`, `IProtocol`, or `ITransport` interfaces (except removing `TransportBrightness` from `ITransmit::transmitBytes`).
- Do not change concrete driver, protocol, or transport implementations.
- Do not change the `AggregateBus` / `ReferenceAggregateBus` delegation model (both are already non-templated post-005).
- Do not change `IPixelBus` interface (already non-templated post-005).
- Do not change the `PixelView` abstraction (already non-templated post-005; pending its own elimination per backlog 004).
- Do not change color math, palette, or factory infrastructure.
- Do not add compatibility shims for removed bus types.

## Dependencies

| Backlog | Status | Relationship |
|---|---|---|
| 005 — Eliminate color templates | **Done** | All types are non-templated. Two cosmetic dead `TColor` defaults remain on aliases (cleaned up in Phase 6). |
| 004 — Eliminate PixelView | Pending | Optional: simplifies `Bus` pixel access but not a hard blocker |
| 003 — Eliminate shader concept | Done | Already removed shader from bus pipeline, clearing the way |
| 002 — Color simplification | Done | Color is fixed 4-channel RGBW |

## Task List

### Phase 1 — Define `IOutputPipeline` seam

- [ ] **`P1a`** — Create `src/buses/IOutputPipeline.h`. Define the pure virtual interface with `begin()`, `isReadyToUpdate()`, `write(span<const Color>, BrightnessType)`, and `alwaysUpdate()`.
- [ ] **`P1b`** — Add `IOutputPipeline.h` to `src/buses/Busses.h` umbrella include.
- [ ] **`P1c`** — Verify the interface compiles in isolation (no dependencies on deleted types).

### Phase 2 — Create `LightOutputPipeline`

- [ ] **`P2a`** — Create `src/buses/LightOutputPipeline.h`. Wraps a `std::unique_ptr<ILightDriver>`. Constructor takes `std::unique_ptr<ILightDriver>`.
- [ ] **`P2b`** — Implement `write(span<const Color> colors, BrightnessType brightness)`: assert single pixel, delegate to `_driver->write(colors[0], brightness)`.
- [ ] **`P2c`** — Implement `begin()` → `_driver->begin()`, `isReadyToUpdate()` → `_driver->isReadyToUpdate()`, `alwaysUpdate()` → `false`.
- [ ] **`P2d`** — Provide a templated convenience factory: `template<typename TDriver, typename... Args> std::unique_ptr<LightOutputPipeline> makeLightPipeline(Args&&...)` that constructs the driver in-place.

### Phase 3 — Create `ProtocolTransportPipeline`

- [ ] **`P3a`** — Create `src/buses/ProtocolTransportPipeline.h`. Wraps `std::unique_ptr<IProtocol>` + `std::unique_ptr<ITransport>`. Constructor takes both + pixel count.
- [ ] **`P3b`** — Implement `write(span<const Color> colors, BrightnessType brightness)`:
  - **On-the-fly brightness**: iterate colors, apply `applyBrightness()` per-channel, feed directly into protocol encoding in a single pass. No separate full-frame scratch buffer.
  - Allocate protocol byte buffer from `requiredBufferSizeBytes()`.
  - Call `_protocol->update(dimmedColors, byteBuffer)`.
  - Call `_transport->beginTransaction()`, `_transport->transmitBytes(byteBuffer)`, `_transport->endTransaction()`.
- [ ] **`P3c`** — Implement `begin()` → `_transport->begin()` + `_protocol->begin()`, `isReadyToUpdate()` → `_transport->isReadyToUpdate()`, `alwaysUpdate()` → `_protocol->alwaysUpdate()`.
- [ ] **`P3d`** — Remove `TransportBrightness` from `ITransport::transmitBytes()` signature. The two-arg overload is deleted; only `transmitBytes(span<uint8_t>)` remains. Update all transport implementations.
- [ ] **`P3e`** — Provide a templated convenience factory: `template<typename TProtocol, typename TTransport, typename... Args> std::unique_ptr<ProtocolTransportPipeline> makeStripPipeline(...)`.

### Phase 4 — Create unified `Bus` class

- [ ] **`P4a`** — Create `src/buses/Bus.h`. Non-templated class implementing `IPixelBus`.
- [ ] **`P4b`** — Constructor takes `span<Color> pixelStorage` + `std::unique_ptr<IOutputPipeline> pipeline`. Stores the span (bus does **not** own the pixel buffer — caller provides it).
- [ ] **`P4c`** — Member variables: `span<Color> _pixels`, `std::unique_ptr<IOutputPipeline> _pipeline`, `PixelView _pixelView`, `BrightnessType _brightness{max}`, `bool _dirty{true}`.
- [ ] **`P4d`** — `show()`: check dirty + `alwaysUpdate()` + `isReadyToUpdate()`, then `_pipeline->write(_pixels, _brightness)`. That's it. No brightness logic, no scratch buffer, no protocol/transport bytes.
- [ ] **`P4e`** — `begin()` → `_pipeline->begin()`, `isReadyToUpdate()` → `_pipeline->isReadyToUpdate()`, `setBrightness()`/`brightness()` trivial getter/setter.
- [ ] **`P4f`** — `pixels()` returns `_pixelView` (constructed from `_pixels`). Marks dirty on non-const access.
- [ ] **`P4g`** — Add accessors: `IOutputPipeline* pipeline()`, `const IOutputPipeline* pipeline() const`.

### Phase 5 — Delete old bus types

- [ ] **`P5a`** — Delete `src/buses/PixelBus.h`.
- [ ] **`P5b`** — Delete `src/buses/LightBus.h`.
- [ ] **`P5c`** — Delete `src/buses/ReferenceBus.h`.
- [ ] **`P5d`** — Delete `src/buses/ReferenceLightBus.h`.
- [ ] **`P5e`** — Delete `src/buses/CompositeBus.h`.
- [ ] **`P5f`** — Update `src/buses/Busses.h`: remove deleted includes, add new ones (`IOutputPipeline.h`, `LightOutputPipeline.h`, `ProtocolTransportPipeline.h`, `Bus.h`). Keep `AggregateBus.h`.

### Phase 6 — Update public surface (`LumaWave.h`)

- [ ] **`P6a`** — Remove `Strip<TProtocol, TTransport>` alias.
- [ ] **`P6b`** — Remove `Light<TDriver>` alias (post-005, only `TDriver` remains).
- [ ] **`P6c`** — Remove `ReferenceLight` alias (post-005, non-templated but alias still has a dead `TColor` default — both the alias and the dead param go).
- [ ] **`P6d`** — Remove `CompositeStrip<TBuses...>` alias.
- [ ] **`P6e`** — Remove `ReferenceAggregateStrip` alias (already non-templated post-005).
- [ ] **`P6f`** — Export `lw::busses::Bus` into the global namespace (when `LW_USE_EXPLICIT_NAMESPACES` is not set).
- [ ] **`P6g`** — Add convenience factory functions in `namespace lw`:
  - `template<typename TDriver, typename... Args> Bus makeLight(span<Color> pixel, Args&&... driverArgs)` — creates a `Bus` with `LightOutputPipeline`.
  - `template<typename TProtocol, typename TTransport> Bus makeStrip(span<Color> pixels, ProtocolSettings, TransportSettings)` — creates a `Bus` with `ProtocolTransportPipeline`.
- [ ] **`P6h`** — Remove `namespace Driver { ... }` block (platform default driver aliases — move to factory function defaults or transport header).
- [ ] **`P6i`** — Remove the `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` guard and its documentation — no combinatorial template types remain.

### Phase 7 — Update tests

- [ ] **`P7a`** — Rewrite `test/busses/test_light_bus/test_main.cpp` → test `Bus` with `LightOutputPipeline`. Same test coverage: write+show, dirty guard, readiness check, brightness passthrough.
- [ ] **`P7b`** — Rewrite `test/busses/test_reference_light_bus/test_main.cpp` → merge into `P7a` (no separate reference variant needed).
- [ ] **`P7c`** — Rewrite `test/busses/test_static_bus_driver_pixel_bus/test_main.cpp` → test `Bus` with `ProtocolTransportPipeline`. Same coverage: multi-pixel write+show, brightness scaling applied by pipeline, protocol encoding, transport transmission.
- [ ] **`P7d`** — Rewrite `test/busses/test_reference_bus/test_main.cpp` → merge into `P7c`.
- [ ] **`P7e`** — Update `test/busses/test_composite_bus/test_main.cpp` → replace `CompositeBus` usage with `AggregateBus` (or delete if `AggregateBus` tests already cover the same ground).
- [ ] **`P7f`** — Update `test/busses/test_aggregate_bus/test_main.cpp` — replace old bus types with `Bus` children (AggregateBus is already non-templated post-005).
- [ ] **`P7g`** — Add focused tests for `ProtocolTransportPipeline`: verify brightness is applied on-the-fly (no full-frame scratch allocation), verify protocol receives brightness-dimmed colors.
- [ ] **`P7h`** — Add focused tests for `LightOutputPipeline`: verify driver receives correct color and brightness.
- [ ] **`P7i`** — Add focused tests for `Bus`: verify `span`-based pixel storage (pixel writes through `pixels()` go to caller's buffer).
- [ ] **`P7j`** — Update `test/CMakeLists.txt` — remove old test targets, add new ones.

### Phase 8 — Update examples

- [ ] **`P8a`** — Update `examples/hello/light/light.ino`: replace `Light<...>` with `makeLight<...>(pixelSpan, driverSettings)` or direct `Bus` construction.
- [ ] **`P8b`** — Update `examples/platform/rp2040/pwm-light/pwm-light.ino`: replace `LightBus<...>` with `makeLight<...>(...)`.
- [ ] **`P8c`** — Update `examples/hello/hello.ino` (strip example): replace `Strip<...>` with `makeStrip<...>(...)`.
- [ ] **`P8d`** — Update `examples/multi-strip/` examples: replace `CompositeStrip<...>` with `AggregateBus`.
- [ ] **`P8e`** — Update `examples/palettes/` and `examples/platform/` examples as needed.

### Phase 9 — Build validation

- [ ] **`P9a`** — `cmake -S . -B build && cmake --build build` — zero errors.
- [ ] **`P9b`** — `ctest --test-dir build --output-on-failure` — all tests pass.
- [ ] **`P9c`** — Verify `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` flag is fully removed (no remaining references).
- [ ] **`P9d`** — Verify no remaining `#include` of deleted bus headers anywhere in the codebase.
- [ ] **`P9e`** — Verify `TransportBrightness` is fully removed (no references in transports, protocols, or tests).

### Phase 10 — Documentation

- [ ] **`P10a`** — Update `docs/usage/compilation-flags.md`: remove `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` documentation.
- [ ] **`P10b`** — Update `docs/internal/information/library-modularization-refactor-design.md`: reflect new bus architecture.
- [ ] **`P10c`** — Update `docs/README.md` and `docs/usage/` guides: new `Bus` API, factory functions, pipeline concepts.
- [ ] **`P10d`** — Update `examples/README.md`: new example patterns.

## Design Decisions

### Why `span<Color>` for pixel storage (bus doesn't own the buffer)?

- The caller decides allocation strategy: stack `std::array<Color, N>`, heap `std::vector<Color>`, static buffer, DMA-safe region.
- No forced heap allocation in `Bus` itself.
- Enables zero-copy scenarios where pixel data lives in a pre-allocated region.
- The bus still owns the `PixelView` wrapping the span — mutation through `pixels()` marks dirty.

### Why on-the-fly brightness instead of a scratch buffer?

- Eliminates the full-frame scratch allocation (`_brightnessScratch` in current `PixelBus`).
- Brightness is applied in the same pass as protocol encoding — no separate pre-processing loop.
- For single-pixel lights, brightness is already handled at the driver level (PWM hardware scaling) — no change needed.
- The `ProtocolTransportPipeline` may use a small fixed-size working buffer (e.g., one row of pixels) but never a full-frame copy.

### Why delete `CompositeBus` instead of keeping it?

- `CompositeBus<TBuses...>` is a variadic template — exactly the kind of combinatorial type this backlog eliminates.
- `AggregateBus` (non-templated, holds `std::vector<std::unique_ptr<IPixelBus>>`) provides the same composition capability without templates.
- The value-semantics advantage of `CompositeBus` (no heap for child buses) is negligible since each child bus already heap-allocates its pipeline.

### Why `std::unique_ptr<IOutputPipeline>` instead of templating `Bus` on the pipeline type?

- Eliminates the last template axis from the bus layer entirely.
- Virtual dispatch at `show()` frequency (tens to thousands of Hz per bus) is acceptable — the project's architecture explicitly permits virtual dispatch at seam boundaries.
- Enables runtime pipeline selection (useful for platform-conditional driver selection without `#ifdef` at the call site).
- Factory functions (`makeLight`, `makeStrip`) preserve ergonomics.

## Open Questions

1. **`IOutputPipeline` location**: Should it live in `src/buses/` (since the bus owns it) or `src/core/` (since it's a seam like `IPixelBus`)? Recommend `src/buses/` since it's the bus's direct delegate, not a cross-cutting seam.

2. **`AggregateBus` ownership model**: Currently `ReferenceAggregateBus` takes raw `IPixelBus*` (non-owning) and `AggregateBus` takes `unique_ptr<IPixelBus>` (owning). Should we unify these? Recommend keeping both — the owning variant for simple cases, the non-owning variant for when buses live elsewhere.

3. **`alwaysUpdate()` on `IOutputPipeline`**: Currently only `IProtocol` has this. For `LightOutputPipeline`, it should always be `false` (single-pixel lights don't need continuous refresh). Confirm this is the right default.

4. **Factory vs direct construction**: Should `Bus` construction always go through factory functions, or should direct construction be supported? Recommend supporting both — factories for the common case, direct construction for advanced use.
