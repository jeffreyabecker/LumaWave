# WLED ↔ LumaWave Compatibility Analysis

> **Purpose:** Identify what LumaWave needs to serve as a replacement for FastLED / NeoPixelBus in WLED-MM's pixel pipeline. LumaWave's goal is to replace the *underlying pixel-driving libraries*, not to reinvent WLED's own application-level features.  
> **Date:** 2026-07-22

---

## 1  Integration Surface: Library Responsibilities vs. WLED Features

A pixel-driving library is responsible for three things: **pixel buffering**, **protocol encoding**, and **hardware transport**. Everything else in WLED's bus layer — `autoWhiteCalc()`, `ColorOrderMap`, `BusManager`, segment mapping, ABL — is WLED's own application code built on top of the library.

This section separates **library-level** touchpoints (where LumaWave must match FastLED/NeoPixelBus) from **WLED integration** touchpoints (where WLED would adapt its own code to sit above LumaWave).

### 1.1  Library-Level Touchpoints (LumaWave Must Cover)

| Touchpoint | What FastLED / NeoPixelBus Do | LumaWave Equivalent | Status |
|-----------|------------------------------|---------------------|--------|
| **Pixel buffer** | Owned pixel array with typed access (`CRGB[]`, `RgbColor[]`) | `bus.pixels()` → `span<Pixel>` — non-owning or owning views | ✅ Compatible |
| **Protocol encoding** | Encode pixel colors to wire-format bytes per chipset | `Protocol::update(span<Pixel>)` → encoded byte buffer | ✅ Compatible |
| **Hardware transport** | Push encoded bytes to physical output (PIO, RMT, SPI, bit-bang) | `Transport::transmitBytes(span<uint8_t>)` | ✅ Compatible (platform transports in progress) |
| **Show / transmit trigger** | `FastLED.show()` / `NeoPixelBus::Show()` | `bus.show()` → pipeline run | ✅ Compatible |
| **Color representation** | `CRGB` / `RgbColor` / `RgbwColor` etc. | LumaWave `Pixel` type (RGB, RGBW, HSV, HSL) | ✅ Compatible |
| **Protocol coverage** | ~15+ chipset protocols (WS2812, APA102, SK6812, etc.) | See §2 — core protocols done, several missing | ⚠️ Partial |
| **Platform transport coverage** | AVR, ESP32 (RMT, I2S), ESP8266, RP2040 (PIO), nRF52 | RP2040 PIO done; ESP32 RMT/SPI in progress | ⚠️ Partial |
| **PWM / analog direct output** | Not provided by either library — WLED's `BusPwm` is custom code | Deleted (was `PwmOutputPipeline`); needs rebuild | ⚠️ Gap |
| **Multi-strip coordination** | `FastLED.addLeds()` with multiple controllers | `PipelineRun[]` — multi-bus composition | ✅ Compatible |

### 1.2  WLED Integration Touchpoints (WLED Adapts, Not LumaWave's Job)

These are WLED's own features built *above* the pixel library. LumaWave does not need to reimplement them — WLED's integration layer maps them onto LumaWave's API. However, some have a natural home in LumaWave's shader pipeline if WLED wants to offload them.

| WLED Feature | Where It Lives Today | Relationship to LumaWave |
|-------------|---------------------|--------------------------|
| **`autoWhiteCalc()`** (RGB→RGBW) | `BusDigital::show()` — pre-encode step | ⚡ Shader opportunity: `AutoWhiteShader` in pipeline |
| **`colorBalanceFromKelvin()`** (CCT) | `BusDigital::show()` — pre-encode step | ⚡ Shader opportunity: `CctWhiteBalanceShader` |
| **`ColorOrderMap`** (per-pixel byte order) | `BusDigital::show()` — per-pixel lookup | Protocol concern — byte order is inherent to the protocol; per-pixel overrides are a WLED feature |
| **`BusManager` / runtime bus dispatch** | WLED's own bus registry, `BusConfig.type` dispatch | WLED integration concern — WLED wraps LumaWave buses, not the other way |
| **Segment mapping** (group/spacing/reverse/etc.) | `BusManager::show()` → virtual→physical index | WLED integration concern — maps indices before `bus.pixels()[i] = color` |
| **`customMappingTable[]`** | `BusManager::show()` → index remap | WLED integration concern — remap index before write |
| **ABL** (`estimateCurrentAndLimitBri()`) | Reads bus pixel data, sums power | WLED integration concern — reads `bus.pixels()` span for power estimation |
| **`BusConfig` struct** | Runtime type selection, pin assignment | WLED integration concern — WLED's config maps to LumaWave construction |
| **Network output** (Art-Net, DDP, E1.31) | `BusNetwork` — WLED custom code | ⚠️ Neither FastLED nor NeoPixelBus provide this; stretch goal for LumaWave |
| **HUB75 matrix** | `BusHub75` — WLED custom code | ⚠️ Neither FastLED nor NeoPixelBus provide this; stretch goal |

