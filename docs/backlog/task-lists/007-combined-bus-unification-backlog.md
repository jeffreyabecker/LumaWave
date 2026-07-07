# Unify Bus Interface, Eliminate PixelView, Move Brightness Lower — Combined Backlog

Purpose: merge the 004 (eliminate PixelView) and 006 (unify bus interface) backlogs into a single effort. Replace all six bus types with a single non-templated `Bus` class backed by a new `IOutputPipeline` seam. Eliminate `PixelView` by switching `IPixelBus::pixels()` to return `span<Color>&`. Move brightness application out of the bus layer and into the output pipeline, applied on-the-fly during protocol transformation. Eliminate `TransportBrightness` and `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES`.

**Supersedes**: Backlog 004 (eliminate PixelView) and Backlog 006 (unify bus interface).

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Not started. All dependencies are resolved — backlog 005 is complete (`IPixelBus`, `IProtocol`, `ILightDriver`, `PixelView`, `AggregateBus` are all non-templated). Two dead `TColor` default parameters remain on the `ReferenceLight<>` and `Driver::PlatformDefault<>` aliases in `LumaWave.h` (cosmetic, cleaned up in Phase 7).

## Motivation

### Problem 1: Six bus classes for one job

| Bus Class | Template Params | Pixel Count | Output Mechanism | Ownership |
|---|---|---|---|---|
| `PixelBus<TProtocol, TTransport>` | 2 | N | protocol.encode → transport.transmit | value |
| `ReferenceBus` | 0 | N | protocol.encode → transport.transmit | unique_ptr |
| `LightBus<TDriver>` | 1 | 1 | driver.write(color, brightness) | value |
| `ReferenceLightBus` | 0 | 1 | driver.write(color, brightness) | unique_ptr |
| `CompositeBus<TBuses...>` | variadic | Σ | delegates to children | value |
| `AggregateBus` | 0 | Σ | delegates to children | unique_ptr |

All six implement `IPixelBus`. All six track dirtiness. All six delegate `show()` to something below. The only differences are (a) what sits below the bus and (b) whether it's owned by value or by pointer.

### Problem 2: Brightness at the wrong layer

`PixelBus` and `ReferenceBus` apply brightness at the bus level via an identical per-channel loop over a scratch buffer before protocol encoding. This is duplicated code and an output concern living in the wrong layer. `LightBus` and `ReferenceLightBus` correctly delegate brightness to the driver, but the asymmetry means two different brightness pipelines for no good reason.

### Problem 3: `PixelView` complexity for single-span use

`PixelView` supports flat and chunked storage modes with a binary-search lookup table for O(log n) access across non-contiguous chunks. This complexity exists solely for aggregate/composite buses. Every simple bus has a single contiguous buffer — `PixelView` is dead weight for 4 out of 6 bus types.

### Problem 4: Template combinatorial concerns

`PixelBus<TProtocol, TTransport>` and `CompositeBus<TBuses...>` require the `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` escape hatch. `LightBus<TDriver>` is exempt but still templated. A fully type-erased design eliminates this category of concern entirely.

### Problem 5: `TransportBrightness` is vestigial

Exists on `ITransport::transmitBytes()` but always called with `upstreamApplied=false` and ignored by every real transport. Dead code.

## Target Shape After This Backlog

```
IPixelBus
    │
    ├── Bus  (NEW — single non-templated class, ~50 lines)
    │       │
    │       └── std::unique_ptr<IOutputPipeline>  (NEW seam)
    │               ├── LightOutputPipeline        (wraps ILightDriver)
    │               └── ProtocolTransportPipeline  (wraps IProtocol + ITransport)
    │
    └── AggregateBus  (kept, reworked for external span<Color> buffer)
```

### `IPixelBus` — simplified interface

```cpp
class IPixelBus {
public:
    using BrightnessType = lw::colors::ColorComponent;
    virtual ~IPixelBus() = default;
    virtual void begin() = 0;
    virtual void show() = 0;
    virtual bool isReadyToUpdate() const = 0;
    virtual void setBrightness(BrightnessType brightness) = 0;
    virtual BrightnessType brightness() const = 0;
    virtual span<Color>& pixels() = 0;              // was PixelView&
    virtual const span<Color>& pixels() const = 0;   // was const PixelView&
};
```

