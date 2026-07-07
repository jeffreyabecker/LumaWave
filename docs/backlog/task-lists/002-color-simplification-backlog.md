# Color Simplification Backlog

Purpose: radically simplify the color object by fixing channel count to exactly 4 (RGBW) and eliminating the internal padding/storage-widening mechanism (`InternalSize`, `InternalStorageComponent`, `ComponentReference`). The result is a single non-template `RgbwColor<TComponent>` with storage exactly `4 × sizeof(TComponent)`.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Phases 1–6 complete. Branch `refactor-color`.

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
- Removed: `Rgb8Color`, `Rgb16Color`, `Rgbcw8Color`, `Rgbcw16Color`, `RgbBasedColor<>` template, `ChannelOrder::RGBCW/GRBCW/BGRCW`, `ChannelSource<T,5>`, `ChannelSource<T,3>`

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

- [x] **`P2a`** — Replaced `ColorChannelIndexIterator<NChannels>` with non-template `ColorChannelIndexIterator`. Channel order is always `R, G, B, W`. Added free `channelIndex()`, `channelAt()`, `isSupportedChannelTag()` utilities.
- [x] **`P2b`** — Replaced `ColorChannelIndexRange<NChannels>` with non-template version. `indexFromChannel()` always supports `R, G, B, W`.
- [x] **`P2c`** — `Color.h`: dropped `colors/ChannelOrder.h` include — replaced with hardcoded `"RGBW"` in `serialize()`. `ChannelOrder.h`: removed `RGBCW`, `GRBCW`, `BGRCW` declarations and 5-channel branches from `normalizeChannelOrderForCount()`.
- [x] **`P2d`** — Replaced `ChannelSource<TColor, TValue, Enable>` template specializations with fixed `ChannelSource<TValue>` (4-channel, R/G/B/W).
- [x] **`P2e`** — Replaced `ChannelMap<TColor, TValue>` with `ChannelMap<TValue>`. Fixed 4-channel storage, removed `HasIntegralC`, `if constexpr (ChannelCount >= 5)` branch. Uses free `channelIndex()`/`isSupportedChannelTag()` from iterator utilities.

### Phase 3 — Protocols

Protocols still need their own channel count for wire-format serialization (e.g., WS2812 sends 3 bytes/pixel, TM1814 sends 4). This is a protocol implementation detail, not a color object property.

- [x] **`P3a`** — `DotStarProtocol.h`: changed default template params from `Rgb8Color`/`Rgb16Color` to `Rgbw8Color`/`Rgbw16Color`. Removed `InterfaceColorType::ChannelCount` static_asserts. `StripColorType` template retained for strip format (3-channel for APA102).
- [x] **`P3b`** — `Ws2812xProtocol.h`: removed `InterfaceColorType::ChannelCount` static_assert. Kept `StripColorType` for wire format (3 or 4 channel).
- [x] **`P3c`** — `Sm168xProtocol.h`: removed 5-channel `resolveSettingsSize()` case (always 2 now), `maxGain()` (always 0x0f), `channelIndexFromTag()` simplified to fixed RGWB. Interface static_assert removed, strip limited to 3-4 channels.
- [x] **`P3d`** — `IProtocol.h`: no change needed — it's generic over `TColor`.
- [x] **`P3e`** — All other protocol headers: `Lpd6803Protocol.h`, `Lpd8806Protocol.h`, `P9813Protocol.h`, `PixieProtocol.h`, `Sm16716Protocol.h`, `Tlc59711Protocol.h`, `Ws2801Protocol.h`, `Tm1814Protocol.h`, `Tm1914Protocol.h`, `DebugProtocol.h` — removed all `InterfaceColorType::ChannelCount` static_asserts; changed default `Rgb8Color`/`Rgb16Color` params to `Rgbw8Color`/`Rgbw16Color`; replaced `Rgb8Color` strip typedefs with comments. `ProtocolAliases.h`: fixed `DotStar`, `Hd108`, `None`, `Debug`, `Tm1914` defaults; replaced `ColorAtLeastAsLarge` with direct `sizeof(ComponentType)` comparisons.

### Phase 4 — Transports and buses

Transports and buses still need their own channel count for hardware pin mapping and frame buffer sizing. This is an implementation detail of each transport/bus, not a color object property.

