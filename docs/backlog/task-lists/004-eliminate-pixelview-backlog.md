# Eliminate PixelView Backlog

Purpose: Replace `PixelView<TColor>` with `span<TColor>` as the buffer-access mechanism in the bus interface and simple bus implementations. `PixelView` is retained only as an internal implementation detail for aggregate/composite bus composition. The result is a simpler core contract, less memory overhead in simple buses, and a clearer separation between contiguous (simple) and composed (aggregate) buses.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Not started.

## Motivation

The `PixelView<TColor>` class supports both single-span (flat) and multi-chunk (non-contiguous) storage modes with a binary-search lookup table for O(log n) element access across chunks. This complexity exists solely to support aggregate/composite buses that present multiple child pixel buffers as a single virtual strip.

However, every simple bus — `PixelBus`, `ReferenceBus`, `LightBus`, `ReferenceLightBus` — has a **single contiguous** pixel buffer. Each wraps it in a `PixelView` just to satisfy the `IPixelBus` interface:

| Bus | Buffer | PixelView usage | Overhead |
|---|---|---|---|
| `PixelBus<TProtocol>` | `std::vector<ColorType>` | Wraps single `span<TColor>` | `_lookupEntries` vector + `_ownedChunks` vector + mode dispatch |
| `ReferenceBus<TColor>` | `unique_ptr<TColor[]>` | Same | Same |
| `LightBus<TColor>` | `std::array<ColorType, 1>` | Same | Same |
| `ReferenceLightBus<TColor>` | `unique_ptr<TColor>` | Same | Same |

For aggregate/composite buses, the PixelView chunk mechanism is essential — they compose non-contiguous child spans into a unified view. This is the **only** place the chunked mode is actually used.

## Target Shape After This Backlog

- `IPixelBus<TColor>::pixels()` returns `span<TColor>` instead of `PixelView<TColor>&`
- Simple bus implementations store a `span<TColor>` member (not `PixelView<TColor>`)
- `PixelView<TColor>` is removed from the public API surface
- `PixelView` is retained as an **internal-only** type in aggregate/composite bus headers for chunked composition
- `fillPixels` and `fillPixelsIndexed` are reworked to accept `span<TColor>` (or replaced with range-based alternatives)
- Aggregate/composite buses switch from zero-copy chunk collection to a **contiguous staging buffer** strategy:
  - Aggregate/Composite bus allocates its own contiguous buffer of `TColor`
  - `pixels()` returns `span<TColor>` pointing to this buffer
  - `show()` distributes the contiguous buffer contents to each child bus's buffer before calling child `show()`
- `PixelView` header may be deleted entirely or moved to an internal location
- The `PixelView` alias in `LumaWave.h` is removed
- `test_core/test_pixel_view/` directory may be deleted (PixelView is no longer a public API)

## Non-Goals

- Do not change `IProtocol`, `ITransport`, or color types
- Do not change `show()`, `begin()`, `isReadyToUpdate()`, `setBrightness()`, or `brightness()` contracts in `IPixelBus`
- Do not change factory/descriptor system
- Do not add compatibility shims or deprecated aliases for removed `PixelView` API
- Do not change `samplePalette()` or palette infrastructure (these already accept `TOutputRange&&` — `span<TColor>` satisfies the `IsBeginEndRange` contract)
- Do not change the `rootPixels()` / `rootBuffer()` accessor methods on individual bus types

## Task List

### Phase 1 — Interface (`IPixelBus.h`)

- [ ] **`P1a`** — Change `IPixelBus<TColor>::pixels()` return type from `PixelView<TColor>&` to `span<TColor>&` (mutable) and `span<const TColor>&` (const).
  - Requires updating all override signatures in bus implementations and tests.
- [ ] **`P1b`** — Remove `#include "core/PixelView.h"` from `IPixelBus.h` (replace with `#include "core/Compat.h"` for `lw::span` if not already present).
- [ ] **`P1c`** — Remove `#include "core/PixelView.h"` from `Core.h` (or keep if other core code needs it).

### Phase 2 — Simple bus implementations

#### 2a — `PixelBus.h`

- [ ] **`P2a1`** — Replace `PixelView<ColorType> _pixels` member with `span<ColorType> _pixels` (or `_pixelSpan`).
- [ ] **`P2a2`** — Update constructor: `_pixels(span<ColorType>{_rootPixels.data(), _rootPixels.size()})` remains, but `_pixels` is now `span<ColorType>`.
- [ ] **`P2a3`** — Update `pixels()` override to return `span<ColorType>&` (mutable) / `span<const ColorType>&` (const).
  - Return `_pixels` directly as `span<ColorType>&` — since `span<ColorType>` is a value type, the member can be stored and returned by reference.

