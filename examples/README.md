# Examples Index

Examples in this folder use the public include surface from `LumaWave.h`.
When a suitable global alias is not available, examples intentionally use fully qualified `lw::...` names.

## Hello Examples

- `hello/ws2812/ws2812.ino`: Basic one-wire strip animation.
- `hello/apa102/apa102.ino`: Basic SPI/DotStar strip animation.
- `hello/light/light.ino`: Single-light RGBW16 output with platform-default light driver.
- `hello/builder/builder.ino`: Hello World using the BusBuilder API (replaces manual construction).

## Builder Examples

The `BusBuilder` API replaces manual construction of protocol, transport, buffers, shaders, pipeline, and bus with a single chained builder expression. See [docs/usage/bus-builder.md](../docs/usage/bus-builder.md) for full API documentation.

- `hello/builder/builder.ino`: Simplest builder usage — single strip, no shaders.
- `external-pixels/external-pixels.ino`: External (caller-owned) pixel buffer via `setPixelStorage()`.
- `stack-allocation/stack-allocation.ino`: Zero-heap construction via `StackBusStorage` + `buildInto()`.
- `shader-chaining/shader-chaining.ino`: Multiple shaders applied in insertion order via `addShader()`.
- `presets/presets.ino`: Protocol/transport presets with `addStrip()`, inline field overrides.
- `multi-strip/builder-multi/builder-multi.ino`: Two strips sharing one pixel buffer via `setPixelStorage()` sub-spans.

## Strip Composition

- `multi-strip/builder-multi/builder-multi.ino`: Two strips sharing one pixel buffer via `setPixelStorage()` sub-spans.
- `multi-strip/topology-tiled-grid/topology-tiled-grid.ino`: `Topology` mapping for a 2x2 grid of 16x16 tiles using separate BusBuilder instances.

## Palette Examples

Palette stops are expected in non-decreasing index order.
Duplicate indexes are allowed and produce hard zero-width transitions between adjacent stops.
Text parsing for consumer-authored palettes is documented in [docs/usage/palette-parsing.md](../docs/usage/palette-parsing.md).

- `palettes/static-gradient/static-gradient.ino`: Static multi-stop gradient sampled across the strip.
- `palettes/rainbow-generator/rainbow-generator.ino`: Animated rainbow palette generator.
- `palettes/random-smooth-generator/random-smooth-generator.ino`: Smooth random palette evolution.
- `palettes/random-cycle-generator/random-cycle-generator.ino`: Cycling random palette evolution.
- `palettes/blend-two-generators/blend-two-generators.ino`: Blend transition between two live generators.

## Platform Transport Examples

- RP2040:
  - `platform/rp2040/transport-pio/transport-pio.ino`
  - `platform/rp2040/transport-spi/transport-spi.ino`
  - `platform/rp2040/transport-uart/transport-uart.ino`
- ESP32:
  - `platform/esp32/transport-rmt/transport-rmt.ino`
  - `platform/esp32/transport-i2s/transport-i2s.ino`
  - `platform/esp32/transport-dma-spi/transport-dma-spi.ino`
- ESP8266:
  - `platform/esp8266/transport-dma-i2s/transport-dma-i2s.ino`
  - `platform/esp8266/transport-dma-uart/transport-dma-uart.ino`

## Existing Utility Example

- `platform/platformio-smoke/src/main_virtual_smoke.cpp`: Minimal compile/smoke target used by the workspace build filter.