- [x] **`P4a`** — `AnalogPwmLightDriver.h`: `MaxChannels` 5→4; `PinsMap` from `ChannelMap<Rgbcw8Color, int>` to `ChannelMap<int>`.
- [x] **`P4b`** — `Esp32SigmaDeltaLightDriver.h`, `Esp32LedcLightDriver.h`, `RpPwmLightDriver.h`: same `MaxChannels` 5→4 and `PinsMap` fix.
- [x] **`P4c`** — `PixelBus.h`: replaced `ColorType::channelIndexes()` with `{'R', 'G', 'B', 'W'}`.
- [x] **`P4d`** — `ReferenceBus.h`: same channelIndexes fix.
- [x] **`P4e`** — `PrintLightDriver.h`: same channelIndexes fix.

### Phase 5 — Public surface

- [x] **`P5a`** — `LumaWave.h`: removed all color aliases except `Rgbw8Color`, `Rgbw16Color`. Removed `ChannelOrder::RGBCW/GRBCW/BGRCW`.
- [x] **`P5b`** — `LumaWave.h`: removed `Ws2805` alias with comment; all protocol aliases updated to `Rgbw8Color`/`Rgbw16Color`.

### Phase 6 — Build and configuration

- [x] **`P6a`** — `Compat.h`: removed `LW_COLOR_MINIMUM_COMPONENT_COUNT` entirely. Kept only `LW_COLOR_MINIMUM_COMPONENT_SIZE`.
- [x] **`P6b`** — `docs/usage/compilation-flags.md`: removed `LW_COLOR_MINIMUM_COMPONENT_COUNT` documentation; updated `LW_COLOR_MINIMUM_COMPONENT_SIZE` notes to reflect always-4-channel.

### Phase 7 — Tests

- [ ] **`P7a`** — `test/protocols/test_protocol_spec_sections_1_5_to_1_13/`: remove RGBCW test cases.
- [ ] **`P7b`** — `test/protocols/test_protocol_debug_pipeline/`: change `TestColor = lw::Rgbcw8Color` → `lw::Rgbw8Color`.
- [ ] **`P7c`** — Search and update all test files that use `Rgb8Color`, `Rgb16Color`, `Rgbcw8Color`, `Rgbcw16Color`, `RgbBasedColor`, or `'C'` channel — replace with `Rgbw8Color`/`Rgbw16Color`.
- [ ] **`P7d`** — Update `test/shaders/test_color_domain_section1/` — remove tests that verify 3-channel or 5-channel behavior (type cleanup, not shader logic).
- [ ] **`P7e`** — Remove `test/shaders/test_cct_white_balance_shader_section8/` directory (references removed color types).

### Phase 8 — Examples

- [ ] **`P8a`** — Remove entire `examples/shaders/cct-white-balance/` directory (references removed color types).
- [ ] **`P8b`** — `examples/hello/light/`: change from `Rgbcw16Color` to `Rgbw16Color`.
- [ ] **`P8c`** — Search all examples for `Rgb8Color`, `Rgb16Color`, `Rgbcw*` and update to `Rgbw8Color`/`Rgbw16Color`.

### Phase 9 — Validation

- [ ] **`P9a`** — Configure and build native tests: `cmake -S . -B build && cmake --build build`
- [ ] **`P9b`** — Run full test suite: `ctest --test-dir build --output-on-failure`
- [ ] **`P9c`** — Verify no remaining references to `RgbBasedColor`, `Rgb8Color`, `Rgb16Color`, `Rgbcw`, `RGBCW`, `InternalSize`, `InternalStorageComponent`, `ComponentReference`, `NChannels`, `ChannelCount` (as template param), or `'C'` channel in active source/test paths.

## Dependencies Between Phases

```text
Phase 1 (Color.h) ──► Phase 2 (channel infra) ──► Phase 3-5 (protocols, transports, surface)
                                                         │
                                                         └──► Phase 6 (build/config)
                                                         │
                                                         └──► Phase 7-8 (tests, examples)
                                                                     │
                                                                     └──► Phase 9 (validation)
```

Phases 3–5 can be done in any order after Phase 2, but all must complete before Phase 9.
