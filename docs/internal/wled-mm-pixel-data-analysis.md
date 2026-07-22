# WLED-MM Pixel Data Handling — Analysis

> **Source:** `C:\ode\WLED-MM` (MoonModules fork, build 2607011)  
> **Date:** 2026-07-22  
> **Purpose:** Understand WLED-MM's pixel data model to inform LumaWave transport rebuild.

---

## 1  Color Representation

### RGBW32 Packed Format

WLED-MM uses its own packed `uint32_t` color format — **not** FastLED's native `CRGB` layout:

```
Bits:  31..24   23..16   15..8    7..0
       White    Red      Green    Blue
```

```cpp
#define RGBW32(r,g,b,w) (uint32_t((byte(w) << 24) | (byte(r) << 16) | (byte(g) << 8) | (byte(b))))
#define R(c) (byte((c) >> 16))
#define G(c) (byte((c) >> 8))
#define B(c) (byte(c))
#define W(c) (byte((c) >> 24))
```

### CRGB (FastLED)

Segment `ledsrgb` buffers are `CRGB*` (3 bytes per pixel — r, g, b). The white channel is **lost** when effects store into `ledsrgb`; it only survives the `SEGMENT.setPixelColor()` → `strip.setPixelColor()` → `busses.setPixelColor()` hot path where the full `uint32_t` is preserved.

### Auto-White Calculation

`Bus::autoWhiteCalc(uint32_t)` computes the W channel from RGB for RGBW strips:
- `BRIGHTER`: w = min(r,g,b), keeps RGB unchanged
- `ACCURATE`: w = min(r,g,b), subtracts w from RGB
- `MAX`: w = max(r,g,b), for white-only LEDs
- `DUAL`: manual W unless w==0, then auto
- `MANUAL_ONLY`: no auto calculation

### CCT Color Balance

When CCT ≥ 1900K, `colorBalanceFromKelvin(kelvin, rgb)` applies a per-channel Kelvin→RGB correction. Cached per-Kelvin to avoid recomputation.

---

## 2  Pixel Storage Architecture

### Global LED Buffer

A single contiguous `CRGB* _globalLeds` array spans the total physical LED count (`_length * sizeof(CRGB)`). The `_length` is the sum of all bus lengths.

### Segment Buffers

Each `Segment` has `ledsrgb` (`CRGB*`) which points **into** `_globalLeds` (shared, not copied) at offset `start + startY * maxWidth`:

```
_globalLeds: [seg0 range...][seg1 range...][...][segN range...]
              ↑                ↑                    ↑
          seg0.ledsrgb    seg1.ledsrgb        segN.ledsrgb
```

This means effects write directly into the shared global buffer. There is no per-segment copy phase — segment boundaries are just offsets into one array.

### Segment Bounds vs Physical Bounds

| Concept | Definition |
|---------|-----------|
| **Segment virtual length** | `(stop - start) / grouping * spacing`, with mirror/double/reverse |
| **Segment physical range** | `start` to `stop` in global LED index space |
| **Bus physical range** | `_start` to `_start + _len` in global LED index space |

When `autoSegments = true`, one segment is created per bus with `segment.start = bus.getStart()` and `segment.stop = bus.getStart() + bus.getLength()` — a 1:1 mapping.

When `autoSegments = false`, a single segment spans the entire `_length`, and `BusManager` routes to the correct bus.

### Segment Data

`Segment::data` (`byte*`) is an optional per-segment runtime scratchpad. Capped by `MAX_SEGMENT_DATA`:
- 32KB on non-PSRAM ESP32
- 64KB on PSRAM boards
- 5KB on ESP8266

---

## 3  Effect Rendering Pipeline

### Main Loop: `WS2812FX::service()`

```
service():
  for each Segment:
    if inactive/off: skip
    set palette, CCT on buses
    call effect function: (*_mode[seg.mode])()   → returns frametime
    seg.call++ (frame counter)
    seg.handleTransition()
  
  if doShow:
    show() → estimateCurrentAndLimitBri() → busses.show()
```

