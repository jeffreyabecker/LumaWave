# Eliminate PixelView Backlog

Purpose: Replace `PixelView<TColor>` with `span<TColor>` as the buffer-access mechanism in the bus interface and all bus implementations. `PixelView` is eliminated entirely — aggregate/composite buses receive an externally-managed contiguous buffer via `span<TColor>` instead of internally composing child chunks. A new `ExternalBus<TColor>` type is introduced that externalizes color-buffer management entirely: it wraps a caller-provided `span<TColor>`, owns no storage, and delegates buffer lifetime to the caller. Consumers of aggregate/composite buses use `ExternalBus` (or manage buffers directly) and are responsible for allocating the buffer and managing child ordering. The result is a simpler core contract, less memory overhead, and complete removal of `PixelView` from the codebase.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

**Superseded.** Merged into [Backlog 007](007-combined-bus-unification-backlog.md) — combined bus unification, PixelView elimination, and brightness relocation.

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
- `PixelView<TColor>` is **completely eliminated** — no internal retention, no header moved, just deleted
- New `ExternalBus<TColor>` type is introduced: an `IPixelBus` implementation that wraps a caller-provided `span<TColor>` and owns no storage
  - Constructor accepts `span<TColor>`; caller manages buffer allocation and lifetime
  - `pixels()` returns the span directly — zero overhead, no indirection
  - Serves as the building block for aggregate/composite bus children and any scenario requiring external buffer ownership
  - Header: `src/buses/ExternalBus.h`; re-exported via `LumaWave.h`
- `fillPixels` and `fillPixelsIndexed` are reworked to accept `span<TColor>` (or replaced with range-based alternatives)
- Aggregate/composite buses use an **externally-managed contiguous buffer** strategy:
  - Aggregate/Composite bus does **not** own its pixel buffer; it receives `span<TColor>` from the caller
  - The caller (consumer) is responsible for allocating a contiguous buffer of `TColor` sized to total child pixels
  - The caller manages child ordering by deciding how to populate the buffer before passing it to the aggregate/composite bus
  - `pixels()` returns `span<TColor>` pointing to the externally-provided buffer
  - `show()` distributes the buffer contents to each child bus's buffer before calling child `show()`
- `PixelView` header is **deleted entirely**
- The `PixelView` alias in `LumaWave.h` is removed
- `test_core/test_pixel_view/` directory is **deleted** (PixelView no longer exists)

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

#### 2e — New: `ExternalBus.h` (external buffer ownership)

A new bus type that externalizes color-buffer management entirely. Unlike all existing buses which own their pixel storage, `ExternalBus<TColor>` wraps a caller-provided `span<TColor>` and owns **no** storage. The caller controls allocation, layout, and lifetime. This is the foundational building block for the aggregate/composite external-buffer strategy and any scenario where the consumer wants full control over the pixel buffer.

- [ ] **`P2e1`** — Create `src/buses/ExternalBus.h`.
  - Template: `template <typename TColor> class ExternalBus : public IPixelBus<TColor>`.
  - No protocol/transport dependency — pure buffer wrapper (follows `ReferenceBus` pattern).
  - Members: `span<TColor> _pixelSpan`.
- [ ] **`P2e2`** — Constructor: `explicit ExternalBus(span<TColor> buffer)`.
  - Stores `_pixelSpan = buffer`.
  - Caller is responsible for buffer lifetime and initial contents.
- [ ] **`P2e3`** — Implement `pixels()` override — returns `_pixelSpan` (simple `span<TColor>&`).
- [ ] **`P2e4`** — Implement `size()` override — returns `_pixelSpan.size()`.
- [ ] **`P2e5`** — Implement remaining `IPixelBus` pure virtuals with sensible defaults:
  - `show()`: no-op (no protocol/transport; caller handles output separately).
  - `begin()`: no-op.
  - `isReadyToUpdate()`: returns `true`.
  - `setBrightness(uint8_t)`: stores value, no-op otherwise.
  - `brightness()`: returns stored value.
- [ ] **`P2e6`** — Add `#include "buses/ExternalBus.h"` to `LumaWave.h`.
- [ ] **`P2e7`** — Add `using ExternalBus = lw::ExternalBus<TColor>;` alias (or unqualified re-export) in `LumaWave.h` if project conventions call for it.
- [ ] **`P2e8`** — Add `ExternalBus.h` to `CMakeLists.txt` header list.

### Phase 3 — Aggregate/Composite bus rework

#### Design decision — External buffer management

Aggregate/composite buses will **not** own their pixel buffer. Instead, the consumer (caller) provides an externally-allocated contiguous `span<TColor>` via an additional constructor parameter or setter. This enables complete elimination of `PixelView`: there is no internal chunk composition, no lookup table, no multi-mode dispatch. The consumer is responsible for:

1. Allocating a contiguous buffer of `TColor` sized to the total pixel count across all child buses.
2. Managing child ordering — the consumer decides which regions of the buffer map to which child bus.
3. Ensuring the buffer outlives the aggregate/composite bus instance.

The aggregate/composite bus stores only a `span<TColor>` (pointing to the external buffer) and child span references for `show()` distribution.

#### 3a — `AggregateBus.h` / `ReferenceAggregateBus.h`

The aggregate bus currently collects child `PixelView` chunks into its own `PixelView`. With child buses returning `span<TColor>` and external buffer management, `PixelView` is completely eliminated.

