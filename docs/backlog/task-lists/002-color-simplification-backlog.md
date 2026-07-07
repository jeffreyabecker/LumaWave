# Color Simplification Backlog

Purpose: radically simplify `RgbBasedColor` by removing 5-channel (RGBCW) support and eliminating the internal padding/storage-widening mechanism (`InternalSize`, `InternalStorageComponent`, `ComponentReference`). The result is a template that takes only `NChannels` and `TComponent`, with storage exactly `NChannels × sizeof(TComponent)`.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Not started. Branch `refactor-color` created.

## Motivation

The `RgbBasedColor<NChannels, TComponent, InternalSize>` template has three orthogonal axes of complexity:

| Axis | Current Values | Problem |
|---|---|---|
| **Channel count** | 3, 4, **5** (RGB / RGBW / RGBCW) | 5-channel is used by only 1 real chip (WS2805) and 2 shaders; drags `'C'` channel through every iterator, channel order, and serialization path. |
| **Component type** | `uint8_t`, `uint16_t` | Fine — keep both. |
| **Internal padding** | `InternalSize`, `InternalStorageComponent`, `ComponentReference` | Exists to let external `uint8_t` colors live in `uint16_t` storage. Adds `ComponentReference` proxy class, SFINAE on every accessor, and compile-time padding logic. Rarely exercised, high cognitive cost. |

Removing these yields 4 concrete types (`Rgb8`, `Rgbw8`, `Rgb16`, `Rgbw16`) instead of 6, eliminates the `ComponentReference` proxy, and lets channel iterators/orders/serialization assume at most 4 channels.

## Target Shape After This Backlog

- `RgbBasedColor<NChannels, TComponent>` — no third parameter
- Storage: `std::array<TComponent, NChannels>` — always exact-fit, no padding
- Channel count: `3` (RGB) or `4` (RGBW) only
- Component type: `uint8_t` or `uint16_t` only
- Concrete aliases: `Rgb8Color`, `Rgbw8Color`, `Rgb16Color`, `Rgbw16Color`
- Removed: `Rgbcw8Color`, `Rgbcw16Color`, `CCTWhiteBalanceShader`, `AutoWhiteBalanceShader::dualWhite`, `ChannelOrder::RGBCW/GRBCW/BGRCW`, `ChannelSource<T,5>`

## Non-Goals

- Do not change the color math, palette, or blend infrastructure.
- Do not change transport, protocol (beyond static asserts and 5-channel branches), or bus code except where required by type removal.
- Do not add compatibility shims or deprecated aliases for removed types.
- Do not change the `LW_COLOR_MINIMUM_*` compilation flags yet — remove `== 5` from valid ranges only.

## Task List

### Phase 1 — Core `Color.h` surgery

- [ ] **`P1a`** — Remove `InternalSize`, `InternalStorageComponent`, `ComponentReference` from `RgbBasedColor`. Storage becomes `std::array<TComponent, NChannels>` directly.
- [ ] **`P1b`** — Remove all `NChannels == 5` / `>= 5` branches: `defaultSerializeChannelOrder`, `tryParseToken` (10-digit, 20-digit cases), `defaultHexChannelTag` (C-channel logic), `canonicalChannelTags` (shrink to 4).
- [ ] **`P1c`** — Remove `AliasInternalSize`, `DefaultInternalSize` constants.
- [ ] **`P1d`** — Remove `Rgbcw8Color`, `Rgbcw16Color` type aliases.
- [ ] **`P1e`** — Simplify `DefaultColorType` conditional — drop `>= 5` branches.
- [ ] **`P1f`** — Simplify `widen()`/`narrow()`/`expand()`/`compress()` — drop `InternalSize` parameter.
- [ ] **`P1g`** — Simplify `ColorChannelsAtLeast<..., 5>` trait and any related type traits that reference 5-channel.

### Phase 2 — Channel infrastructure

- [ ] **`P2a`** — `ColorChannelIndexIterator.h`: remove `'C'` from `channelAt()`, `channelCount()`, `isSupportedChannelTag()`, `indexFromChannel()`.
- [ ] **`P2b`** — `ChannelOrder.h`: remove `RGBCW`, `GRBCW`, `BGRCW` declarations and `>= 5` branches from `normalizeChannelOrderForCount()`.
- [ ] **`P2c`** — `ChannelSource.h`: remove `ChannelCount == 5` specialization.
- [ ] **`P2d`** — `ChannelMap.h`: remove `>= 5` branches from `IsTaggedChannelSource` and `assign()`.