`pixels()` returns `span<Color>&` instead of `PixelView&`. All the chunked-mode complexity, lookup tables, and multi-mode dispatch are gone from the interface.

### `IOutputPipeline` — new seam

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

### `Bus` — the only bus class

- Non-templated. Constructor takes `span<Color> pixelStorage` + `std::unique_ptr<IOutputPipeline>`.
- Caller provides the pixel buffer — `Bus` owns **no** storage.
- `show()`: check dirty + `alwaysUpdate()` + `isReadyToUpdate()`, then `_pipeline->write(_pixels, _brightness)`.
- No brightness logic, no scratch buffer, no protocol/transport/driver knowledge.
- ~50 lines total.

### Brightness: on-the-fly during protocol transformation

- `ProtocolTransportPipeline::write()` applies brightness inline during color→bytes encoding — no separate pre-pass, no full-frame scratch buffer.
- `LightOutputPipeline::write()` delegates to `ILightDriver::write(color, brightness)`.
- `TransportBrightness` removed from `ITransport`.

### `AggregateBus` — external buffer management

- Constructor takes `span<Color> externalBuffer` — caller provides a contiguous buffer sized to total child pixel count.
- `pixels()` returns `_pixelSpan` directly (no chunk composition, no lookup table).
- `show()` distributes external buffer contents into each child's span, then calls child `show()`.
- O(n) copy on `show()` — trade-off for eliminating `PixelView` complexity.

### What gets deleted

| Artifact | Lines | Reason |
|---|---|---|
| `src/buses/PixelBus.h` | ~358 | Replaced by `Bus` + `ProtocolTransportPipeline` |
| `src/buses/LightBus.h` | ~120 | Replaced by `Bus` + `LightOutputPipeline` |
| `src/buses/ReferenceBus.h` | ~160 | Replaced by `Bus` (already type-erased) |
| `src/buses/ReferenceLightBus.h` | ~110 | Replaced by `Bus` (already type-erased) |
| `src/buses/CompositeBus.h` | ~130 | Replaced by `AggregateBus` |
| `src/core/PixelView.h` | ~400 | Replaced by `span<Color>` throughout |
| `TransportBrightness` struct | ~15 | Brightness no longer flows through transport |
| `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` | ~5 | No combinatorial template types remain |

**Total deleted: ~1,298 lines.**

### What gets created

| File | Purpose | ~Lines |
|---|---|---|
| `src/buses/IOutputPipeline.h` | New seam interface | ~25 |
| `src/buses/LightOutputPipeline.h` | Wraps `ILightDriver` into `IOutputPipeline` | ~50 |
| `src/buses/ProtocolTransportPipeline.h` | Wraps `IProtocol`+`ITransport`, on-the-fly brightness | ~100 |
| `src/buses/Bus.h` | Single unified bus class | ~60 |

**Total created: ~235 lines. Net reduction: ~1,063 lines.**

### Updated public surface

```cpp
// Before (post-005)                          // After
Light<TDriver>                                makeLight<TDriver>(span, settings) → Bus
Strip<TProtocol, TTransport>                  makeStrip<TProtocol, TTransport>(span, settings) → Bus
ReferenceLight                                Bus(span, make_unique<LightOutputPipeline>(...))
ReferenceStrip                                Bus(span, make_unique<ProtocolTransportPipeline>(...))
CompositeStrip<TBuses...>                     AggregateBus (non-templated)
PixelView                                     (deleted)
Driver::PlatformDefault                       (deleted — absorbed into factory defaults)
LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES       (deleted)
```

## Non-Goals

- Do not change `ILightDriver`, `IProtocol`, or `ITransport` interfaces (except removing `TransportBrightness` from `ITransport::transmitBytes`).
- Do not change concrete driver, protocol, or transport implementations.
- Do not change `IPixelBus` interface beyond `pixels()` return type.
- Do not change color math, palette, or factory infrastructure.
- Do not add compatibility shims for removed bus types or `PixelView`.

