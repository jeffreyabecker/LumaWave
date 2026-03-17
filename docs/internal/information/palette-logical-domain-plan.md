# Palette Logical Domain Plan

## Goal

Expand the logical domain of palette index parameters without changing the canonical authored stop domain or weakening the current palette contracts.

The target end-state is:

- palette inputs may represent a logical domain larger than 0..255
- wrap and boundary behavior remain mathematically consistent across all sampling modes
- authored palette stops remain canonical 8-bit endpoints at 0 and 255
- sampling continues to use integer math and C++17-compatible implementation techniques
- existing 8-bit palette behavior remains the default contract unless a wider logical domain is explicitly requested

## Non-Goals

- Expanding `PaletteStop::index` into an arbitrary authored domain in the first phase
- Rewriting default palettes or generator stop layouts into a non-8-bit storage format
- Introducing floating-point palette coordinate math where fixed-point math is sufficient
- Adding compatibility overloads purely to preserve accidental behavior from partial or inconsistent domain handling

## Current Model

The current subsystem has two partially-overlapping concepts of domain.

### Canonical stop domain

Authored and generated palette stops are validated and distributed in a fixed 8-bit domain:

- first stop must be at index 0
- last stop must be at index 255
- all stop indexes must be ordered and within 0..255

This is the current storage and authoring contract.

### Logical sample domain

Sampling APIs accept `size_t` palette indexes and `PaletteSampleOptions::scaledSampleCount` can redefine the legal sample span used by wrap normalization.

That means the API surface looks wider than the underlying stop domain, but the implementation does not fully separate those two concepts.

## Current Problems

### 1. The logical domain is implicit rather than first-class

Today the domain of the input index is inferred indirectly from `scaledSampleCount` or from the absence of that option.

This makes the behavior harder to reason about because the code mixes:

- raw input indexes
- wrapped logical indexes
- canonical stop-space indexes

without a dedicated internal type that distinguishes them.

### 2. Circular interpolation still assumes a fixed 256-step wrap period

Interpolated wrap sampling crosses the boundary using a hard-coded canonical palette span of 256 steps.

That is correct for the authored stop domain, but it is not correct as a model of the caller's logical sample domain when `scaledSampleCount` is larger or smaller than 256.

### 3. Nearest and interpolated circular behavior are not fully aligned

Nearest circular distance uses the normalized domain size derived from the sampling options, while interpolated wrapped-span blending still uses the fixed 256-step authored span.

As a result, circular nearest sampling and circular interpolated sampling can disagree about the geometry of the same logical domain.

### 4. The public surface implies more domain generality than the implementation really provides

The pervasive use of `size_t` for palette indexes makes it appear that palette sampling already supports a broad logical coordinate model.

In practice, most code paths still collapse directly into assumptions tied to the 0..255 authored stop domain.

## Architectural Rule

The correct expansion is not to immediately widen every stored stop index. The correct expansion is:

- keep authored stops in a canonical 8-bit storage domain
- introduce an explicit logical sampling domain
- normalize wrap and boundary behavior in that logical domain
- map the logical coordinate into canonical stop space before stop lookup and blending

This preserves the current authoring model while giving the sampling layer a mathematically coherent input domain.

## Recommended Direction

## Phase 1: Make domain separation explicit in the design

### Objective

Represent logical sample coordinates and canonical stop-space coordinates as separate concepts in the implementation.

### Changes

1. Define an internal logical-domain descriptor instead of relying on `scaledSampleCount` alone.
2. Define an internal normalized-sample result that clearly separates:
   - logical wrapped position
   - out-of-range state
   - boundary-sampling policy
   - canonical stop-space coordinate
3. Stop passing raw `size_t` indexes directly into stop lookup and interpolation logic after wrap normalization.

### Expected outcome

- The code no longer conflates logical input space with canonical stop space.
- The meaning of palette index parameters becomes explicit and reviewable.

## Phase 2: Introduce canonical fixed-point sampling coordinates

### Objective

Map wide logical domains into canonical 8-bit stop space without losing interpolation fidelity.

### Changes

1. Introduce an internal fixed-point coordinate for canonical stop space.
2. Keep the integer part aligned with the 0..255 authored stop domain.
3. Use fractional bits so a larger logical domain can map into canonical stop space without collapsing every sample to an integer stop index too early.
4. Keep the implementation integer-only.

### Recommendation

Use a fixed-point representation rather than floating point.

The exact bit layout can be chosen during implementation, but it should satisfy these rules:

- enough fractional precision to preserve smooth interpolation when mapping large logical domains into 0..255 canonical space
- inexpensive arithmetic on embedded targets
- no dependency on C++20 features

### Expected outcome

- Large logical domains can be sampled smoothly.
- Interpolation quality is no longer limited to integer collapse in the logical-to-canonical mapping step.

## Phase 3: Normalize all wrap modes in the logical domain

### Objective

Make wrap behavior consistent before any canonical stop-space lookup occurs.

### Changes

1. Perform clamp, circular, mirror, hold-first, hold-last, and blackout decisions in logical space.
2. Treat the caller's logical domain size as the source of truth for circular and mirror geometry.
3. Convert the normalized logical position into canonical fixed-point stop space only after logical wrap behavior has been resolved.

### Expected outcome

- All wrap modes behave consistently regardless of blend mode.
- Circular nearest and circular interpolated sampling use the same logical geometry.

