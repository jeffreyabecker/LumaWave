**COMPLETED**

# Backlog: Replace Color Class with Packed Integer Type Alias

## Motivation

Replace the `Color` class (which wraps `std::array<ColorComponent, 4>` with `operator[]`, variadic constructors, comparison operators, and packing/unpacking) with a simple integer type alias. Colors become trivial POD types with no constructor overhead, natural bit-level access, and simpler memory model.

## Target State

```cpp
// Color.h becomes:
using Color = std::conditional_t<(ColorComponentBitDepth == 16), uint64_t, uint32_t>;

// Component masks and shifts (WRGB packed layout: W at top, then R, G, B at bottom):
static constexpr size_t ColorComponentBits = ColorComponentBitDepth;
static constexpr Color componentMask = (ColorComponentBits == 16) ? 0xFFFFull : 0xFFu;
static constexpr size_t shiftR = 2 * ColorComponentBits;
static constexpr size_t shiftG = 1 * ColorComponentBits;
static constexpr size_t shiftB = 0 * ColorComponentBits;
static constexpr size_t shiftW = 3 * ColorComponentBits;

// Component access — templated return type defaults to ColorComponent,
// allows callers to request narrower types (e.g. colorR<uint8_t>(c) for 8-bit strips):
template <typename T = ColorComponent>
inline constexpr T colorR(Color c) { return static_cast<T>((c >> shiftR) & componentMask); }
template <typename T = ColorComponent>
inline constexpr T colorG(Color c) { return static_cast<T>((c >> shiftG) & componentMask); }
template <typename T = ColorComponent>
inline constexpr T colorB(Color c) { return static_cast<T>((c >> shiftB) & componentMask); }
template <typename T = ColorComponent>
inline constexpr T colorW(Color c) { return static_cast<T>((c >> shiftW) & componentMask); }

// Setters accept the stored ColorComponent width:
inline void setColorR(Color& c, ColorComponent v) { c = (c & ~(componentMask << shiftR)) | (static_cast<Color>(v) << shiftR); }
inline void setColorG(Color& c, ColorComponent v) { c = (c & ~(componentMask << shiftG)) | (static_cast<Color>(v) << shiftG); }
inline void setColorB(Color& c, ColorComponent v) { c = (c & ~(componentMask << shiftB)) | (static_cast<Color>(v) << shiftB); }
inline void setColorW(Color& c, ColorComponent v) { c = (c & ~(componentMask << shiftW)) | (static_cast<Color>(v) << shiftW); }

// Construction:
inline constexpr Color colorFromRGB(ColorComponent r, ColorComponent g, ColorComponent b) { return colorFromRGBW(r, g, b, 0); }
inline constexpr Color colorFromRGBW(ColorComponent r, ColorComponent g, ColorComponent b, ColorComponent w)
{
    return (static_cast<Color>(w) << shiftW) | (static_cast<Color>(r) << shiftR)
         | (static_cast<Color>(g) << shiftG) | (static_cast<Color>(b) << shiftB);
}

// Channel-by-tag (replaces color['R'], color[channelTag]) — templated return type:
template <typename T = ColorComponent>
inline constexpr T colorComponentByTag(Color c, char tag)
{
    switch (tag) {
        case 'R': case 'r': return colorR<T>(c);
        case 'G': case 'g': return colorG<T>(c);
        case 'B': case 'b': return colorB<T>(c);
        case 'W': case 'w': return colorW<T>(c);
        default: return 0;
    }
}

inline void setColorComponentByTag(Color& c, char tag, ColorComponent v)
{
    switch (tag) {
        case 'R': case 'r': setColorR(c, v); break;
        case 'G': case 'g': setColorG(c, v); break;
        case 'B': case 'b': setColorB(c, v); break;
        case 'W': case 'w': setColorW(c, v); break;
    }
}
```

---

## Phase 1 — Define the New Color Type

### P1a — Replace Color class with type alias
- [x] Replace `class Color` with `using Color = uint32_t` (or `uint64_t` for 16-bit)
- [x] Define bit-shift constants: `shiftR`, `shiftG`, `shiftB`, `shiftW`
- [x] Define `ColorComponent` accessor functions: `colorR()`, `colorG()`, `colorB()`, `colorW()`
- [x] Define `ColorComponent` setter functions: `setColorR()`, `setColorG()`, `setColorB()`, `setColorW()`
- [x] Define factory functions: `colorFromRGB()`, `colorFromRGBW()`
- [x] Define `colorComponentByTag(char)` for dynamic channel access
- [x] Move `Color::parse()` / `Color::tryParse()` to free functions
- [x] Remove `Color::ComponentType` (use `ColorComponent` directly)
- [x] Remove `Color::MaxComponent` (use `std::numeric_limits<ColorComponent>::max()`)

### P1b — Update Color comparison usage
- [x] Remove `operator==` / `operator!=` / `operator<` etc. (integer comparison works natively)
- [x] Verify all comparison sites still compile

---

## Phase 2 — Update Shaders