## Dependencies

| Backlog | Status | Relationship |
|---|---|---|
| 005 — Eliminate color templates | Done | All types non-templated. Cleaned up in Phase 7 (dead aliases). |
| 003 — Eliminate shader concept | Done | Already removed shader from bus pipeline. |
| 002 — Color simplification | Done | Color is fixed 4-channel RGBW. |

## Task List

### Phase 1 — Define `IOutputPipeline` seam

- [ ] **`P1a`** — Create `src/buses/IOutputPipeline.h`. Pure virtual interface: `begin()`, `isReadyToUpdate()`, `write(span<const Color>, BrightnessType)`, `alwaysUpdate()`.
- [ ] **`P1b`** — Add `IOutputPipeline.h` to `src/buses/Busses.h` umbrella include.
- [ ] **`P1c`** — Verify compiles in isolation (no dependencies on types being deleted later).

### Phase 2 — Change `IPixelBus::pixels()` to `span<Color>&`

- [ ] **`P2a`** — Change `IPixelBus::pixels()` return type from `PixelView&` to `span<Color>&` (mutable), and `span<const Color>&` (const).
- [ ] **`P2b`** — Remove `#include "core/PixelView.h"` from `IPixelBus.h`. Add `#include "core/Compat.h"` for `lw::span` if not present.
- [ ] **`P2c`** — Update all bus implementations' `pixels()` override signatures (these files are deleted in Phase 6, so minimal changes — just enough to keep the build compiling during intermediate phases):
  - `PixelBus.h`: member `PixelView _pixels` → `span<Color> _pixels`; constructor builds span from `_rootPixels`.
  - `LightBus.h`: member `PixelView _pixels` → `span<Color> _pixels`; constructor builds span from `_rootPixel`.
  - `ReferenceBus.h`: member `PixelView _pixels` → `span<Color> _pixels`; constructor builds span from `_rootBuffer`.
  - `ReferenceLightBus.h`: member `PixelView _pixels` → `span<Color> _pixels`; constructor builds span from `_rootBuffer`.
  - `AggregateBus.h`: member `PixelView _pixels` → `span<Color> _pixels`; update `collectAggregateChunks` helper (temporary — fully reworked in Phase 5).
  - `CompositeBus.h`: member `PixelView _pixels` → `span<Color> _pixels` (temporary — deleted in Phase 6).
- [ ] **`P2d`** — Update all test stubs' `pixels()` override signatures to return `span<Color>&`.

### Phase 3 — Create pipeline implementations

- [ ] **`P3a`** — Create `src/buses/LightOutputPipeline.h`. Wraps `std::unique_ptr<ILightDriver>`. `write()` delegates to `_driver->write(colors[0], brightness)`. `begin()`/`isReadyToUpdate()` delegate to driver. `alwaysUpdate()` → `false`.
- [ ] **`P3b`** — Provide templated factory: `template<typename TDriver, typename... Args> std::unique_ptr<LightOutputPipeline> makeLightPipeline(Args&&...)`.
- [ ] **`P3c`** — Create `src/buses/ProtocolTransportPipeline.h`. Wraps `std::unique_ptr<IProtocol>` + `std::unique_ptr<ITransport>`. Constructor takes both + pixel count.
- [ ] **`P3d`** — Implement `ProtocolTransportPipeline::write()`:
  - On-the-fly brightness: iterate colors, apply `applyBrightness()` per-channel inline with protocol encoding in a single pass. No full-frame scratch buffer.
  - Allocate protocol byte buffer from `requiredBufferSizeBytes()`.
  - Call `_protocol->update(dimmedColors, byteBuffer)`.
  - Call `_transport->beginTransaction()`, `_transport->transmitBytes(byteBuffer)`, `_transport->endTransaction()`.