- [ ] **`P3a1`** — Remove `PixelView`-based `collectAggregateChunks` helper.
- [ ] **`P3a2`** — Remove `_pixelChunks` vector (no longer needed).
- [ ] **`P3a3`** — Remove `using ChunkType` typedef (no longer needed).
- [ ] **`P3a4`** — Replace `PixelView<TColor> _pixels` with `span<TColor> _pixelSpan`.
- [ ] **`P3a5`** — Add constructor parameter: `span<TColor> externalBuffer` — the caller provides the contiguous buffer. Store as `_pixelSpan`.
  - Constructor validates that `externalBuffer.size() == totalChildPixelCount` (sum of child `size()`).
  - Store `std::vector<span<TColor>> _childSpans` populated from each child's `pixels()` for use in `show()`.
- [ ] **`P3a6`** — Rework `pixels()` override — returns `_pixelSpan` (simple `span<TColor>&`).
- [ ] **`P3a7`** — Rework `show()`: before delegating to child `show()`, distribute the external buffer contents back to each child bus's buffer.
  - For each child at index `i`, copy `_pixelSpan[offset..offset+childSize]` → `_childSpans[i]`.
  - Then call `child->show()` for each child.
  - This is O(n) copy on `show()`, which is the trade-off for eliminating `PixelView` complexity.
- [ ] **`P3a8`** — Remove `#include "core/PixelView.h"` from aggregate bus headers.

#### 3b — `CompositeBus.h`

Same strategy as `AggregateBus`, but with tuple-based child storage.

- [ ] **`P3b1`** — Remove `collectChunks()` static method.
- [ ] **`P3b2`** — Remove `_pixelChunks` vector member.
- [ ] **`P3b3`** — Remove `ChunkType` typedef.
- [ ] **`P3b4`** — Replace `PixelView<ColorType> _pixels` with `span<ColorType> _pixelSpan`.
- [ ] **`P3b5`** — Add constructor parameter: `span<ColorType> externalBuffer` — the caller provides the contiguous buffer.
  - Constructor validates `externalBuffer.size() == sum of child pixel counts`.
  - Store `std::array<span<ColorType>, sizeof...(TBuses)> _childSpans` populated via `std::get<Indices>(_buses).pixels()`.
- [ ] **`P3b6`** — Rework `pixels()` override — returns `_pixelSpan`.
- [ ] **`P3b7`** — Rework `show()`: distribute external buffer → child spans, then call child `show()` for each child.
- [ ] **`P3b8`** — Remove `#include "core/PixelView.h"` from composite bus headers.

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

With external buffer management in aggregate/composite buses, `PixelView` is no longer needed anywhere.

- [ ] **`P6a`** — Delete `src/core/PixelView.h` entirely.
- [ ] **`P6b`** — Remove `#include "core/PixelView.h"` from `Core.h`.

### Phase 7 — Tests

#### 7a — `test_core/test_pixel_view/`

- [ ] **`P7a1`** — Delete `test/core/test_pixel_view/` directory and remove from `test/CMakeLists.txt`.
  - `PixelView` is completely eliminated; these tests are obsolete.

#### 7b — Bus test stubs

Update all bus test stub implementations that override `pixels()`:

- [ ] **`P7b1`** — `test/busses/test_aggregate_bus/test_main.cpp`: `StubBus::pixels()` → return `span<TestColor>&`.
- [ ] **`P7b2`** — `test/busses/test_composite_bus/test_main.cpp`: `StubBus::pixels()` → return `span<TestColor>&`.
- [ ] **`P7b3`** — `test/busses/test_reference_bus/test_main.cpp`: if any stub overrides `pixels()`.
- [ ] **`P7b4`** — `test/busses/test_reference_light_bus/test_main.cpp`: if any stub overrides `pixels()`.
- [ ] **`P7b5`** — `test/contracts/test_disable_template_combinatorial_types_compile/test_main.cpp`: update stub.

#### 7b2 — `test_busses/test_external_bus/` (new)

- [ ] **`P7b6`** — Create `test/busses/test_external_bus/` directory with `test_main.cpp` and `CMakeLists.txt`.
- [ ] **`P7b7`** — Test: constructor accepts `span<TColor>` and `pixels()` returns the same span.
- [ ] **`P7b8`** — Test: writes through `pixels()` are visible in the original caller buffer (zero-copy).
- [ ] **`P7b9`** — Test: `size()` matches the span extent.
- [ ] **`P7b10`** — Test: `ExternalBus` satisfies `IPixelBus<TColor>` contract (compile-time check).
- [ ] **`P7b11`** — Test: buffer outlives bus — caller controls lifetime; no double-free or dangling reference.
- [ ] **`P7b12`** — Register `test_external_bus` in `test/CMakeLists.txt`.

#### 7c — Bus behavior tests

- [ ] **`P7c1`** — `test/busses/test_aggregate_bus/test_main.cpp`: update aggregate bus tests for external buffer management.
  - Tests must allocate an external contiguous buffer and pass it to the aggregate bus constructor.
  - After writing to `aggregate.pixels()`, `show()` must be called to distribute to children.
  - Verify child pixel buffers are updated after `show()`.
  - Verify that child ordering is determined by the test's buffer layout, not by the aggregate bus.
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