### `SEGMENT.setPixelColor(i, col)` — The Hot Path

Effects call this macro to set pixel colors. The 1D path:

```
setPixelColor(virtualIndex, color):
  1. Validate virtualIndex < virtualLength
  2. Store CRGB to ledsrgb[virtualIndex] = CRGB(color)  [white channel lost here]
  3. Compute brightness = currentBri(on ? opacity : 0)
  4. Apply fade: color_fade(color, brightness)
  5. Expand virtualIndex accounting for grouping, reverse, mirror
  6. Map to physical: physicalIndex = virtualIndex + start + offset
  7. For each LED in group (including mirrored): strip.setPixelColor(physicalIndex, color)
```

**Key insight:** The CRGB copy to `ledsrgb` is for effects that read-back pixel state (e.g., fire, plasma). The full 32-bit RGBW color is only preserved through the `strip.setPixelColor()` → bus path.

### `WS2812FX::show()`

1. Runs optional pre-show callback
2. `estimateCurrentAndLimitBri()` — Auto Brightness Limiter: reads back all pixel data, sums estimated power draw, downscales brightness to stay within budget
3. Acquires `busDrawMux` semaphore
4. `busses.show()` — triggers all bus output
5. Updates FPS counters

---

## 4  Bus Layer: Pixel → Hardware

### Bus Types

| Type | Class | Output Method |
|------|-------|---------------|
| Digital (clockless) | `BusDigital` | NeoPixelBus RMT/I2S via `PolyBus` |
| Digital (clocked/SPI) | `BusDigital` | NeoPixelBus SPI via `PolyBus` |
| PWM (analog) | `BusPwm` | `ledcWrite()` (ESP32) or `analogWrite()` (ESP8266) |
| On/Off (relay) | `BusOnOff` | `digitalWrite()` |
| Network (virtual) | `BusNetwork` | UDP Art-Net / E1.31 / DDP |
| HUB75 Matrix | `BusHub75Matrix` | Dirty-bit tracked DMA to panel |

### `BusDigital::setPixelColor(pix, c)` — Per-Pixel Processing

Each pixel goes through this pipeline before reaching the NeoPixelBus buffer:

```
pixel → autoWhiteCalc(c)         [auto W channel for RGBW strips]
      → colorBalanceFromKelvin() [CCT correction if ≥ 1900K]
      → reverse logic            [if bus is reversed]
      → skip pixels              [sacrificial/skip offset]
      → ColorOrderMap lookup     [per-pixel color order overrides]
      → WS2811 1CH×3 handling    [special IC mapping]
      → PolyBus::setPixelColor() [writes to NeoPixelBus internal buffer]
```

### Bus Cache Optimization

`BusManager::setPixelColor(pix, c)` caches the last-used bus. If `pix` falls within that bus's range, it skips the O(n) bus search — saves ~20-30% CPU. Disabled in `slowMode` when buses overlap.

### `BusDigital::show()`

```cpp
PolyBus::show(_busPtr, _iType);
```

This triggers NeoPixelBus to generate and transmit the RMT/I2S/SPI waveform via DMA. On ESP32 this is **asynchronous** — returns before all data is sent.

### `BusManager::show()`

```
for each bus:
  while !bus.canShow(): yield()  [max 80ms wait]
  bus.show()
```

### ABL: `estimateCurrentAndLimitBri()`

Before each show, reads back the pixel buffer from all buses, estimates total power draw, and scales brightness down to stay within configured power budget. This is a **read-back** operation — it accesses bus pixel data already set by effects.

---

## 5  Segment-to-Bus Mapping

### Physical Coordinate Space

Segments and buses share a unified physical pixel index space (0 to `_length-1`).

### Mapping Flow