- [ ] **`P3e`** — Implement `begin()` → `_transport->begin()` + `_protocol->begin()`, `isReadyToUpdate()` → `_transport->isReadyToUpdate()`, `alwaysUpdate()` → `_protocol->alwaysUpdate()`.
- [ ] **`P3f`** — Provide templated factory: `template<typename TProtocol, typename TTransport> std::unique_ptr<ProtocolTransportPipeline> makeStripPipeline(PixelCount, ProtocolSettings, TransportSettings)`.

### Phase 4 — Create unified `Bus` class

- [ ] **`P4a`** — Create `src/buses/Bus.h`. Non-templated class implementing `IPixelBus`.
- [ ] **`P4b`** — Constructor: `Bus(span<Color> pixelStorage, std::unique_ptr<IOutputPipeline> pipeline)`. Stores the span — bus does not own the buffer, caller provides it.
- [ ] **`P4c`** — Members: `span<Color> _pixels`, `std::unique_ptr<IOutputPipeline> _pipeline`, `BrightnessType _brightness{max}`, `bool _dirty{true}`.
- [ ] **`P4d`** — `pixels()` returns `_pixels`. Mutable access marks dirty.
- [ ] **`P4e`** — `show()`: check dirty + `alwaysUpdate()` + `isReadyToUpdate()`, then `_pipeline->write(_pixels, _brightness)`. That's it — no brightness logic, no scratch, no protocol/transport bytes.
- [ ] **`P4f`** — `begin()` → `_pipeline->begin()`, `isReadyToUpdate()` → `_pipeline->isReadyToUpdate()`, `setBrightness()`/`brightness()` trivial getter/setter.
- [ ] **`P4g`** — Accessor: `IOutputPipeline* pipeline()`.

### Phase 5 — Rework `AggregateBus` for external buffer management

- [ ] **`P5a`** — Remove `collectAggregateChunks` helper and `_pixelChunks` vector.
- [ ] **`P5b`** — Replace `PixelView _pixels` with `span<Color> _pixelSpan`.
- [ ] **`P5c`** — Add constructor parameter: `span<Color> externalBuffer`. Validates `externalBuffer.size() == totalChildPixelCount`.
- [ ] **`P5d`** — Store `std::vector<span<Color>> _childSpans` populated from each child's `pixels()`.
- [ ] **`P5e`** — Rework `show()`: copy `_pixelSpan` sub-ranges into `_childSpans[i]`, then call `child->show()` for each child. O(n) copy on show.
- [ ] **`P5f`** — Same treatment for `ReferenceAggregateBus` (raw pointer variant).
- [ ] **`P5g`** — Remove `#include "core/PixelView.h"` from aggregate bus headers.

### Phase 6 — Bulk deletion

- [ ] **`P6a`** — Delete `src/buses/PixelBus.h`.
- [ ] **`P6b`** — Delete `src/buses/LightBus.h`.
- [ ] **`P6c`** — Delete `src/buses/ReferenceBus.h`.
- [ ] **`P6d`** — Delete `src/buses/ReferenceLightBus.h`.
- [ ] **`P6e`** — Delete `src/buses/CompositeBus.h`.
- [ ] **`P6f`** — Delete `src/core/PixelView.h`.
- [ ] **`P6g`** — Remove `TransportBrightness` struct and the two-arg `transmitBytes(span<uint8_t>, TransportBrightness)` overload from `ITransport`. Update all transport implementations to remove the two-arg override.
- [ ] **`P6h`** — Update `src/buses/Busses.h`: remove deleted includes, keep `IOutputPipeline.h`, `LightOutputPipeline.h`, `ProtocolTransportPipeline.h`, `Bus.h`, `AggregateBus.h`.
- [ ] **`P6i`** — Remove `#include "core/PixelView.h"` from `src/core/Core.h`.
- [ ] **`P6j`** — Remove `test/core/test_pixel_view/` directory and its test target from `test/CMakeLists.txt`.

### Phase 7 — Update public surface (`LumaWave.h`)

