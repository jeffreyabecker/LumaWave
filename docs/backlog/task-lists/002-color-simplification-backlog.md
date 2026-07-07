# Color Simplification Backlog

Purpose: radically simplify the color object by fixing channel count to exactly 4 (RGBW) and eliminating the internal padding/storage-widening mechanism (`InternalSize`, `InternalStorageComponent`, `ComponentReference`). The result is a single non-template `RgbwColor<TComponent>` with storage exactly `4 × sizeof(TComponent)`.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Phase 1 complete. Branch `refactor-color`.

## Motivation

The `RgbBasedColor<NChannels, TComponent, InternalSize>` template has three orthogonal axes of complexity:

| Axis | Current Values | Problem |
|---|---|---|
| **Channel count** | 3, 4, **5** (RGB / RGBW / RGBCW) | 5-channel is used by only 1 real chip (WS2805) and 2 shaders; drags `'C'` channel through every iterator, channel order, and serialization path. Even 3 vs 4 adds `NChannels` as a template parameter threaded everywhere. |
| **Component type** | `uint8_t`, `uint16_t` | Fine — keep both. |
| **Internal padding** | `InternalSize`, `InternalStorageComponent`, `ComponentReference` | Exists to let external `uint8_t` colors live in `uint16_t` storage. Adds `ComponentReference` proxy class, SFINAE on every accessor, and compile-time padding logic. Rarely exercised, high cognitive cost. |

Fixing channel count to exactly 4 eliminates the `NChannels` template parameter entirely. All downstream code knows the channel layout is always `R, G, B, W`. No more `ChannelCount` branching, no more `NChannels == 3` vs `== 4` SFINAE, no more `expand`/`compress` across channel counts.

## Target Shape After This Backlog

- `RgbwColor<TComponent>` — no `NChannels`, no `InternalSize`, just `std::array<TComponent, 4>`
- Channel count: always **4** (RGBW)
- Component type: `uint8_t` or `uint16_t`
- Concrete aliases: `Rgbw8Color`, `Rgbw16Color`
- Removed: `Rgb8Color`, `Rgb16Color`, `Rgbcw8Color`, `Rgbcw16Color`, `RgbBasedColor<>` template, `CCTWhiteBalanceShader`, `AutoWhiteBalanceShader::dualWhite`, `ChannelOrder::RGBCW/GRBCW/BGRCW`, `ChannelSource<T,5>`, `ChannelSource<T,3>`

## Non-Goals

- Do not change the color math, palette, or blend infrastructure except where they reference removed types/traits.
- Do not change transport or bus code's runtime behavior — only remove `NChannels` template parameters and channel-count branching.
- Do not add compatibility shims, deprecated aliases, or `[[deprecated]]` attributes for removed types.
- Do not change `LW_COLOR_MINIMUM_COMPONENT_SIZE` behavior — 8-bit vs 16-bit selection remains.

## Task List

### Phase 1 — Core `Color.h` surgery

- [x] **`P1a`** — Replaced `RgbBasedColor<NChannels, TComponent>` with `RgbwColor<TComponent>`. Storage is `std::array<TComponent, 4>`. Removed `InternalSize`, `InternalStorageComponent`, `ComponentReference` (already gone in prior pass).
- [x] **`P1b`** — Removed all `ChannelCount`-dependent branching: packed int conversions (no SFINAE), `defaultSerializeChannelOrder` (always RGBW), `tryParseToken` (only 8/16 digit cases), `defaultHexChannelTag` (fixed RGWB lookup), `canonicalChannelTags`, `compareCanonical`.
- [x] **`P1c`** — Removed `InternalChannelCount<>`, `AliasInternalSize`, `DefaultInternalSize` constants.
- [x] **`P1d`** — Removed all type aliases except `Rgbw8Color` and `Rgbw16Color`.
- [x] **`P1e`** — Simplified `DefaultColorType` to `Rgbw8Color` or `Rgbw16Color` based on `LW_COLOR_MINIMUM_COMPONENT_SIZE`.
- [x] **`P1f`** — Removed `widen()`/`narrow()`/`expand()`/`compress()`.
- [x] **`P1g`** — Removed all `ColorChannelsExactly<N>`, `ColorChannelsAtLeast<N>`, `ColorChannelsAtMost<N>`, `ColorChannelsInRange` traits.
- [x] **`P1h`** — Removed `RequireColorChannelsExactly`, `RequireColorChannelsInRange`; kept `RequireColorComponentBitDepth`.
- [x] **`P1i`** — Simplified `ColorType<TColor>` trait — no longer checks `ChannelCount`. Also removed `ColorComponentAtLeastAsLarge`, `ColorChannelAtLeastAsLarge`, `ColorAtLeastAsLarge`, `LargerColorType`, `ColorMinimumComponentCount`.

### Phase 2 — Channel infrastructure

- [ ] **`P2a`** — Replace `ColorChannelIndexIterator<NChannels>` with non-template `ColorChannelIndexIterator`. Channel order is always `R, G, B, W`. Remove `'C'`, `channelCount()`, `isSupportedChannelTag()`, `indexFromChannel()`.
- [ ] **`P2b`** — Replace `ColorChannelIndexRange<NChannels>` with non-template version. `indexFromChannel()` always supports `R, G, B, W`.
- [ ] **`P2c`** — `ChannelOrder.h`: remove `RGBCW`, `GRBCW`, `BGRCW` declarations; remove `normalizeChannelOrderForCount()` entirely (no branching needed); keep only `RGB`, `GRB`, `BGR`, `RGBW`, `GRBW`, `BGRW`, `WRGB`, `W`, `CW`.
- [ ] **`P2d`** — `ChannelSource.h`: replace with fixed 4-channel `ChannelSource` — remove template specializations.
- [ ] **`P2e`** — `ChannelMap.h`: remove `>= 5` and `>= 4` branches — always 4 channels.