---

## 2  Bus Types: What LumaWave Needs to Cover

WLED supports these bus output types. Green = LumaWave already covers, Red = gap.

### 2.1  Digital (One-Wire / Clockless)

| WLED Type | Protocol | LumaWave Status |
|-----------|----------|-----------------|
| `TYPE_WS2812_RGB` (22) | WS2812/WS2811 | ✅ `Ws2812Protocol` |
| `TYPE_WS2811_400KHZ` (24) | Half-speed WS2812 | ❌ No 400kHz variant |
| `TYPE_GS8608` (23) | GS8608 (WS2812-like, needs refresh) | ❌ Missing |
| `TYPE_TM1829` (25) | TM1829 | ❌ Missing |
| `TYPE_UCS8903` (26) | UCS8903 (RGB) | ❌ Missing |
| `TYPE_UCS8904` (29) | UCS8904 (RGBW) | ❌ Missing |
| `TYPE_SK6812_RGBW` (30) | SK6812 RGBW | ❌ Missing (Ws2812 is RGB only) |
| `TYPE_TM1814` (31) | TM1814 (RGBW) | ✅ `Tm1814Protocol` |
| `TYPE_WS2812_1CH_X3` (19) | 1ch white, 3 zones per IC | ❌ Niche |
| `TYPE_WS2812_2CH_X3` (20) | 2ch CCT, 3 zones per IC | ❌ Niche |
| `TYPE_WS2812_WWA` (21) | Amber + WW + CW white | ❌ Niche |

### 2.2  Digital (Clocked / SPI)

| WLED Type | Protocol | LumaWave Status |
|-----------|----------|-----------------|
| `TYPE_APA102` (51) | APA102/SK9822/DotStar | ✅ `DotStarProtocol` |
| `TYPE_WS2801` (50) | WS2801 | ❌ Missing |
| `TYPE_LPD8806` (52) | LPD8806 | ✅ `Lpd8806Protocol` |
| `TYPE_P9813` (53) | P9813 | ✅ `P9813Protocol` |
| `TYPE_LPD6803` (54) | LPD6803 | ✅ `Lpd6803Protocol` |

### 2.3  PWM / Analog Direct-Drive

| WLED Type | Description | LumaWave Status |
|-----------|-------------|-----------------|
| `TYPE_ANALOG_1CH` (41) | Single PWM channel | ❌ Deleted (was `PwmOutputPipeline`) |
| `TYPE_ANALOG_2CH` (42) | WW + CW PWM | ❌ Deleted |
| `TYPE_ANALOG_3CH` (43) | RGB PWM | ❌ Deleted |
| `TYPE_ANALOG_4CH` (44) | RGBW PWM | ❌ Deleted |
| `TYPE_ANALOG_5CH` (45) | RGB + WW + CW PWM | ❌ Deleted |
| `TYPE_ONOFF` (40) | Binary relay output | ❌ Never existed |

### 2.4  Network / Virtual

| WLED Type | Description | LumaWave Status |
|-----------|-------------|-----------------|
| `TYPE_NET_DDP_RGB` (80) | DDP RGB output | ❌ No network transport |
| `TYPE_NET_DDP_RGBW` (88) | DDP RGBW output | ❌ No network transport |
| `TYPE_NET_ARTNET_RGB` (82) | Art-Net RGB output | ❌ No network transport |
| `TYPE_NET_ARTNET_RGBW` (83) | Art-Net RGBW output | ❌ No network transport |

### 2.5  Matrix

| WLED Type | Description | LumaWave Status |
|-----------|-------------|-----------------|
| `TYPE_HUB75MATRIX` (100) | HUB75 LED matrix panel | ❌ No matrix support |

---

## 3  Gap Analysis

This section separates actual library-level gaps from WLED integration concerns and architectural opportunities.