- [ ] **`P7a`** — Remove aliases: `Strip<TProtocol, TTransport>`, `Light<TDriver>`, `ReferenceLight`, `CompositeStrip<TBuses...>`, `ReferenceAggregateStrip`, `PixelView`.
- [ ] **`P7b`** — Export `lw::busses::Bus` into global namespace.
- [ ] **`P7c`** — Add factory functions:
  - `template<typename TDriver, typename... Args> Bus makeLight(span<Color> pixel, Args&&... driverArgs)`
  - `template<typename TProtocol, typename TTransport> Bus makeStrip(span<Color> pixels, PixelCount, ProtocolSettings, TransportSettings)`
- [ ] **`P7d`** — Remove `namespace Driver { ... }` block.
- [ ] **`P7e`** — Remove the `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` conditional compilation guards and all references.
- [ ] **`P7f`** — Remove dead `TColor` default parameters on `ReferenceLight<>` and `Driver::PlatformDefault<>` aliases (these go away with alias removal anyway).

### Phase 8 — Update `fillPixels` / `fillPixelsIndexed`

- [ ] **`P8a`** — Add/update `fillPixels(span<Color>, const Color&)` overload.
- [ ] **`P8b`** — Add/update `fillPixelsIndexed(span<Color>, TGenerator&&)` overload.
- [ ] **`P8c`** — Remove `PixelView`-based overloads.
- [ ] **`P8d`** — Verify palette infrastructure works with `span<Color>` (it accepts `TOutputRange&&`, which `span<Color>` satisfies).

### Phase 9 — Update tests

#### 9a — New tests for `Bus` and pipelines

- [ ] **`P9a1`** — Create `test/busses/test_bus/`: test `Bus` with `LightOutputPipeline` (write+show, dirty guard, readiness, brightness passthrough).
- [ ] **`P9a2`** — Test `Bus` with `ProtocolTransportPipeline` (multi-pixel write+show, brightness scaling applied by pipeline, protocol encoding, transport transmission).
- [ ] **`P9a3`** — Test `Bus` span-based storage: writes through `pixels()` go to caller's buffer (zero-copy).
- [ ] **`P9a4`** — Test `ProtocolTransportPipeline`: brightness applied on-the-fly, protocol receives dimmed colors.
- [ ] **`P9a5`** — Test `LightOutputPipeline`: driver receives correct color and brightness.

#### 9b — Remove old bus tests

- [ ] **`P9b1`** — Delete `test/busses/test_light_bus/`.
- [ ] **`P9b2`** — Delete `test/busses/test_reference_light_bus/`.
- [ ] **`P9b3`** — Delete `test/busses/test_static_bus_driver_pixel_bus/`.
- [ ] **`P9b4`** — Delete `test/busses/test_reference_bus/`.
- [ ] **`P9b5`** — Delete `test/busses/test_composite_bus/`.

#### 9c — Update remaining tests

- [ ] **`P9c1`** — Update `test/busses/test_aggregate_bus/`: allocate external contiguous buffer, pass to constructor, verify distribute-on-show behavior.
- [ ] **`P9c2`** — Update any contract tests referencing deleted types.
- [ ] **`P9c3`** — Update any transport tests that reference `TransportBrightness`.
- [ ] **`P9c4`** — Update `test/CMakeLists.txt`: remove deleted test targets, add new ones.

### Phase 10 — Update examples

- [ ] **`P10a`** — `examples/hello/light/light.ino`: `Light<...>` → `makeLight<...>(span, settings)`.
- [ ] **`P10b`** — `examples/platform/rp2040/pwm-light/pwm-light.ino`: `LightBus<...>` → `makeLight<...>(...)`.
- [ ] **`P10c`** — `examples/hello/hello.ino` (strip): `Strip<...>` → `makeStrip<...>(...)`.
- [ ] **`P10d`** — `examples/multi-strip/`: `CompositeStrip<...>` → `AggregateBus` with external buffer.
- [ ] **`P10e`** — Verify all other examples compile (most use `auto& pixels = strip.pixels()` which deduces `span<Color>&` — no code changes needed).

### Phase 11 — Build validation