### P2a — BrightnessShader
- [x] Replace `dest[i][channel] = applyBrightness(source[i][channel], ...)` with
  `setColorComponentByTag(dest[i], channel, applyBrightness(colorComponentByTag(source[i], channel), ...))`

### P2b — GammaShader
- [x] Replace `d[channel] = gamma8(s[channel])` with
  `setColorComponentByTag(d, channel, gamma8(colorComponentByTag(s, channel)))`

---

## Phase 3 — Update Protocols

### P3a — Simple channel-order protocols (Lpd6803, Lpd8806, Pixie, Ws2801, Ws2812x, Sm16716, Tm1814, Tm1914)
Pattern: `color[_settings.channelOrder[channel]]` or `toWireComponent8(color[...])`
- [x] Replace with `colorComponentByTag<uint8_t>(color, _settings.channelOrder[channel])` when protocol needs 8-bit wire values
- [x] Use `colorComponentByTag(color, ...)` (defaults to `ColorComponent`) for full-width access

### P3b — P9813Protocol (uses literal channel chars)
- [x] Replace `color['R']`, `color['G']`, `color['B']` with `colorR(color)`, `colorG(color)`, `colorB(color)`

### P3c — DotStar protocols (Apa102Protocol, Hd108Protocol)
- [x] Replace `color[effectiveChannelOrder[channel]]` with `colorComponentByTag(color, effectiveChannelOrder[channel])`
- [x] Verify gain encoding still works (uses `_gainValue`, not Color members)

### P3d — Sm168xProtocol, Tlc59711Protocol
- [x] Update channel access patterns

---

## Phase 4 — Update Transport Light Drivers

### P4a — PwmOutputPipeline (and Esp8266LedcLightDriver)
- [x] Replace `color[channelTag]` with `colorComponentByTag(color, channelTag)`
- [x] Replace `Color::ComponentType` with `ColorComponent`

### P4b — PrintOutputPipeline
- [x] Replace `color[channelTag]` with `colorComponentByTag(color, channelTag)`
- [x] Replace `Color::ComponentType` with `ColorComponent`

### P4c — Other light drivers (Esp32LedcLightDriver, Esp32SigmaDeltaLightDriver, RpPwmLightDriver)
- [x] Replace `color[channelTag]` with `colorComponentByTag(color, channelTag)`

---

## Phase 5 — Update ColorMath and Palette Code

### P5a — ColorMath.h
- [x] `applyBrightness(color, brightness)` — already takes Color by value, update internal `color[channel]` access
- [x] `applyBrightness(component, brightness)` — no change needed
- [x] Any other color manipulation functions

### P5b — Palette code (Blends.h, Types.h, Generators.h, Detail.h, WrapModes.h)
- [x] Replace `Color{}` default construction (becomes `Color{0}` or just `0`)
- [x] Replace `Color(r, g, b)` construction with `colorFromRGB(r, g, b)`
- [x] Replace `color[channel]` with `setColorComponentByTag(color, channel, ...)`
- [x] Replace `Color::ComponentType` with `ColorComponent`
- [x] Replace `Color::MaxComponent` with `std::numeric_limits<ColorComponent>::max()`

---

## Phase 6 — Update Tests

### P6a — Protocol tests (test_protocol_spec_sections_*)
- [x] Replace `Color{r, g, b}` → `colorFromRGB(r, g, b)`
- [x] Replace `Color{r, g, b, w}` → `colorFromRGBW(r, g, b, w)`
- [x] Replace `mock.lastColor['R']` → `colorR(mock.lastColor)`
- [x] Replace `mock.lastColor['G']` / `['B']` / `['W']` similarly

### P6b — Bus tests
- [x] Replace `Color{10, 20, 30}` → `colorFromRGB(10, 20, 30)`
- [x] Update channel accessors in assertions

### P6c — Print light driver tests
- [x] Replace `Color{0x11, 0x22, 0x33}` → `colorFromRGB(0x11, 0x22, 0x33)`

### P6d — Palette tests
- [x] Update Color construction patterns

---

## Phase 7 — Update Examples and Docs

### P7a — Examples (hello/ws2812, hello/apa102)
- [x] Replace `Color(...)` construction with factory functions
- [x] Update any channel access in example code

### P7b — Documentation
- [x] Update copilot-instructions.md
- [x] Update object-model-contracts.md
- [x] Update ReadMe.md

---

## Phase 8 — Remove Dead Code and Cleanup

### P8a — Remove Color.h class remnants
- [x] Delete `Channels` array storage
- [x] Delete variadic template constructor
- [x] Delete `operator[]` overloads
- [x] Delete `channelAtIndex()`
- [x] Delete comparison operators
- [x] Delete packing/unpacking `operator=` and conversion operators
- [x] Delete `parse()`/`tryParse()` (moved to free functions)

### P8b — Final verification
- [x] `cmake --build build && ctest --test-dir build --output-on-failure`
- [x] Verify no remaining `color[...]` patterns (except `ColorComponent` which is fine)
- [x] Verify no remaining `Color(...)` constructor calls
