# PixelView Dual-Mode Access Redesign

## Goal

Restructure `PixelView` so the common contiguous case has true O(1) indexed access while fragmented views move from linear chunk scans toward predictable near-constant practical access time.

## Problem Summary

Current `PixelView` is a span-of-spans abstraction. That works for aggregate and composite buses, but it forces all indexed access through chunk resolution even when the underlying storage is a single contiguous buffer.

Observed issues:

- Single-buffer buses still pay `PixelView` indirection overhead.
- Fragmented indexed access degrades with chunk count.
- Iterator-only optimizations help palette-style sequential writes but do not solve indexed access.
- Cache-only approaches flatten the worst-case curve but still leave the common contiguous case slower than it should be.

## Target Design

Implement `PixelView` as a dual-mode view.

### Mode 1: Flat

Use when the view represents a single contiguous pixel buffer.

Store:

- `TColor*` base pointer
- pixel count

Expected behavior:

- `operator[]` is direct O(1)
- iterator dereference is direct
- `begin()` and `end()` stay lightweight

### Mode 2: Chunked

Use when the view represents multiple discontiguous chunks.

Store:

- chunk spans
- chunk start offsets or equivalent prefix index table
- optional last-hit cache only as a secondary optimization

Expected behavior:

- `operator[]` uses chunk boundaries instead of a full linear scan
- chunk lookup should be O(log chunk_count) with prefix search, or practical near-O(1) if a bounded lookup structure is added later
- sequential iteration should remain efficient

## Scope

In scope:

- `src/core/PixelView.h`
- constructors and internal representation changes needed to support flat and chunked modes
- preserving the current external API shape where practical
- targeted native tests for flat and chunked access behavior

Out of scope for first pass:

- broad API changes to `IPixelBus`
- public compatibility shims for old internals
- benchmark infrastructure checked into the repo unless explicitly requested

## Expected Construction Paths

Common contiguous producers should use flat mode:

- `PixelBus`
- `LightBus`
- `ReferenceBus`
- `ReferenceLightBus`

Fragmented producers should use chunked mode:

- `AggregateBus`
- `ReferenceAggregateBus`
- `CompositeBus`
- sliced or concatenated `PixelView` instances with multiple chunks

## Work Items

1. Add an explicit flat representation to `PixelView`.
2. Add a chunked representation with chunk-start metadata.
3. Route single-chunk construction through flat mode instead of generic chunked mode.
4. Keep `chunks()` behavior usable for existing chunk-oriented callers.
5. Update iterators so flat mode stays as lightweight as possible.
6. Make chunked indexed lookup use prefix metadata instead of scanning every chunk from the front.
7. Re-evaluate whether a small last-hit cache still provides value after prefix metadata is in place.
8. Add focused tests for:
   - contiguous flat indexed access
   - contiguous flat iteration
   - multi-chunk indexed access across boundaries
   - slices and concatenation preserving expected behavior
9. Run focused native validation.
10. Re-run the existing microbenchmark or equivalent one-off measurement to compare:
   - flat `PixelView` vs `span`
   - chunked `PixelView[index]`
   - chunked sequential iteration

## Acceptance Criteria

- Single-buffer `PixelView` indexed access is materially closer to `span` than the current chunked implementation.
- Fragmented indexed access no longer scales linearly with chunk count.
- Existing `PixelView` tests pass.
- No public API break is introduced unless intentionally chosen and updated at call sites.
- Aggregate and composite buses still expose correct write-through behavior.

## Validation Plan

Minimum validation:

- `pio test -e native-test --filter core/test_pixel_view`

Recommended follow-up validation:

- targeted bus tests covering aggregate/composite behavior
- one-off native microbenchmark against `PixelView[index]`, range-for iteration, and chunk iteration

## Risks

- Dual-mode internals can make lifetime rules less obvious if construction paths are not explicit.
- `chunks()` compatibility may require a small synthetic single-chunk view for flat mode.
- Iterator category promises should match actual behavior; avoid claiming stronger semantics than the implementation can support efficiently.
- Slices and concatenation must not silently regress into owning unnecessary metadata or invalid spans.

## Open Questions

- Should flat mode be selected only for direct contiguous constructors, or also automatically for a single chunk passed through the chunked constructor?
- Should chunked lookup use binary search over prefix starts immediately, or start with a compact coarse lookup table if that benchmarks better?
- Should `IPixelBus` eventually expose a separate contiguous fast path, or should `PixelView` remain the only public surface?

## Done When

The common single-buffer case behaves like a real contiguous view, fragmented access avoids front-to-back chunk scans, and the resulting implementation remains simple enough to maintain within the current virtual-first architecture.