- [ ] **`P11a`** — `cmake -S . -B build && cmake --build build` — zero errors.
- [ ] **`P11b`** — `ctest --test-dir build --output-on-failure` — all tests pass.
- [ ] **`P11c`** — Verify no remaining `#include` of deleted headers anywhere.
- [ ] **`P11d`** — Verify no `PixelView` references remain in any header.
- [ ] **`P11e`** — Verify no `TransportBrightness` references remain.
- [ ] **`P11f`** — Verify `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` is fully removed.

### Phase 12 — Documentation

- [ ] **`P12a`** — Update `docs/usage/compilation-flags.md`: remove `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES`.
- [ ] **`P12b`** — Update `docs/internal/information/library-modularization-refactor-design.md`: reflect new bus architecture, remove `PixelView` references.
- [ ] **`P12c`** — Update `docs/README.md` and usage guides: new `Bus` API, factory functions, pipeline concepts.
- [ ] **`P12d`** — Update `examples/README.md`: new patterns.
- [ ] **`P12e`** — Remove any design docs that still frame `PixelView` as a core kernel contract.

## Design Decisions

### Why `span<Color>` for pixel storage (bus doesn't own the buffer)?

- Caller controls allocation: stack `std::array<Color, N>`, heap `std::vector<Color>`, static buffer, DMA-safe region.
- No forced heap allocation in `Bus`.
- Enables zero-copy scenarios where pixel data lives in a pre-allocated region.
- Same design supports simple buses (caller allocates 1 or N pixels) and aggregate buses (caller allocates total child count).

### Why on-the-fly brightness instead of a scratch buffer?

- Eliminates the full-frame scratch allocation (`_brightnessScratch` in current `PixelBus`/`ReferenceBus`).
- Brightness applied in the same pass as protocol encoding — no separate pre-processing loop.
- Single-pixel brightness stays at driver level (PWM hardware scaling) — no change needed.

### Why delete `CompositeBus`?

- Variadic template — combinatorial type this backlog eliminates.
- `AggregateBus` provides the same composition without templates.
- Value-semantics advantage (no heap for children) is negligible — each child bus already heap-allocates its pipeline.

### Why `std::unique_ptr<IOutputPipeline>` instead of templating `Bus`?

- Eliminates the last template axis from the bus layer.
- Virtual dispatch at `show()` frequency is acceptable per project's virtual-first architecture.
- Enables runtime pipeline selection (platform-conditional driver selection without `#ifdef`).
- Factory functions preserve ergonomics.

### Why O(n) copy on `AggregateBus::show()`?

- Trade-off for eliminating `PixelView` chunked-mode complexity.
- The copy is a `memcpy`-equivalent of N `Color` values — for typical strips (hundreds of pixels), negligible.
- If profiling shows it's hot, an optimization path (direct zero-copy when children use the same buffer layout) can be added later.

### Why combine 004 and 006?

- 004 converts every bus type's `PixelView` member to `span<Color>` — 006 then deletes every one of those bus types. Sequential execution wastes ~200 lines of conversion work.
- 004's `ExternalBus` (span, no-op show) is 006's `Bus` (span, delegated show) minus the pipeline. No reason for both to exist.
- Combined execution: ~11 phases vs ~17 separate, net ~1,063 lines deleted vs net ~660 if done sequentially.

## Open Questions

1. **`IOutputPipeline` location**: `src/buses/` (bus's delegate) or `src/core/` (cross-cutting seam)? Recommend `src/buses/`.

2. **`AggregateBus` ownership**: `ReferenceAggregateBus` (raw `IPixelBus*`, non-owning) and `AggregateBus` (`unique_ptr<IPixelBus>`, owning). Keep both — owning for simple cases, non-owning when buses live elsewhere.

3. **`alwaysUpdate()` default**: `false` for `LightOutputPipeline` (single-pixel lights), delegated from protocol for `ProtocolTransportPipeline`.

4. **Factory vs direct construction**: Support both — factories for common case, direct `Bus(span, make_unique<Pipeline>(...))` for advanced use.
