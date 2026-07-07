# Eliminate Color Templates Backlog

Purpose: eliminate all color type template parameters from the codebase. Replace `RgbwColor<TComponent>` with a single `Color` class whose bit-depth is controlled by a compile-time flag. Remove `TColor` template from `IPixelBus`, `IProtocol`, and all bus/transport types.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

**Completed**. Branch `refactor-color`. All phases done — build compiles with 0 errors (1 pre-existing linker-only issue in a compile-contract test). All sub-items marked `[x]`. Remaining work tracked separately under Backlog 006 (unify bus interface).

## Motivation

After the 002 backlog, the color object is always 4-channel RGBW. The only remaining template axis is `TComponent` (uint8_t vs uint16_t), controlled at compile-time by `LW_COLOR_COMPONENT_SIZE`. This template threads through every layer of the architecture:

| Layer | Before | After |
|---|---|---|
| Color | `RgbwColor<uint8_t>` / `RgbwColor<uint16_t>` | `Color` (single class) |
| IPixelBus | `IPixelBus<TColor>` | `IPixelBus` |
| IProtocol | `IProtocol<TColor>` | `IProtocol` |
| PixelBus | template on ColorType derived from protocol | template-free |
| LightBus | `LightBus<TColor, TDriver>` | `LightBus<TDriver>` |
| ILightDriver | `ILightDriver<TColor>` | `ILightDriver` |

Since bit-depth is a global compile-time decision (`LW_COLOR_COMPONENT_SIZE`), it should be a global `using` or `typedef`, not a template parameter threaded through every type.

## Task List

### Phase 1 — Core `Color.h` surgery

- [x] **`P1a`** — Replace `RgbwColor<TComponent>` with single `Color` class. `ComponentType` becomes `std::conditional_t<(LW_COLOR_COMPONENT_SIZE == 16), uint16_t, uint8_t>`. Remove template parameter.
- [x] **`P1b`** — Remove `Rgbw8Color`, `Rgbw16Color`, `RgbwColor<>` aliases. `Color` is the only type.
- [x] **`P1c`** — Remove `DefaultColorType` alias — it was `Color` anyway.
- [x] **`P1d`** — Remove all remaining color type aliases from `namespace lw` (already minimal after 002).

### Phase 2 — Core infrastructure

- [x] **`P2a`** — `IPixelBus.h`: remove `TColor` template parameter. `IPixelBus` becomes a non-template class.
- [x] **`P2b`** — `PixelView.h`: remove `TColor` template parameter (or eliminate per 004 backlog).
- [x] **`P2c`** — `IProtocol.h`: remove `TColor` template parameter from `IProtocol`. `update()` takes `span<const Color>`.
- [x] **`P2d`** — `ITransport.h` and `ILightDriver.h`: remove `TColor` template parameters.

### Phase 3 — Buses

- [x] **`P3a`** — `PixelBus.h`: remove `ColorType` derived from protocol. All pixel storage/iteration uses `Color`.
- [x] **`P3b`** — `LightBus.h`: remove `TColor` template param. Bus takes only `TDriver`.
- [x] **`P3c`** — `AggregateBus.h`, `ReferenceBus.h`, `ReferenceLightBus.h`, `CompositeBus.h`: remove all `TColor` template params.

### Phase 4 — Protocols

- [x] **`P4a`** — Remove `TInterfaceColor` template parameter from all protocol classes. All protocols use `Color`.
- [x] **`P4b`** — `ProtocolAliases.h`: remove `TInterfaceColor` from all wrapper structs. Remove `ColorType` typedefs.
- [x] **`P4c`** — `DebugProtocol.h`, `NilProtocol.h`, `ProtocolDecoratorBase.h`: remove color type templates.

### Phase 5 — Transports and light drivers

- [x] **`P5a`** — Remove `TColor` from `ILightDriver` and all concrete light drivers (`AnalogPwmLightDriver`, `Esp32SigmaDeltaLightDriver`, etc.).
- [x] **`P5b`** — Remove `PlatformDefaultLightDriver<TColor>` → `PlatformDefaultLightDriver`.

### Phase 6 — Public surface

- [x] **`P6a`** — `LumaWave.h`: remove `Rgbw8Color`, `Rgbw16Color` aliases. Export only `Color`.
- [x] **`P6b`** — `LumaWave.h`: remove `TColor` defaults from `Light<>`, `IStrip<>`, `Palette<>`, etc. All use `Color`.
- [x] **`P6c`** — `LumaWave.h`: remove `PixelView<TColor>` alias.

### Phase 7 — Color math, palette, and utilities

- [x] **`P7a`** — `ColorMath.h`: remove `TColor` template, use `Color` directly.
- [x] **`P7b`** — `palette/` files: remove `TColor` template, use `Color` directly.
- [x] **`P7c`** — `ChannelMap.h`, `ChannelSource.h`: remove remaining template params.
- [x] **`P7d`** — `Colors.h`: update umbrella include.

### Phase 8 — Tests

- [x] **`P8a`** — Bulk replace `lw::Rgbw8Color` / `lw::Rgbw16Color` → `lw::Color` in all test files.
- [x] **`P8b`** — Bulk replace `using TestColor = lw::Rgbw8Color` → `using TestColor = lw::Color` in test files.
- [x] **`P8c`** — Remove all `TColor` template parameters from test protocol/transport/bus helpers.

### Phase 9 — Validation

- [x] **`P9a`** — Configure and build native tests.
- [x] **`P9b`** — Run full test suite.
- [x] **`P9c`** — Verify no remaining references to `RgbwColor`, `Rgbw8Color`, `Rgbw16Color`, `TColor`, `TInterfaceColor` in active source paths.

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