### 3.1  🔴 Library Gaps: What LumaWave Must Provide to Replace FastLED / NeoPixelBus

#### 3.1.1  Missing Protocols

New `Protocol` subclasses needed to match FastLED/NeoPixelBus coverage:

| Priority | Protocol | Type | Rationale |
|----------|----------|------|-----------|
| **High** | SK6812 RGBW | One-wire RGBW | Very common; WLED's default for RGBW strips |
| **High** | WS2801 | Clocked/SPI | Legacy but widely deployed |
| **Medium** | UCS8903 (RGB) | One-wire | Used in commercial products |
| **Medium** | UCS8904 (RGBW) | One-wire | RGBW variant of UCS8903 |
| **Medium** | TM1829 | One-wire | Used in some addressable strips |
| **Medium** | GS8608 | One-wire | WS2812-like, needs periodic refresh |
| **Low** | 400kHz WS2811 variant | One-wire | Half-speed WS2812; niche |
| **Low** | WS2812 WWA / 1CH / 2CH variants | One-wire | Specialty white/amber variants |

#### 3.1.2  Platform Transports

Transports are the platform-specific hardware output layer (PIO, RMT, SPI DMA, bit-banging). Current status:

| Platform | Transport | Status |
|----------|-----------|--------|
| RP2040 / Pico2W | PIO (PIOasm state machine) | ✅ Complete |
| ESP32 | RMT | ⚠️ In progress |
| ESP32 | SPI DMA | ⚠️ In progress |
| ESP8266 | Bit-bang | ❌ Not started |
| AVR (Arduino Uno/Mega) | Bit-bang | ❌ Not started |
| nRF52 | PWM + PPI | ❌ Not started |

#### 3.1.3  PWM / Analog Direct Output

Neither FastLED nor NeoPixelBus provide PWM direct-drive output — WLED's `BusPwm` is custom code. However, it's a legitimate use case LumaWave should cover for completeness. This is the `OutputPipeline` pattern (deleted, needs rebuild):

- **PWM output pipeline** — per-platform: RP2040 PWM slices, ESP32 LEDC
- **On/Off relay output** — binary GPIO toggle
- **Sigma-delta output** — ESP32 sigma-delta modulation (optional)

### 3.2  ⚡ Shader Opportunities: WLED Features with a Natural Home in LumaWave's Pipeline

These are WLED's own features that currently run in `BusDigital::show()` before the NeoPixelBus buffer write. LumaWave's shader pipeline is a *better* architectural home for the color-transform ones — composable, testable, reusable. But these are **not gaps** in the library sense; WLED could keep them as-is in its integration layer.

| WLED Feature | Shader Equivalent | Benefit of Moving to Shader |
|-------------|-------------------|----------------------------|
| `autoWhiteCalc()` (5 modes: `MANUAL_ONLY`, `BRIGHTER`, `ACCURATE`, `DUAL`, `MAX`) | `AutoWhiteShader` | Composable with other shaders; unit-testable independently; only active when protocol supports W channel |
| `colorBalanceFromKelvin()` (CCT ≥ 1900K) | `CctWhiteBalanceShader` | Kelvin→RGB lookup cached per temperature; clean separation from protocol logic |

> **Note on `ColorOrderMap`:** Color order (RGB vs GRB vs BRG etc.) is a protocol-level concern — it determines byte layout during encoding, not a color-space transform. NeoPixelBus handles this via template features (`NeoGrbFeature`, `NeoRgbFeature`); FastLED bakes it into the chipset definition. WLED's per-pixel `ColorOrderMap` overrides are a WLED-specific extension. In LumaWave, per-bus color order is inherent to each `Protocol` subclass; per-pixel overrides would be a protocol configuration option (e.g., passing a `ColorOrderMap` to `Protocol::update()`), not a shader.

**Architectural note:** WLED's current per-pixel processing runs as a monolithic block in `BusDigital::show()`. LumaWave's shader pipeline lets the color-transform pieces be composed as independent stages:

```
effect writes color
  → CctWhiteBalanceShader    // optional
  → AutoWhiteShader          // optional, only for RGBW protocols
  → Protocol::update()       // encode to wire format (handles color order internally)
  → Transport::transmitBytes()
```

### 3.3  🔷 WLED Integration Concerns: WLED Keeps These

These are WLED's own application-level architecture. LumaWave should not reimplement them — WLED's integration layer maps them onto LumaWave's API.