#### 2b — `ReferenceBus.h`

- [ ] **`P2b1`** — Replace `PixelView<TColor> _pixels` member with `span<TColor> _pixelSpan`.
- [ ] **`P2b2`** — Update constructor: construct `_pixelSpan` from `_rootBuffer`.
- [ ] **`P2b3`** — Update `pixels()` override to return `span<TColor>&` / `span<const TColor>&`.
- [ ] **`P2b4`** — Remove `makePixelChunk()` helper (or simplify to direct span construction).

#### 2c — `LightBus.h`

- [ ] **`P2c1`** — Replace `PixelView<ColorType> _pixels` member with `span<ColorType> _pixelSpan`.
- [ ] **`P2c2`** — Update constructor: `_pixelSpan(span<ColorType>{_rootPixel.data(), _rootPixel.size()})`.
- [ ] **`P2c3`** — Update `pixels()` override.

#### 2d — `ReferenceLightBus.h`

- [ ] **`P2d1`** — Replace `PixelView<TColor> _pixels` member with `span<TColor> _pixelSpan`.
- [ ] **`P2d2`** — Update constructor: `_pixelSpan(makePixelChunk(...))`.
- [ ] **`P2d3`** — Update `pixels()` override.

### Phase 3 — Aggregate/Composite bus rework

#### 3a — `AggregateBus.h` / `ReferenceAggregateBus.h`

The aggregate bus currently collects child `PixelView` chunks into its own `PixelView`. With child buses returning `span<TColor>`, we switch to a contiguous staging buffer strategy.

- [ ] **`P3a1`** — Remove `PixelView`-based `collectAggregateChunks` helper.
- [ ] **`P3a2`** — Add a contiguous `std::vector<TColor> _stagingBuffer` member sized to total child pixel count.
- [ ] **`P3a3`** — Replace `PixelView<TColor> _pixels` with `span<TColor> _pixelSpan` pointing to `_stagingBuffer`.
- [ ] **`P3a4`** — In constructor: iterate child buses, sum their pixel counts, allocate `_stagingBuffer`, set `_pixelSpan`. Optionally capture child span pointers for `show()` distribution.
- [ ] **`P3a5`** — Rework `pixels()` override — returns `_pixelSpan` (simple `span<TColor>&`).
- [ ] **`P3a6`** — Rework `show()`: before delegating to child `show()`, distribute the staging buffer contents back to each child bus's buffer.
  - For each child, copy `_stagingBuffer[offset..offset+childSize]` → `child->pixels()`.
  - Then call `child->show()`.
  - This is O(n) copy on `show()`, which is the trade-off for eliminating `PixelView` complexity.
- [ ] **`P3a7`** — Remove `_pixelChunks` vector (no longer needed).
- [ ] **`P3a8`** — Remove `using ChunkType` typedef (no longer needed).

#### 3b — `CompositeBus.h`

Same strategy as `AggregateBus`, but with tuple-based child storage.

- [ ] **`P3b1`** — Remove `collectChunks()` static method.
- [ ] **`P3b2`** — Remove `_pixelChunks` vector member.
- [ ] **`P3b3`** — Add `std::vector<ColorType> _stagingBuffer` member.
- [ ] **`P3b4`** — Replace `PixelView<ColorType> _pixels` with `span<ColorType> _pixelSpan`.
- [ ] **`P3b5`** — In constructor: sum child pixel counts, allocate `_stagingBuffer`, set `_pixelSpan`. Capture child span references for `show()`.
  - Store `std::array<span<ColorType>, sizeof...(TBuses)> _childSpans` populated via `std::get<Indices>(_buses).pixels()`.
- [ ] **`P3b6`** — Rework `pixels()` override.
- [ ] **`P3b7`** — Rework `show()`: distribute staging buffer → child spans, then call child `show()`.
- [ ] **`P3b8`** — Remove `ChunkType` typedef.

### Phase 4 — Public surface (`LumaWave.h`)

- [ ] **`P4a`** — Remove `template <typename TColor> using PixelView = lw::PixelView<TColor>;` alias from `LumaWave.h`.
- [ ] **`P4b`** — Remove `#include "core/PixelView.h"` from `Core.h` (if `PixelView` is no longer re-exported) or keep if `PixelView` remains an internal export.
- [ ] **`P4c`** — Update any public comments referencing `PixelView`.