```
Effect writes to virtual segment index
  → segment grouping/reverse/mirror/offset expansion
  → + segment.start → physical pixel index
  → strip.setPixelColor(physical, color)   [applies custom LED map]
  → busses.setPixelColor(physical, color)  [finds owning bus by range]
  → bus.setPixelColor(busRelative, color)  [per-pixel processing]
```

### Auto-Segments (1:1)

When `autoSegments = true`, `makeAutoSegments()` creates one segment per bus. Physical segment `start/stop` align with bus `_start/_start+_len`. Each bus/GPIO gets its own segment.

### Manual Segments (M:N)

A single segment can span multiple buses. Effects write to one virtual range, and `BusManager` routes each physical pixel to the correct bus by range check.

### Overlapping Buses

When bus start ranges overlap, `slowMode` is enabled, disabling the bus cache. Multiple buses receive the same pixel — used for synchronized duplicate outputs.

---

## 6  Key Architectural Observations

### Strengths

1. **Single shared buffer** — No per-segment copy phase. Effects write directly to `_globalLeds`, and buses read from it. Zero-copy for the common case.

2. **Virtual-to-physical indirection** — Segment grouping/spacing/reverse/mirror is a pure mapping layer. Effects only think in virtual indices. The mapping layer handles all geometry.

3. **Bus abstraction** — `BusDigital`, `BusPwm`, `BusNetwork`, `BusHub75Matrix` all implement the same interface. Adding a new output type means implementing `setPixelColor()` + `show()`.

4. **Per-pixel color pipeline** — `autoWhiteCalc()` → `colorBalanceFromKelvin()` → `ColorOrderMap` runs in the bus layer, not in effects. Effects don't care about strip color order or auto-white.

5. **ABL as a read-back** — The power limiter reads back pixel data from the bus layer, computes draw, and adjusts brightness. Clean separation from rendering.

### Weaknesses

1. **White channel loss** — The `CRGB` segment buffers lose the W channel. Effects that read back `ledsrgb` can't use the white channel for state.

2. **CRGB + RGBW32 dual format** — Two color representations in play. FastLED's `CRGB` for effects, WLED's `RGBW32` for the bus layer. Constant conversion overhead.

3. **NeoPixelBus dependency** — All digital output goes through NeoPixelBus (Makuna). The bus wrapper has ~80+ `I_*` type constants mapping to NeoPixelBus template instantiations. Tight coupling.

4. **No protocol abstraction** — There's no `Protocol` layer. Bus types encode both protocol (WS2812, TM1814, etc.) and transport (RMT, I2S, bitbang) as a single enum value. Can't independently swap protocol and transport.

5. **Bus cache hack** — The bus cache optimization in `setPixelColor()` is clever but fragile — depends on effects writing pixels in mostly-sequential order.

6. **Single global buffer** — All segments share one array. No isolation between segments for effects that want their own state.

---

## 7  Contrast with LumaWave

| Aspect | WLED-MM | LumaWave |
|--------|---------|----------|
| Color format | RGBW32 packed uint32 + CRGB dual | Single `lw::Pixel` type |
| Pixel buffer | Single shared `CRGB*` array | Caller-owned `span<Pixel>`, per-bus |
| Protocol | Encoded in bus type enum (NeoPixelBus) | Separate `Protocol` class hierarchy |
| Transport | Encoded in bus type enum | Separate `Transport` class hierarchy |
| Effect → pixel | Direct write to shared segment buffer | Write to bus `.pixels()` span |
| Output trigger | `busses.show()` iterates all buses | `bus.show()` per `IPixelBus` |
| Multi-strip | Bus ranges in single address space | `PipelineRun[]` with per-run `{pipeline, length}` |
| Shader/transform | autoWhiteCalc, CCT, ColorOrderMap in bus | Explicit `IShader` chain via `ShaderProtocol` |
| Power limiting | ABL reads back bus data, scales bri | `CurrentLimiterShader` in shader pipeline |