| Concern | Why WLED Keeps It | LumaWave Integration Pattern |
|---------|-------------------|-----------------------------|
| **`BusManager` + runtime bus dispatch** | WLED's bus registry with `BusConfig.type` dispatch. FastLED and NeoPixelBus are also compile-time; WLED already wraps them. | WLED wraps LumaWave buses the same way it wraps NeoPixelBus today. No change to WLED's `BusManager` pattern. |
| **Segment mapping** (group/spacing/reverse/mirror/offset) | WLED's segment system is its core UI abstraction. Effects write virtual indices; mapping layer converts to physical. | `virtualIndex → physicalIndex → bus.pixels()[physicalIndex] = color`. Mapping happens above LumaWave. |
| **`customMappingTable[]`** | Arbitrary index remapping owned by WLED. | Same pattern: remap index before calling `bus.pixels()[i] = color`. |
| **ABL** (`estimateCurrentAndLimitBri()`) | WLED's power management reads pixel buffer, sums channel values, limits brightness. | `bus.pixels()` provides `span<Pixel>` — WLED reads it directly for power estimation, same as today. |
| **`ColorOrderMap`** (per-pixel byte order overrides) | WLED's per-pixel color order lookup. Basic color order is inherent to each protocol; per-pixel overrides are a WLED-specific extension. | WLED keeps `ColorOrderMap` as-is. LumaWave protocols could optionally accept a `ColorOrderMap` for per-pixel lookup during encoding, but this is a WLED integration concern. |
| **`BusConfig` / JSON config** | WLED's configuration schema. | WLED's config deserialization maps to LumaWave construction parameters. |
| **Network output** (Art-Net, DDP, E1.31) | WLED's `BusNetwork` is custom UDP broadcast code. Neither FastLED nor NeoPixelBus provide this. | WLED keeps `BusNetwork` as-is; optionally, LumaWave could provide a `UdpTransport` as a stretch goal. |
| **HUB75 matrix** | WLED's `BusHub75` with dirty-bit tracking. Neither FastLED nor NeoPixelBus provide this. | WLED keeps `BusHub75` as-is; LumaWave stretch goal only.

---

## 4  Architectural Fit: How WLED Would Integrate LumaWave

Three integration models, ordered by how much of WLED's existing bus layer changes.

### Option A: LumaWave as Drop-In NeoPixelBus Replacement (Minimal Change)

WLED keeps `BusDigital`, `BusPwm`, and `BusManager` as-is. Only the inner library call changes:

```cpp
// Today: WLED's BusDigital wraps NeoPixelBus
BusDigital::show() {
  autoWhiteCalc();               // WLED's own per-pixel processing (kept)
  colorBalanceFromKelvin();      // WLED's own CCT correction (kept)
  ColorOrderMap::apply();        // WLED's own color order (kept)
  polyBus->SetPixelColor(i, c);  // NeoPixelBus buffer write
  polyBus->Show();               // NeoPixelBus RMT/I2S driver
}

// With LumaWave: same structure, different library calls
BusDigital::show() {
  autoWhiteCalc();                              // WLED keeps this
  colorBalanceFromKelvin();                     // WLED keeps this
  ColorOrderMap::apply();                       // WLED keeps this
  lumaWaveBus.pixels()[i] = color;              // LumaWave span write
  lumaWaveBus.show();                           // LumaWave pipeline
}
```

**Pros:** Minimal WLED changes. WLED's per-pixel processing stays put.  
**Cons:** No architectural benefit beyond cleaner protocol/transport code. Shader pipeline unused.

### Option B: WLED Offloads Per-Pixel Processing to LumaWave Shaders

WLED keeps `BusManager` and `BusDigital` but moves `autoWhiteCalc`, `colorBalanceFromKelvin`, and `ColorOrderMap` into LumaWave's shader pipeline:

```cpp
// WLED configures the pipeline with shaders:
auto bus = PixelBus<Ws2812Protocol, PioTransport>(count);
bus.shaders()
  .add<CctWhiteBalanceShader>(kelvin)
  .add<AutoWhiteShader>(AutoWhiteMode::BRIGHTER);

// WLED's BusDigital::show() becomes:
BusDigital::show() {
  // No per-pixel processing here — shaders handle color transforms;
  // ColorOrderMap stays in WLED or becomes a protocol config option
  for (i = 0; i < count; i++)
    lumaWaveBus.pixels()[i] = pixelData[i];
  lumaWaveBus.show();  // shaders run → protocol encodes → transport transmits
}
```

