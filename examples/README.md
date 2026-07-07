# Examples Index

Examples in this folder use the public include surface from `LumaWave.h`.
When a suitable global alias is not available, examples intentionally use fully qualified `lw::...` names.

## Hello Examples

- `hello/ws2812/ws2812.ino`: Basic one-wire strip animation.
- `hello/apa102/apa102.ino`: Basic SPI/DotStar strip animation.
- `hello/light/light.ino`: Single-light RGBW16 output with platform-default light driver.

## Strip Composition

- `multi-strip/composite-strip/composite-strip.ino`: Composite strip composition (ownership model).
- `multi-strip/aggregate-strip/aggregate-strip.ino`: Aggregate strip composition (non-owning references).
- `multi-strip/topology-tiled-grid/topology-tiled-grid.ino`: `Topology` mapping for a 4x4 grid of 16x16 tiles.

Choose composite strip composition when you want ownership in one composed object.
Choose aggregate strip composition when child strips are created elsewhere and only referenced.
Choose `AggregateStrip` when the aggregate should own heap-allocated child strips.

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
