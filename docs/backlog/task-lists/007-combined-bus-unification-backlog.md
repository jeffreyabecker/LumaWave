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

**✅ COMPLETE.** All 13 phases done. `Bus(span<Color>, {{pipeline, length}})` is the canonical API. `IPixelBus::pixels()` returns `span<Color>&`. All light drivers implement `IOutputPipeline` directly. `PixelView`, `ILightDriver`, `TransportBrightness`, `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES`, and 6 old bus types eliminated. ~1,400+ lines net reduction.

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
    └── Bus  (NEW — single non-templated class, ~60 lines)
            │
            └── std::vector<PipelineRun> _runs
                    │
                    └── each: unique_ptr<IOutputPipeline> pipeline + size_t length
                            ├── ConcreteDriver (light, length=1)
                            └── ProtocolTransportPipeline (strip, length=N)
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

### `IOutputPipeline` — new seam (lives in `src/core/`)

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

Located in `src/core/` (not `src/buses/`) to avoid layering violations — transports must be able to implement it without depending on buses.

Concrete light drivers (`RpPwmLightDriver`, `Esp32SigmaDeltaLightDriver`, `AnalogPwmLightDriver`, etc.) implement `IOutputPipeline` directly. They ignore all but `colors[0]`. `ILightDriver`, `LightOutputPipeline`, `LightDriverSettingsBase`, and the `LightDriverLike` traits are all eliminated — one seam instead of three.

### `Bus` — the only bus class

```cpp
struct PipelineRun {
    std::unique_ptr<IOutputPipeline> pipeline;
    size_t length;
};

class Bus : public IPixelBus {
public:
    Bus(span<Color> pixelStorage, std::initializer_list<PipelineRun> runs);
    // ...
};
```

- Non-templated. Constructor takes `span<Color> pixelStorage` + `std::initializer_list<PipelineRun>`.
- Caller provides the pixel buffer — `Bus` owns **no** storage.
- One class for single lights, LED strips, and multi-strip setups — no separate aggregate type.
- `show()` iterates over `_runs`, passing each pipeline its sub-view span at the correct offset. Zero-copy — sub-views point directly into the caller's buffer.
- No brightness logic, no scratch buffer, no protocol/transport/driver knowledge.
- ~60 lines total.

### Usage examples

```cpp
// Single light
Color pixel;
Bus light({&pixel, 1}, {{make_unique<RpPwmLightDriver>(pins), 1}});

// LED strip
vector<Color> strip(64);
Bus leds(span{strip}, {{make_unique<ProtocolTransportPipeline>(64, ws2812, spi), 64}});

// Multi-strip — one buffer, two pipelines, zero-copy
Color buf[100];
Bus multi(span{buf}, {
    {make_unique<ProtocolTransportPipeline>(50, ws2812, spi), 50},
    {make_unique<ProtocolTransportPipeline>(50, apa102, spi), 50}
});
multi.pixels()[0] = red;   // buf[0], first pipeline sub-view
multi.pixels()[50] = blue; // buf[50], second pipeline sub-view
multi.show();  // pipeline[0]->write({buf, 50}) → pipeline[1]->write({buf+50, 50})
```

### Brightness: on-the-fly during protocol transformation