**Pros:** Clean separation. Shaders are composable and unit-testable. WLED's per-pixel logic becomes reusable outside WLED.  
**Cons:** More WLED changes. Requires shader pipeline to be mature.

### Option C: LumaWave Replaces Entire Bus Layer (Maximum Change)

WLED's `BusManager`, `BusDigital`, `BusPwm`, `BusNetwork` are replaced by LumaWave's `Bus`, `PixelBus`, `PipelineRun`. WLED effects write directly to LumaWave spans:

```cpp
// WLED effects:
SEGMENT.setPixelColor(virtualIndex, color)
  → strip.setPixelColor(physicalIndex, color)   // WLED segment mapping stays
    → lumaWaveBus.pixels()[physicalIndex] = color;

// Show:
WS2812FX::show()
  → estimateCurrentAndLimitBri()  // reads lumaWaveBus.pixels() for ABL
  → lumaWaveBus.show()
```

**Pros:** Full LumaWave architecture — protocol/transport separation, shader pipeline, non-owning composition.  
**Cons:** Largest WLED refactor. BusManager is deeply embedded in WLED's config and UI layers.

### Recommendation

**Start with Option A** for proof-of-concept integration. Once LumaWave's shader pipeline is mature, incrementally move to Option B for per-pixel processing. Option C is a long-term architectural goal.

---

## 5  Recommended Implementation Order

Priorities reflect the library-replacement goal: first match FastLED/NeoPixelBus capabilities, then offer architectural improvements WLED can adopt incrementally.

| Phase | Work | Category | Rationale |
|-------|------|----------|-----------|
| **1** | Complete platform transports (ESP32 RMT, SPI DMA, ESP8266) | Library gap | Foundation — nothing works without hardware output |
| **2** | Add missing protocols (SK6812 RGBW, WS2801, UCS8903, UCS8904, TM1829, GS8608) | Library gap | Match FastLED/NeoPixelBus protocol coverage |
| **3** | Rebuild `PwmOutputPipeline` (per-platform PWM) | Library gap | Cover analog/PWM direct-drive use cases |
| **4** | Mature the shader pipeline — ensure hooks are clean and composable | Shader infra | Foundation for optional WLED feature offload |
| **5** | Implement `AutoWhiteShader`, `CctWhiteBalanceShader` | Shader opportunity | WLED can offload `autoWhiteCalc()` + `colorBalanceFromKelvin()` |
| **6** | Integration examples: Option A (minimal) and Option B (shader-offload) | Documentation | Reference for WLED-MM adoption |
| **7** | `NetworkTransport` (UDP Art-Net/DDP) | Stretch | Neither FastLED nor NeoPixelBus provide this |
| **8** | `Hub75OutputPipeline` | Stretch | Neither FastLED nor NeoPixelBus provide this |

---

## 6  Summary

### LumaWave's Library-Level Gaps (Must Fix to Replace FastLED/NeoPixelBus)

1. **Platform transports** — ESP32 RMT/SPI DMA, ESP8266 bit-bang, AVR, nRF52
2. **Missing protocols** — SK6812 RGBW, WS2801, UCS8903, UCS8904, TM1829, GS8608
3. **PWM/analog output** — `PwmOutputPipeline` needs rebuilding (neither FastLED nor NeoPixelBus provide this; it's a legitimate addition)

### What LumaWave Does Not Need to Replace

These are WLED's own application features, built above the pixel library. WLED keeps them:

- `BusManager` / runtime bus type dispatch (WLED already wraps compile-time libraries)
- Segment mapping (virtual→physical index translation)
- `customMappingTable[]` (arbitrary index remapping)
- ABL / power management (`estimateCurrentAndLimitBri()`)
- Network output / HUB75 matrix (not provided by FastLED or NeoPixelBus either)

### Architectural Opportunities (WLED Can Adopt Incrementally)

- **Shader pipeline** — `AutoWhiteShader`, `CctWhiteBalanceShader` can host WLED's color-transform processing more cleanly than the current monolithic `BusDigital::show()`. Color order remains a protocol-level concern (byte layout during encoding), not a shader.
- **Protocol/transport separation** — cleaner than WLED's enum-based bus types
- **Non-owning `Bus` model** — ABL reads `span<Pixel>` directly, no buffer copy
- **Multi-strip composition** via `PipelineRun[]`