## Phase 4: Retarget stop lookup and blending to canonical coordinates

### Objective

Make nearest and interpolated sampling consume the same canonical coordinate model.

### Changes

1. Update nearest sampling to operate on canonical mapped positions rather than raw caller indexes.
2. Update interpolated sampling to use the canonical fixed-point coordinate and derive blend progress from that coordinate.
3. Replace the current wrapped-span interpolation path so circular interpolation no longer assumes the caller's logical domain is always 256 samples wide.
4. Preserve the existing 8-bit blend contract for color interpolation.

### Expected outcome

- Nearest and interpolated paths are domain-consistent.
- Existing blend semantics remain intact while domain handling becomes correct.

## Phase 5: Clarify and simplify the public surface

### Objective

Make the public API describe the sampling model clearly.

### Changes

1. Re-evaluate whether `scaledSampleCount` should remain as-is, be renamed, or be replaced by a clearer domain descriptor.
2. Keep current overload behavior working for the default 0..255 case.
3. Avoid adding convenience overloads until the internal domain model is stable.
4. Consider whether a later phase should generalize `IndexRange` beyond the current `size_t`-only iterator model.

### Expected outcome

- Public semantics become easier to understand.
- The default 8-bit usage remains straightforward.

## Implementation Notes

### Draft Coordinate Types

The initial implementation should introduce explicit internal coordinate types with names close to the following:

- `PaletteLogicalDomain`
   - owns logical sample-count metadata
   - answers logical max-index queries
   - is derived from sampling options and defaults to the canonical 256-sample logical span when not overridden

- `PaletteCanonicalCoordinate`
   - stores a canonical stop-space position in fixed-point form
   - keeps integer stop-space alignment with the existing 0..255 authored stop contract
   - exposes integer-index and fractional-position accessors for interpolation and nearest-distance logic

- `NormalizedPaletteSample`
   - owns the normalized logical index after wrap processing
   - owns out-of-range and boundary-sampling flags
   - owns the mapped canonical coordinate used for stop lookup and blending

The intended data flow is:

1. derive `PaletteLogicalDomain` from options
2. normalize the raw caller index into a `NormalizedPaletteSample`
3. map that logical position into `PaletteCanonicalCoordinate`
4. perform nearest or interpolated stop sampling using the canonical coordinate

### Why not widen `PaletteStop::index` first

Changing the authored stop domain first would force churn through:

- palette validation
- parsing
- default palette data
- generator stop distribution
- existing section 7 tests

That is a large change surface and it is not required to solve the core problem of logical sample-domain expansion.

The storage domain should remain canonical until the sampling domain refactor is complete and validated.

### Relationship to `scaledSampleCount`

`scaledSampleCount` is the current indicator that logical-domain size and canonical stop-space size are not the same thing.

However, it is only a partial mechanism today. The implementation should treat it as a transitional expression of logical-domain intent rather than as the final abstraction.

### Integer math requirement

All logical-to-canonical mapping should use integer or fixed-point math.

This is aligned with the repository preference for integer math by default and avoids introducing unnecessary floating-point cost into hot sampling paths.

## Testing Plan

## Phase 1 tests

Add focused native tests that expose the current inconsistency and lock in the intended new behavior.

### Required coverage

1. Circular interpolation with a logical domain larger than 256.
2. Circular nearest sampling and circular interpolated sampling agreeing on the same logical geometry.
3. Mirror behavior for logical domains smaller and larger than 256.
4. Hold-first, hold-last, clamp, and blackout behavior when the logical domain is wider than the canonical authored domain.
5. Default behavior with no explicit logical-domain override remaining unchanged.

## Phase 2 tests

Add precision-oriented tests that validate logical-to-canonical mapping.

### Required coverage

1. Monotonic mapping from logical domain into canonical coordinate space.
2. Endpoint preservation.
3. Stable midpoint behavior for representative wide logical domains.
4. No discontinuity at circular wrap boundaries.

## Verification Gates

For contract-sensitive implementation work, run these existing gates:

- `pio test -e native-test`
- `pio test -e native-test --filter shaders/test_palette_*`
- `pio test -e native-test --filter contracts/test_palette_first_pass_compile`

If the refactor is split into smaller passes, run targeted palette section 7 suites first before the full native suite.

## Suggested Execution Order

The recommended order is:

1. add tests that expose the current domain inconsistency
2. introduce explicit logical-domain and canonical-coordinate internal types
3. normalize wrap behavior in logical space
4. retarget nearest and interpolated sampling to canonical coordinates
5. simplify or rename the public logical-domain option once the implementation is stable

## Open Questions

These questions should be resolved before implementation starts:

1. Should the long-term public API expose logical-domain size as a simple count, or as a richer domain descriptor?
2. Is signed palette indexing a real requirement, or is wider unsigned logical indexing sufficient for the planned use cases?
3. Does the final design need to support authored stop domains other than 0..255, or is canonical 8-bit stop storage the permanent model?
4. Should the currently unused scaled-sampler utility be integrated into the new design or removed during the refactor?

## Recommendation Summary

The recommended strategy is:

- preserve the current 8-bit authored stop contract
- make logical sample domain an explicit internal concept
- map logical positions into canonical stop space using fixed-point integer math
- unify nearest and interpolated sampling around that canonical coordinate model
- defer any stop-storage widening until there is a proven need beyond logical input-domain expansion