### Phase 3 — Shaders

- [ ] **`P3a`** — Remove entire `CCTWhiteBalanceShader.h` (requires 5-channel `ColorChannelsAtLeast<TColor, 5>`).
- [ ] **`P3b`** — Strip `dualWhite` mode from `AutoWhiteBalanceShader.h` (removes `color['C']` usage, `_coolCorrection`, and the `bool _dualWhite` member).
- [ ] **`P3c`** — Remove `ColorChannelsAtLeast<TColor, 4>` constraints from shader templates — always 4 channels.

### Phase 4 — Protocols

- [ ] **`P4a`** — `DotStarProtocol.h`: remove `ChannelCount` template on `InterfaceColorType`/`StripColorType`; fix `static_assert`s to always expect 4 channels. Remove `StripChannelCount` as a derived constant.
- [ ] **`P4b`** — `Ws2812xProtocol.h`: remove `ChannelCount` template on `InterfaceColorType`; fix `static_assert`s to always expect 4 channels.
- [ ] **`P4c`** — `Sm168xProtocol.h`: remove 5-channel `resolveSettingsSize()` and `maxGain()` cases; remove `channelIndexFromTag()`; fix `static_assert`s to always expect 4 channels.
- [ ] **`P4d`** — `IProtocol.h`: remove `NChannels` template parameter from `IProtocol<TColor>` — `TColor` is always `RgbwColor<TComponent>`.
- [ ] **`P4e`** — All other protocol headers: remove `ChannelCount`-derived constants and `static_assert` branching; fix to always expect 4-channel color.

### Phase 5 — Transports and buses

- [ ] **`P5a`** — `AnalogPwmLightDriver.h`: change `MaxChannels = 5` → `4`; change `PinsMap` from `Rgbcw8Color` to `Rgbw8Color`.
- [ ] **`P5b`** — All `PixelBus`/`LightBus`/`AggregateBus` etc.: remove `NChannels` from color template parameters — always 4.

### Phase 6 — Public surface

- [ ] **`P6a`** — `LumaWave.h`: remove all color aliases except `Rgbw8Color`, `Rgbw16Color`. Remove `ChannelOrder::RGBCW/GRBCW/BGRCW`. Remove `PixelView<TColor>` as template (always `PixelView<RgbwColor<TComponent>>`).
- [ ] **`P6b`** — `LumaWave.h`: update/remove `Ws2805` alias; fix any aliases still referencing `RgbBasedColor` or 3-channel colors.

### Phase 7 — Build and configuration

- [ ] **`P7a`** — `Compat.h`: remove `LW_COLOR_MINIMUM_COMPONENT_COUNT` entirely (channel count is always 4). Keep only `LW_COLOR_MINIMUM_COMPONENT_SIZE`.
- [ ] **`P7b`** — `docs/usage/compilation-flags.md`: remove `LW_COLOR_MINIMUM_COMPONENT_COUNT` documentation; keep only `LW_COLOR_MINIMUM_COMPONENT_SIZE` and `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES`.

### Phase 8 — Tests

- [ ] **`P8a`** — Remove entire `test/shaders/test_cct_white_balance_shader_section8/` directory.
- [ ] **`P8b`** — `test/protocols/test_protocol_spec_sections_1_5_to_1_13/`: remove RGBCW test cases.
- [ ] **`P8c`** — `test/protocols/test_protocol_debug_pipeline/`: change `TestColor = lw::Rgbcw8Color` → `lw::Rgbw8Color`.
- [ ] **`P8d`** — Search and update all test files that use `Rgb8Color`, `Rgb16Color`, `Rgbcw8Color`, `Rgbcw16Color`, `RgbBasedColor`, or `'C'` channel — replace with `Rgbw8Color`/`Rgbw16Color`.
- [ ] **`P8e`** — Update `test/shaders/test_color_domain_section1/` — remove tests that verify 3-channel or 5-channel behavior.

### Phase 9 — Examples

- [ ] **`P9a`** — Remove entire `examples/shaders/cct-white-balance/` directory.
- [ ] **`P9b`** — `examples/hello/light/`: change from `Rgbcw16Color` to `Rgbw16Color`.
- [ ] **`P9c`** — Search all examples for `Rgb8Color`, `Rgb16Color`, `Rgbcw*` and update to `Rgbw8Color`/`Rgbw16Color`.

### Phase 10 — Validation

- [ ] **`P10a`** — Configure and build native tests: `cmake -S . -B build && cmake --build build`
- [ ] **`P10b`** — Run full test suite: `ctest --test-dir build --output-on-failure`
- [ ] **`P10c`** — Verify no remaining references to `RgbBasedColor`, `Rgb8Color`, `Rgb16Color`, `Rgbcw`, `RGBCW`, `InternalSize`, `InternalStorageComponent`, `ComponentReference`, `NChannels`, `ChannelCount` (as template param), or `'C'` channel in active source/test paths.

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