### Phase 5 — `fillPixels` / `fillPixelsIndexed` rework

- [ ] **`P5a`** — Add `fillPixels(span<TColor>, const TColor&)` overload (or replace existing `PixelView` overload).
- [ ] **`P5b`** — Add `fillPixelsIndexed(span<TColor>, TGenerator&&)` overload.
- [ ] **`P5c`** — Remove `PixelView`-based overloads (or keep as deprecated internal helpers).
- [ ] **`P5d`** — Update `LumaWave.h` `using` declarations — they already re-export `lw::fillPixels` and `lw::fillPixelsIndexed`; no change needed if signatures remain compatible.

### Phase 6 — `PixelView.h` cleanup

- [ ] **`P6a`** — Decide fate of `PixelView.h`:
  - **Option A**: Delete entirely if no code references it after Phase 5.
  - **Option B**: Move to `src/busses/internal/PixelView.h` if aggregate/composite buses need it internally for chunked operations.
  - **Option C**: Keep in `src/core/` but mark as internal/deprecated (not recommended — adds confusion).
- [ ] **`P6b`** — If retained internally, remove the `PixelView` alias from `Core.h`.

### Phase 7 — Tests

#### 7a — `test_core/test_pixel_view/`

- [ ] **`P7a1`** — If `PixelView` is removed entirely, delete `test/core/test_pixel_view/` directory and remove from `test/CMakeLists.txt`.
- [ ] **`P7a2`** — If retained internally, move these tests to an internal test location.

#### 7b — Bus test stubs

Update all bus test stub implementations that override `pixels()`:

- [ ] **`P7b1`** — `test/busses/test_aggregate_bus/test_main.cpp`: `StubBus::pixels()` → return `span<TestColor>&`.
- [ ] **`P7b2`** — `test/busses/test_composite_bus/test_main.cpp`: `StubBus::pixels()` → return `span<TestColor>&`.
- [ ] **`P7b3`** — `test/busses/test_reference_bus/test_main.cpp`: if any stub overrides `pixels()`.
- [ ] **`P7b4`** — `test/busses/test_reference_light_bus/test_main.cpp`: if any stub overrides `pixels()`.
- [ ] **`P7b5`** — `test/contracts/test_disable_template_combinatorial_types_compile/test_main.cpp`: update stub.

#### 7c — Bus behavior tests

- [ ] **`P7c1`** — `test/busses/test_aggregate_bus/test_main.cpp`: update aggregate bus tests to work with span-based API and new staging-buffer `show()` semantics.
  - After writing to `aggregate.pixels()`, `show()` must be called to distribute to children.
  - Verify child pixel buffers are updated after `show()`.
- [ ] **`P7c2`** — `test/busses/test_composite_bus/test_main.cpp`: same as P7c1.
- [ ] **`P7c3`** — All other bus tests: update `auto& pixels = bus.pixels()` → now returns `span<TColor>&` (API-compatible pattern, just different type).
- [ ] **`P7c4`** — Update `test/CMakeLists.txt` to reflect any test directory deletions/renames.

### Phase 8 — Examples

Examples use `auto& pixels = strip.pixels()` and then index into it. Since `span<TColor>` supports `operator[]`, `size()`, and range-based `for`, examples need **no code changes** — the pattern `auto& pixels = strip.pixels()` deduces `span<TColor>&` instead of `PixelView<TColor>&`, and all pixel operations through `operator[]` continue to work.

- [ ] **`P8a`** — Verify all examples compile (build check).
- [ ] **`P8b`** — Update any example that explicitly uses `PixelView` type name.

### Phase 9 — Build and validation

- [ ] **`P9a`** — CMake configure and build all environments.
- [ ] **`P9b`** — Run `ctest --output-on-failure` for native tests.
- [ ] **`P9c`** — Run targeted test: `ctest -R test_aggregate_bus --output-on-failure`.
- [ ] **`P9d`** — Run targeted test: `ctest -R test_composite_bus --output-on-failure`.
- [ ] **`P9e`** — Validate no `PixelView` references remain in public headers.

### Phase 10 — Cleanup

- [ ] **`P10a`** — Remove any remaining `PixelView` includes in files that no longer need them.
- [ ] **`P10b`** — Remove `_ownedChunks`, `_lookupEntries`, `_lookupCache` remnants if `PixelView.h` is deleted.
- [ ] **`P10c`** — Update any internal design docs that reference `PixelView` as a core contract component.
- [ ] **`P10d`** — Remove `PixelView` from `docs/internal/information/library-modularization-refactor-design.md` as a core kernel contract.
