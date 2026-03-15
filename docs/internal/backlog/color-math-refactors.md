# ColorMath Utility Promotion Backlog

> **Status:** Backlog
> **Scope:** Promote reusable internal scalar utility helpers into `ColorMath` / `ColorMathBackend` where they represent general-purpose integer-domain color math rather than feature-local policy.

---

## Goal

Reduce repeated scalar math helpers embedded in feature headers and move broadly reusable operations behind the existing `LW_COLOR_MATH_BACKEND` seam.

This backlog is specifically about **utility promotion**, not about rewriting color-model algorithms or pushing feature-specific policy into `ColorMath`.

---

## Non-Goals

- Do not move Kelvin-specific range policy such as `clampKelvin(...)` into `ColorMath`.
- Do not move the exact Kelvin `logf` / `powf` conversion algorithm into `ColorMath`.
- Do not add compatibility wrappers purely to preserve old names.
- Do not add shipped presets or default animation behaviors as part of this work.

---

## Candidate Refactors

### 1. Promote rounded scalar rescaling

**Primary candidates**

- `KelvinToRgbExactStrategy::scale255ToComponent(...)` in `src/colors/KelvinToRgbStrategies.h`
- `CCTWhiteBalanceShader::scaleByUnit(...)` in `src/colors/CCTWhiteBalanceShader.h`
- offset/span to `0..255` mappings in `src/colors/palette/SamplingTransition.h`
- offset/span to `0..255` mappings in `src/colors/palette/Blends.h`

**Problem**

Several features open-code the same family of integer-domain rescale operations with local rounding rules.

**Target direction**

Add one canonical scalar rescale primitive to `ColorMathBackend`, exposed through `ColorMath`.

Possible API shapes:

- `rescaleRound(value, inMax, outMax)`
- `scaleByUnit(value, unit, unitMax)`

**Expected consumers**

- Kelvin component scaling from `0..255` to `0..MaxComponent`
- CCT brightness and white-channel scaling
- palette transition progress normalization

---

### 2. Promote rounded scalar lerp with arbitrary denominator

**Primary candidates**

- `KelvinToRgbLut64Strategy::interpolate8(...)` in `src/colors/KelvinToRgbStrategies.h`
- `CCTWhiteBalanceShader::lerpKelvin(...)` in `src/colors/CCTWhiteBalanceShader.h`

**Problem**

The LUT interpolation helper is not actually Kelvin-specific. It is a general rounded integer lerp over an arbitrary denominator.

**Target direction**

Add a scalar lerp primitive to `ColorMathBackend`, exposed through `ColorMath`.

Possible API shape:

- `lerpRound(left, right, numerator, denominator)`

**Expected consumers**

- Kelvin LUT segment interpolation
- CCT low/high Kelvin interpolation from component-domain balance

---

### 3. Promote scalar weighted-average helpers

**Primary candidates**

- warm/cool correction blending in `src/colors/AutoWhiteBalanceShader.h`

**Problem**

The codebase has reusable color-level blend helpers but no equivalent scalar weighted-average primitive for integer-domain channel or correction math.

**Target direction**

Add a scalar blend helper for common `0..255` weighting contracts.

Possible API shapes:

- `blendScalar255(left, right, progress)`
- `weightedAverage255(left, right, weightRight)`

**Expected consumers**

- Auto white balance correction mixing
- future per-channel modulation or correction paths

---

### 4. Promote scalar progress normalization helpers

**Primary candidates**

- `mapTransitionProgressToBlend8(...)` in `src/colors/palette/SamplingTransition.h`
- repeated palette offset-to-progress calculations in `src/colors/palette/Blends.h`

**Problem**

Palette code currently repeats domain conversion logic from local sample spans into `0..255` blend progress.

**Target direction**

Add a small scalar helper for mapping a value within a span to 8-bit progress.

Possible API shapes:

- `mapToProgress8(position, span)`
- `mapClampedToProgress8(position, span)`

**Expected consumers**

- palette interpolation
- transition blending
- any future integer-domain timeline math

---

### 5. Add fast integer trig for consuming code

**Related current surface**

- `smoothstep8(...)` in `src/colors/ColorMath.h`
- `cubicEaseInOut8(...)` in `src/colors/ColorMath.h`
- `cosineLike8(...)` in `src/colors/ColorMath.h`

**Problem**

The backend already exposes reusable easing math, but it does not expose actual integer-domain trigonometric helpers that consuming code is likely to need for waveforms, motion, and procedural modulation.

**Target direction**

Add canonical phase-domain integer trig helpers behind `ColorMathBackend`.

Candidate APIs:

- `sin8(phase)`
- `cos8(phase)`
- optional later extension: `sin16(...)`, `cos16(...)`

**Notes**

- `cosineLike8(...)` is an easing approximation, not a true phase-domain cosine function.
- This work is justified by general consumer utility, not by the Kelvin implementation itself.

---

## Keep Local

These should remain feature-local unless a broader use-case appears:

- Kelvin range policy in `src/colors/KelvinToRgbStrategies.h`
- exact Kelvin conversion formulas based on `logf` / `powf`
- any helper whose meaning depends on Kelvin-domain policy rather than generic scalar math

---

## Suggested Implementation Order

1. Add scalar rescale helpers to `ColorMathBackend` and `ColorMath`.
2. Add scalar lerp helpers to `ColorMathBackend` and `ColorMath`.
3. Convert Kelvin and CCT to the new primitives.
4. Convert palette transition/progress mapping to the new primitives.
5. Add scalar weighted-average helpers if remaining call sites still duplicate the pattern.
6. Add integer trig as a separate follow-up once the scalar utility layer is settled.

---

## Completion Criteria

- Reusable scalar math helpers no longer live as one-off implementations in Kelvin/CCT/palette headers when they can be expressed as backend math.
- The promoted APIs are general scalar math primitives, not feature-specific wrappers.
- Existing behavior and rounding contracts remain covered by targeted native tests.
- `ColorMath` remains integer-first and backend-driven for reusable scalar/easing/trig operations.