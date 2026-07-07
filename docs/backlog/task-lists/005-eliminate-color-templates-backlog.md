# Eliminate Color Templates Backlog

Purpose: eliminate all color type template parameters from the codebase. Replace `RgbwColor<TComponent>` with a single `Color` class whose bit-depth is controlled by a compile-time flag. Remove `TColor` template from `IPixelBus`, `IProtocol`, and all bus/transport types.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Not started. Branch `refactor-color`.

## Motivation

After the 002 backlog, the color object is always 4-channel RGBW. The only remaining template axis is `TComponent` (uint8_t vs uint16_t), controlled at compile-time by `LW_COLOR_COMPONENT_SIZE`. This template threads through every layer of the architecture:

| Layer | Current | Desired |
|---|---|---|
| Color | `RgbwColor<uint8_t>` / `RgbwColor<uint16_t>` | `Color` (single class) |
| IPixelBus | `IPixelBus<TColor>` | `IPixelBus` |
| IProtocol | `IProtocol<TColor>` | `IProtocol` |
| PixelBus | template on ColorType derived from protocol | template-free |
| LightBus | `LightBus<TColor, TDriver>` | `LightBus<TDriver>` |
| ILightDriver | `ILightDriver<TColor>` | `ILightDriver` |

Since bit-depth is a global compile-time decision (`LW_COLOR_COMPONENT_SIZE`), it should be a global `using` or `typedef`, not a template parameter threaded through every type.

## Target Shape After This Backlog

- `Color` — a single non-template class. `Color::ComponentType` is `uint8_t` or `uint16_t` based on `LW_COLOR_COMPONENT_SIZE`.
- `RgbwColor<>`, `Rgbw8Color`, `Rgbw16Color` — removed.
- `IPixelBus` — no template parameter.
- `IProtocol` — no template parameter.
- `PixelBus<TProtocol, TTransport>` — no color type derived from protocol.
- `LightBus<TDriver>` — single template param.
- `LightBus<ColorType, TDriver>` → `LightBus<TDriver>`.
- `ILightDriver` — no template parameter.
- `PixelView<TColor>` → `PixelView` (or eliminated entirely per 004 backlog).
- All protocol template params `TInterfaceColor` — removed.

## Non-Goals

- Do not change the runtime behavior of color math, palette, or blend infrastructure.
- Do not change wire format or protocol serialization logic.
- Do not change `LW_COLOR_COMPONENT_SIZE` behavior — 8-bit vs 16-bit selection remains the same.

## Task List

### Phase 1 — Core `Color.h` surgery

- [ ] **`P1a`** — Replace `RgbwColor<TComponent>` with single `Color` class. `ComponentType` becomes `std::conditional_t<(LW_COLOR_COMPONENT_SIZE == 16), uint16_t, uint8_t>`. Remove template parameter.
- [ ] **`P1b`** — Remove `Rgbw8Color`, `Rgbw16Color`, `RgbwColor<>` aliases. `Color` is the only type.
- [ ] **`P1c`** — Remove `DefaultColorType` alias — it was `Color` anyway.
- [ ] **`P1d`** — Remove all remaining color type aliases from `namespace lw` (already minimal after 002).

### Phase 2 — Core infrastructure

- [ ] **`P2a`** — `IPixelBus.h`: remove `TColor` template parameter. `IPixelBus` becomes a non-template class.
- [ ] **`P2b`** — `PixelView.h`: remove `TColor` template parameter (or eliminate per 004 backlog).
- [ ] **`P2c`** — `IProtocol.h`: remove `TColor` template parameter from `IProtocol`. `update()` takes `span<const Color>`.
- [ ] **`P2d`** — `ITransport.h` and `ILightDriver.h`: remove `TColor` template parameters.

### Phase 3 — Buses

- [ ] **`P3a`** — `PixelBus.h`: remove `ColorType` derived from protocol. All pixel storage/iteration uses `Color`.
- [ ] **`P3b`** — `LightBus.h`: remove `TColor` template param. Bus takes only `TDriver`.
- [ ] **`P3c`** — `AggregateBus.h`, `ReferenceBus.h`, `ReferenceLightBus.h`, `CompositeBus.h`: remove all `TColor` template params.

### Phase 4 — Protocols

- [ ] **`P4a`** — Remove `TInterfaceColor` template parameter from all protocol classes. All protocols use `Color`.
- [ ] **`P4b`** — `ProtocolAliases.h`: remove `TInterfaceColor` from all wrapper structs. Remove `ColorType` typedefs.
- [ ] **`P4c`** — `DebugProtocol.h`, `NilProtocol.h`, `ProtocolDecoratorBase.h`: remove color type templates.

### Phase 5 — Transports and light drivers

- [ ] **`P5a`** — Remove `TColor` from `ILightDriver` and all concrete light drivers (`AnalogPwmLightDriver`, `Esp32SigmaDeltaLightDriver`, etc.).
- [ ] **`P5b`** — Remove `PlatformDefaultLightDriver<TColor>` → `PlatformDefaultLightDriver`.

### Phase 6 — Public surface

- [ ] **`P6a`** — `LumaWave.h`: remove `Rgbw8Color`, `Rgbw16Color` aliases. Export only `Color`.
- [ ] **`P6b`** — `LumaWave.h`: remove `TColor` defaults from `Light<>`, `IStrip<>`, `Palette<>`, etc. All use `Color`.
- [ ] **`P6c`** — `LumaWave.h`: remove `PixelView<TColor>` alias.

### Phase 7 — Color math, palette, and utilities

- [ ] **`P7a`** — `ColorMath.h`: remove `TColor` template, use `Color` directly.
- [ ] **`P7b`** — `palette/` files: remove `TColor` template, use `Color` directly.
- [ ] **`P7c`** — `ChannelMap.h`, `ChannelSource.h`: remove remaining template params.
- [ ] **`P7d`** — `Colors.h`: update umbrella include.

### Phase 8 — Tests

- [ ] **`P8a`** — Bulk replace `lw::Rgbw8Color` / `lw::Rgbw16Color` → `lw::Color` in all test files.
- [ ] **`P8b`** — Bulk replace `using TestColor = lw::Rgbw8Color` → `using TestColor = lw::Color` in test files.
- [ ] **`P8c`** — Remove all `TColor` template parameters from test protocol/transport/bus helpers.

### Phase 9 — Validation

- [ ] **`P9a`** — Configure and build native tests.
- [ ] **`P9b`** — Run full test suite.
- [ ] **`P9c`** — Verify no remaining references to `RgbwColor`, `Rgbw8Color`, `Rgbw16Color`, `TColor`, `TInterfaceColor` in active source paths.

## Dependencies Between Phases

```text
Phase 1 (Color.h) ──► Phase 2 (core infra) ──► Phase 3-5 (buses, protocols, transports)
                                                    │
                                                    └──► Phase 6 (public surface)
                                                    │
                                                    └──► Phase 7 (color math, palette)
                                                    │
                                                    └──► Phase 8 (tests)
                                                                │
                                                                └──► Phase 9 (validation)
```

Phases 3–7 can be done in any order after Phase 2, but all must complete before Phase 9.