### Phase 3 — Shaders

- [ ] **`P3a`** — Remove entire `CCTWhiteBalanceShader.h` (requires 5-channel `ColorChannelsAtLeast<TColor, 5>`).
- [ ] **`P3b`** — Strip `dualWhite` mode from `AutoWhiteBalanceShader.h` (removes `color['C']` usage, `_coolCorrection`, and the `bool _dualWhite` member).
- [ ] **`P3c`** — Verify `AutoWhiteBalanceShader` still compiles with only single-white (`ColorChannelsAtLeast<TColor, 4>`).

### Phase 4 — Protocols

- [ ] **`P4a`** — `DotStarProtocol.h`: tighten static_asserts from `[3, 5]` to `[3, 4]`.
- [ ] **`P4b`** — `Ws2812xProtocol.h`: tighten static_asserts from `[3, 5]` to `[3, 4]`.
- [ ] **`P4c`** — `Sm168xProtocol.h`: remove 5-channel `resolveSettingsSize()` case, `maxGain()` case, `channelIndexFromTag('C')` case; tighten static_asserts to `[3, 4]`.
- [ ] **`P4d`** — Re-map or remove the `Ws2805` alias from `LumaWave.h`. (WS2805 is a 5-channel chip; decide: drop the alias, or re-map to RGBW with caveats, or defer.)

### Phase 5 — Transports and buses

- [ ] **`P5a`** — `AnalogPwmLightDriver.h`: change `MaxChannels = 5` → `4`; change `PinsMap` from `Rgbcw8Color` to `Rgbw8Color`.

### Phase 6 — Public surface

- [ ] **`P6a`** — `LumaWave.h`: remove `Rgbcw8Color`, `Rgbcw16Color` using-declarations; remove `ChannelOrder::RGBCW/GRBCW/BGRCW`.
- [ ] **`P6b`** — `LumaWave.h`: update `Ws2805` or mark it removed; update any other alias referencing 5-channel types.

### Phase 7 — Build and configuration

- [ ] **`P7a`** — `Compat.h`: update `LW_COLOR_MINIMUM_COMPONENT_COUNT` validation — remove `== 5` as an allowed value.
- [ ] **`P7b`** — `docs/usage/compilation-flags.md`: update allowed range from `[3, 5]` to `[3, 4]` for `LW_COLOR_MINIMUM_COMPONENT_COUNT`; remove 5-channel documentation.

### Phase 8 — Tests

- [ ] **`P8a`** — Remove entire `test/shaders/test_cct_white_balance_shader_section8/` directory.
- [ ] **`P8b`** — `test/protocols/test_protocol_spec_sections_1_5_to_1_13/`: remove RGBCW test cases (SM168x 5-channel tests).
- [ ] **`P8c`** — `test/protocols/test_protocol_debug_pipeline/`: change `TestColor = lw::Rgbcw8Color` → `lw::Rgbw8Color` or `lw::Rgb8Color`.
- [ ] **`P8d`** — Search and update any remaining test files that reference `Rgbcw*Color` or `'C'` channel.

### Phase 9 — Examples

- [ ] **`P9a`** — Remove entire `examples/shaders/cct-white-balance/` directory.
- [ ] **`P9b`** — `examples/hello/light/`: change from `Rgbcw16Color` to `Rgbw16Color` (single white channel, no cool-white).

### Phase 10 — Validation

- [ ] **`P10a`** — Configure and build native tests: `cmake -S . -B build && cmake --build build`
- [ ] **`P10b`** — Run full test suite: `ctest --test-dir build --output-on-failure`
- [ ] **`P10c`** — Verify no remaining references to `Rgbcw`, `RGBCW`, `InternalSize`, `InternalStorageComponent`, `ComponentReference`, or `'C'` channel in active source/test paths.

## Dependencies Between Phases

```text
Phase 1 (Color.h) ──► Phase 2 (channel infra) ──► Phase 3-6 (shaders, protocols, transports, surface)
                                                         │
                                                         └──► Phase 7 (build/config)
                                                         │
                                                         └──► Phase 8-9 (tests, examples)
                                                                     │
                                                                     └──► Phase 10 (validation)
```

Phases 3–6 can be done in any order after Phase 2, but all must complete before Phase 10.
