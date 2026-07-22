# Palette Parsing

This document specifies the consumer-facing text formats accepted by `Palette<TPixel>::parse(...)` and `Palette<TPixel>::parseDynamic(...)`.

These formats are part of the public palette API surface.

## Overview

Palette parsing accepts two input forms:

1. Explicit stop form: each stop provides both an index and a color.
2. Color-only form: each stop provides only a color, and indexes are assigned automatically.

Both forms use `|` as the stop separator.

## 1. Explicit Stop Form

Syntax:

```text
index,color|index,color|index,color
```

Examples:

```text
0,FF0000|128,00FF00|255,0000FF
0,#112233|64,#AABBCC|255,#00FF10
0,0x11223344|255,0xAABBCCDD
```

Rules:

- Stop indexes must be in non-decreasing order.
- The first stop must be at index `0`.
- The last stop must be at the configured upper bound (defaults to `255`).
- Duplicate indexes are allowed and create hard zero-width transitions.
- Any stop index above the configured upper bound is rejected. (The upper bound is configurable but defaults to `255`.)

This is the most explicit form and is appropriate when callers need exact stop placement.

## 2. Color-Only Form

Syntax:

```text
color|color|color
```

Examples:

```text
FF0000|00FF00|0000FF
#220000|#FF6600|#FFF2AA
0x11223344|0xAABBCCDD
```

When this form is used, all colors are parsed first and then the stops are evenly re-indexed across the full `0..255` palette domain.

For `N` parsed colors:

```text
index[i] = (i * UPPER_BOUND) / (N - 1)
```
Where `UPPER_BOUND` is the configured palette domain upper bound (defaults to `255`).

Examples:

- `FF0000|00FF00` becomes stops at `0` and `UPPER_BOUND` (default `255`)
- `FF0000|00FF00|0000FF` becomes stops at `0`, `UPPER_BOUND/2`, and `UPPER_BOUND` (default `0`, `127`, `255`)
- `AA0000|BB0000|CC0000|DD0000` becomes stops at `0`, `UPPER_BOUND/3`, `2*UPPER_BOUND/3`, and `UPPER_BOUND` (default `0`, `85`, `170`, `255`)

Rules:

- At least two colors are required for a valid parsed palette.
- The color-only form does not allow mixing explicit indexes with bare colors in the same string.
- Even spacing is applied only after all colors have parsed successfully.

This form is useful when callers only care about the color sequence and want the parser to distribute stops automatically.

## 3. Whitespace

Leading and trailing whitespace is ignored:

```text
 0,112233 | 64,AABBCC | 255,00FF10 
```

Whitespace around separators is allowed in both forms.

## 4. Accepted Color Tokens

Color tokens are parsed from hexadecimal text.

Accepted prefixes:

- no prefix: `112233`
- hash prefix: `#112233`
- `0x` prefix: `0x112233`

Accepted widths depend on the target color type and on supported widening conversions.

Common examples:

- RGB8: `RRGGBB`
- RGBW8: `RRGGBBWW`
- RGBCW8: `RRGGBBCCWW`
- RGB16: `RRRRGGGGBBBB`
- RGBW16: `RRRRGGGGBBBBWWWW`
- RGBCW16: `RRRRGGGGBBBBCCCCWWWW`

The parser also supports widening compatible parsed colors into larger target color types when the channel count and component width are compatible.

Examples:

- RGB8 input can be parsed into RGBW8 or RGBW16 targets
- RGBW8 input can be parsed into RGBCW16 targets

The parser does not downscale channel count or component width.

## 5. Validation Rules

Parsing fails when any of the following are true:

- the input is `nullptr`
- the input is empty or whitespace-only
- a stop separator is malformed
- a color token has an unsupported or invalid hex width
- explicit-stop input is missing the required `0` or upper bound endpoints
- explicit-stop input contains decreasing indexes
- explicit-stop input contains an index above the configured upper bound
- fewer than two total stops are present after parsing
- indexed and non-indexed forms are mixed in one string

On failure:

- `Palette<TPixel>::parse(...)` returns an empty palette
- `Palette<TPixel>::parseDynamic(...)` returns `nullptr`

## 6. Behavior Notes

- Explicit-stop form preserves caller-provided indexes exactly.
- Color-only form always normalizes the stop positions to full-domain even spacing.
- Duplicate explicit indexes are legal and intentionally supported.
- Color parsing is based on hex token width, not on named colors.

## 7. Usage Examples

Explicit stop placement:

```cpp
auto palette = Palette<Color>::parse("0,220000|48,FF6600|255,FFF2AA");
```

Color-only even spacing:

```cpp
auto palette = Palette<Color>::parse("220000|FF6600|FFF2AA");
```

Dynamic parse:

```cpp
auto palette = Palette<Color>::parseDynamic("#220000|#FF6600|#FFF2AA");
```

## 8. Summary

Palette parsing supports both exact stop authoring and compact color-sequence authoring.

- Use `index,color|...` when stop positions matter.
- Use `color|color|...` when evenly spaced stops are sufficient.