- `ProtocolTransportPipeline::write()` applies brightness inline during color→bytes encoding — no separate pre-pass, no full-frame scratch buffer.
- Concrete light drivers apply brightness at PWM hardware level (same as today's `driver.write(color, brightness)`, but now via `IOutputPipeline::write(colors, brightness)` using `colors[0]`).
- `TransportBrightness` removed from `ITransport`.

### What gets deleted

| Artifact | Lines | Reason |
|---|---|---|
| `src/buses/PixelBus.h` | ~358 | Replaced by `Bus` + `ProtocolTransportPipeline` |
| `src/buses/LightBus.h` | ~120 | Replaced by `Bus` + `LightOutputPipeline` |
| `src/buses/ReferenceBus.h` | ~160 | Replaced by `Bus` (already type-erased) |
| `src/buses/ReferenceLightBus.h` | ~110 | Replaced by `Bus` (already type-erased) |
| `src/buses/CompositeBus.h` | ~130 | Replaced by multi-run `Bus` |
| `src/buses/AggregateBus.h` (entire file) | ~180 | Deleted — multi-run `Bus` with sub-view spans replaces both `ReferenceAggregateBus` and `AggregateBus` |
| `src/core/PixelView.h` | ~400 | Replaced by `span<Color>` throughout |
| `src/transports/ILightDriver.h` | ~50 | Replaced by `IOutputPipeline` — drivers implement it directly |
| `LightDriverSettingsBase` + `LightDriverLike` traits | ~40 | Obsolete — driver settings remain per-driver, no base class needed |
| `TransportBrightness` struct | ~15 | Brightness no longer flows through transport |
| `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` | ~5 | No combinatorial template types remain |

**Total deleted: ~1,568 lines.**

### What gets created

| File | Purpose | ~Lines |
|---|---|---|
| `src/core/IOutputPipeline.h` | New seam interface | ~25 |
| `src/buses/ProtocolTransportPipeline.h` | Wraps `IProtocol`+`ITransport`, on-the-fly brightness | ~100 |
| `src/buses/Bus.h` | Single unified bus class with `PipelineRun` multi-run support | ~60 |

**Total created: ~185 lines. Net reduction: ~1,383 lines.**

### Updated public surface

```cpp
// Before (post-005)                          // After
Light<TDriver>                                Bus(span, {{make_unique<ConcreteDriver>(settings), 1}})
Strip<TProtocol, TTransport>                  Bus(span, {{make_unique<ProtocolTransportPipeline>(...), N}})
ReferenceLight                                Bus(span, {{...}})
ReferenceStrip                                Bus(span, {{...}})
CompositeStrip<TBuses...>                     Bus(span, {{pipeline1, N1}, {pipeline2, N2}}) — multi-run
AggregateStrip / ReferenceAggregateStrip       (deleted — consolidated into Bus)
ILightDriver / LightDriverSettingsBase         (deleted — drivers implement IOutputPipeline)
PixelView                                     (deleted)
Driver::PlatformDefault                       (deleted)
LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES       (deleted)
```

## Non-Goals

- Do not change `IProtocol`, or `ITransport` interfaces (already cleaned up in Phase 1).
- Do not change concrete driver, protocol, or transport implementations.
- Do not change `IPixelBus` interface beyond `pixels()` return type.
- Do not change color math or palette infrastructure.
- Do not add compatibility shims for removed bus types or `PixelView`.
- **Factory/descriptor infrastructure**: the `makeBus(...)` composition style (documented in copilot-instructions.md and design docs but not yet implemented as a standalone file) is incompatible with direct `Bus` construction. Remove factory guidance from docs and copilot-instructions. No factory code to adapt — direct `Bus(span, make_unique<Pipeline>(...))` replaces the planned factory API.

## Dependencies

| Backlog | Status | Relationship |
|---|---|---|
| 005 — Eliminate color templates | Done | All types non-templated. Cleaned up in Phase 7 (dead aliases). |
| 003 — Eliminate shader concept | Done | Already removed shader from bus pipeline. |
| 002 — Color simplification | Done | Color is fixed 4-channel RGBW. |

## Task List

### Phase 1 — Bulk deletion (old bus types, PixelView, TransportBrightness) — ✅ DONE

Clean slate. Delete everything being replaced before building the new types. No analysis, no refactoring, no conversion work — just delete.

- [x] **`P1a`** — Delete `src/buses/PixelBus.h`.
- [x] **`P1b`** — Delete `src/buses/LightBus.h`.
- [x] **`P1c`** — Delete `src/buses/ReferenceBus.h`.
- [x] **`P1d`** — Delete `src/buses/ReferenceLightBus.h`.
- [x] **`P1e`** — Delete `src/buses/CompositeBus.h`.
- [x] **`P1f`** — Delete `src/buses/AggregateBus.h` (owning variant). `ReferenceAggregateBus` is reworked in Phase 6.
- [x] **`P1g`** — Delete `src/core/PixelView.h`.
- [x] **`P1h`** — Remove `TransportBrightness` struct and the two-arg `transmitBytes(span<uint8_t>, TransportBrightness)` overload from `ITransport`. Update all transport implementations to remove the two-arg override.
- [x] **`P1i`** — Remove `#include "core/PixelView.h"` from `src/core/Core.h`.
- [x] **`P1j`** — Remove `test/core/test_pixel_view/` directory and its test target from `test/CMakeLists.txt`.

### Phase 2 — Define `IOutputPipeline` seam — ✅ DONE

- [x] **`P2a`** — Create `src/buses/IOutputPipeline.h`. Pure virtual interface: `begin()`, `isReadyToUpdate()`, `write(span<const Color>, BrightnessType)`, `alwaysUpdate()`.
- [x] **`P2b`** — Verify compiles in isolation (no dependencies on deleted types).

### Phase 3 — Change `IPixelBus::pixels()` to `span<Color>&` — ✅ DONE

Only `ReferenceAggregateBus` (surviving type) and `IPixelBus` itself need updating.

- [x] **`P3a`** — Change `IPixelBus::pixels()` return type from `PixelView&` to `span<Color>&` (mutable), and `span<const Color>&` (const). Removed `#include "core/PixelView.h"`, already had `core/Compat.h`.
- [x] **`P3b`** — Update surviving test stubs: `test/busses/test_aggregate_bus/` and `test/contracts/test_disable_template_combinatorial_types_compile/` stubs that override `pixels()` to return `span<Color>&`.

### Phase 4 — Create pipeline implementations — ✅ DONE

- [x] **`P4a`** — Move `IOutputPipeline.h` from `src/buses/` to `src/core/`.
- [x] **`P4b`** — Delete `src/transports/ILightDriver.h` (including `LightDriverSettingsBase`, `LightDriverLike`, `SettingsConstructibleLightDriverLike` traits).
- [x] **`P4c`** — Update all 6 concrete light drivers to implement `IOutputPipeline` directly instead of `ILightDriver`:
  - `RpPwmLightDriver`, `Esp32SigmaDeltaLightDriver`, `Esp32LedcLightDriver`, `AnalogPwmLightDriver`, `PrintLightDriverT`, `NilLightDriver`
  - Change base class from `ILightDriver` to `IOutputPipeline`
  - `write(span<const Color> colors, BrightnessType brightness)` — read `colors[0]`, apply brightness as before
  - `alwaysUpdate()` → `false`
  - Single-arg `write(Color)` removed (callers always pass brightness now)
  - Remove `LightDriverSettingsBase` inheritance from settings structs
- [x] **`P4d`** — Update `PlatformDefaultLightDriver` alias (in `Transports.h`) to reference `IOutputPipeline`-based types.
- [x] **`P4e`** — Create `src/buses/ProtocolTransportPipeline.h`. Wraps `std::unique_ptr<IProtocol>` + `std::unique_ptr<ITransport>`. Constructor takes both.
- [x] **`P4f`** — Implement `ProtocolTransportPipeline::write()`:
  - On-the-fly brightness: iterate colors, apply `applyBrightness()` per-channel inline with protocol encoding in a single pass. No full-frame scratch buffer.
  - Allocate protocol byte buffer from `requiredBufferSizeBytes()`.
  - Call `_protocol->update(dimmedColors, byteBuffer)`.
  - Call `_transport->beginTransaction()`, `_transport->transmitBytes(byteBuffer)`, `_transport->endTransaction()`.
- [x] **`P4g`** — Implement `begin()` → `_transport->begin()` + `_protocol->begin()`, `isReadyToUpdate()` → `_transport->isReadyToUpdate()`, `alwaysUpdate()` → `_protocol->alwaysUpdate()`.

### Phase 5 — Create unified `Bus` class — ✅ DONE

- [x] **`P5a`** — Create `src/buses/Bus.h`. Non-templated class implementing `IPixelBus`.
- [x] **`P5b`** — Define `PipelineRun` struct: `std::unique_ptr<IOutputPipeline> pipeline` + `size_t length`.
- [x] **`P5c`** — Constructor: `Bus(span<Color> pixelStorage, std::initializer_list<PipelineRun> runs)`. Validates sum of `length` fields equals `pixelStorage.size()`. Stores span + vector of runs.
- [x] **`P5d`** — Members: `span<Color> _pixels`, `std::vector<PipelineRun> _runs`, `BrightnessType _brightness{max}`, `bool _dirty{true}`.
- [x] **`P5e`** — `pixels()` returns `_pixels`. Mutable access marks dirty.
- [x] **`P5f`** — `show()`: check dirty, iterate `_runs`, pass each pipeline its sub-view `span{_pixels.data() + offset, run.length}`. Zero-copy — sub-view spans point directly into caller buffer.
- [x] **`P5g`** — `begin()` → each `run.pipeline->begin()`, `isReadyToUpdate()` → all pipelines ready, `setBrightness()`/`brightness()` trivial getter/setter.
- [x] **`P5h`** — Accessor: `const std::vector<PipelineRun>& runs()`.

### Phase 6 — Delete `AggregateBus.h` entirely — ✅ DONE

- [x] **`P6a`** — Delete `src/buses/AggregateBus.h` (both `ReferenceAggregateBus` and the owning variant). Multi-run `Bus` with sub-view spans replaces all aggregate/composite functionality.

### Phase 7 — Update `Busses.h` umbrella include — ✅ DONE

- [x] **`P7a`** — Update `src/buses/Busses.h`: remove deleted includes, add `ProtocolTransportPipeline.h`, `Bus.h`.

### Phase 8 — Update public surface (`LumaWave.h`) — ✅ DONE

- [x] **`P8a`** — Remove aliases: `Strip<TProtocol, TTransport>`, `Light<TDriver>`, `ReferenceLight`, `CompositeStrip<TBuses...>`, `ReferenceAggregateStrip`, `AggregateStrip`, `PixelView`.
- [x] **`P8b`** — Export `lw::busses::Bus` into global namespace.
- [x] **`P8c`** — Remove `namespace Driver { ... }` block.
- [x] **`P8d`** — Remove the `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` conditional compilation guards and all references.
- [x] **`P8e`** — Remove dead `TColor` default parameters on `ReferenceLight<>` and `Driver::PlatformDefault<>` aliases (these go away with alias removal anyway).

### Phase 9 — Update `fillPixels` / `fillPixelsIndexed` — ✅ DONE

- [x] **`P9a`** — Add/update `fillPixels(span<Color>, const Color&)` overload.
- [x] **`P9b`** — Add/update `fillPixelsIndexed(span<Color>, TGenerator&&)` overload.
- [x] **`P9c`** — Remove `PixelView`-based overloads.
- [x] **`P9d`** — Verify palette infrastructure works with `span<Color>` (it accepts `TOutputRange&&`, which `span<Color>` satisfies).

### Phase 10 — Update tests

#### 10a — New tests for `Bus` and pipelines

- [x] **`P10a1`** — Create `test/busses/test_bus/`: test `Bus` with a mock `IOutputPipeline` (light path: write+show, dirty guard, readiness, brightness passthrough).
- [x] **`P10a2`** — Test `Bus` with `ProtocolTransportPipeline` (multi-pixel write+show, brightness scaling applied by pipeline, protocol encoding, transport transmission).
- [x] **`P10a3`** — Test `Bus` span-based storage: writes through `pixels()` go to caller's buffer (zero-copy).
- [x] **`P10a4`** — Test `ProtocolTransportPipeline`: brightness applied on-the-fly, protocol receives dimmed colors.
- [x] **`P10a5`** — Test concrete light driver as `IOutputPipeline`: driver receives `colors[0]` and brightness correctly.

#### 10b — Remove old bus tests

- [x] **`P10b1`** — Delete `test/busses/test_light_bus/`.
- [x] **`P10b2`** — Delete `test/busses/test_reference_light_bus/`.
- [x] **`P10b3`** — Delete `test/busses/test_static_bus_driver_pixel_bus/`.
- [x] **`P10b4`** — Delete `test/busses/test_reference_bus/`.
- [x] **`P10b5`** — Delete `test/busses/test_composite_bus/`.
- [x] **`P10b6`** — Delete `test/busses/test_aggregate_bus/` (aggregate functionality replaced by multi-run `Bus`).

#### 10c — Update remaining tests

- [x] **`P10c1`** — Update any contract tests referencing deleted types.
- [x] **`P10c2`** — Update any transport tests that reference `TransportBrightness`.
- [x] **`P10c3`** — Update `test/CMakeLists.txt`: remove deleted test targets, add new ones.

### Phase 11 — Update examples — ✅ DONE

- [x] **`P11a`** — `examples/hello/light/light.ino`: `Light<...>` → `Bus(span, {{make_unique<Driver>(settings), 1}})`.
- [x] **`P11b`** — `examples/platform/rp2040/pwm-light/pwm-light.ino`: `LightBus<...>` → direct `Bus` construction with single `PipelineRun`.
- [x] **`P11c`** — `examples/hello/hello.ino` (strip): `Strip<...>` → `Bus(span, {{make_unique<ProtocolTransportPipeline>(...), N}})`.
- [x] **`P11d`** — `examples/multi-strip/`: `CompositeStrip<...>` → multi-run `Bus` with sub-view spans.
- [x] **`P11e`** — Verify all other examples compile (most use `auto& pixels = strip.pixels()` which deduces `span<Color>&` — no code changes needed).

### Phase 12 — Build validation — ✅ DONE

- [x] **`P12a`** — `cmake -S . -B build && cmake --build build` — zero errors.
- [x] **`P12b`** — `ctest --test-dir build --output-on-failure` — all tests pass.
- [x] **`P12c`** — Verify no remaining `#include` of deleted headers anywhere.
- [x] **`P12d`** — Verify no `PixelView` references remain in any header.
- [x] **`P12e`** — Verify no `TransportBrightness` references remain.
- [x] **`P12f`** — Verify `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` is fully removed.

### Phase 13 — Documentation — ✅ DONE

- [x] **`P13a`** — Update `docs/usage/compilation-flags.md`: remove `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES`.
- [x] **`P13b`** — Update `docs/internal/information/library-modularization-refactor-design.md`: reflect new bus architecture, remove `PixelView` references.
- [x] **`P13c`** — Update `docs/README.md` and usage guides: new `Bus` API, pipeline concepts (no factory functions — direct construction only).
- [x] **`P13d`** — Update `examples/README.md`: new patterns.
- [x] **`P13e`** — Remove any design docs that still frame `PixelView` as a core kernel contract.
- [x] **`P13f`** — Update `.github/copilot-instructions.md`: remove factory/descriptor guidance (`makeBus`, `MakeBus.h`, factory authoring rules). Replace with direct `Bus` construction guidance.

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
- Multi-run `Bus` with `PipelineRun` + sub-view spans provides the same composition without templates.

### Why eliminate `ReferenceAggregateBus`?

- Multi-run `Bus` handles 1-to-N pipelines in a single class. No need for a separate aggregate type.
- Sub-view spans (`span{_pixels.data() + offset, run.length}`) give zero-copy fan-out — no distribute loop.
- The caller can still create separate `Bus` instances with sub-view spans for independent control; multi-run `Bus` is for when a single flat write surface is desired.

### Why `std::unique_ptr<IOutputPipeline>` instead of templating `Bus`?

- Eliminates the last template axis from the bus layer.
- Virtual dispatch at `show()` frequency is acceptable per project's virtual-first architecture.
- Enables runtime pipeline selection (platform-conditional driver selection without `#ifdef`).
- Direct construction with `initializer_list<PipelineRun>` is explicit and clear.

### Why combine 004 and 006?

- 004 converts every bus type's `PixelView` member to `span<Color>` — 006 then deletes every one of those bus types. Sequential execution wastes ~200 lines of conversion work.
- 004's `ExternalBus` (span, no-op show) is 006's `Bus` (span, delegated show) minus the pipeline. No reason for both to exist.
- Combined execution: ~13 phases, net ~1,383 lines deleted.

## Resolved Design Decisions

1. **`IOutputPipeline` location**: `src/core/` — cross-cutting seam at the same level as `IPixelBus`. Transports implement it directly without depending on buses. `ILightDriver` and `LightOutputPipeline` eliminated.

2. **Consolidated bus architecture**: `Bus` is the only `IPixelBus` implementation. Constructor takes `initializer_list<PipelineRun>` where each run is `{unique_ptr<IOutputPipeline>, length}`. Handles single lights, strips, and multi-strip setups in one class. `ReferenceAggregateBus` and `AggregateBus` deleted.

3. **`alwaysUpdate()` default**: `false` for `LightOutputPipeline` (single-pixel lights don't need continuous refresh), delegated from `_protocol->alwaysUpdate()` for `ProtocolTransportPipeline`.

4. **Construction**: Direct construction only — `Bus(span, make_unique<LightOutputPipeline>(...))` or `Bus(span, make_unique<ProtocolTransportPipeline>(...))`. No factory functions.
