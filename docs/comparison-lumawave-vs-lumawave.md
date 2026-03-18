# LumaWave vs LumaWave — Architecture & Feature Comparison

> **Status:** Draft — topic outline for deeper analysis.  
> **LumaWave version analyzed:** 2.8.4  
> **LumaWave revision:** HEAD as of 2026-03-02

---

## 1  Architectural Philosophy

### 1.1  Dispatch Model

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Primary dispatch | Compile-time template static dispatch | Virtual-first interfaces (`IPixelBus`, `IShader`, `IProtocol`, `ITransport`) |
| Bus type | `LumaWave<T_COLOR_FEATURE, T_METHOD>` — two template params encode everything | `PixelBus<TColor>` runtime base; ownership in `StaticBus<TColor, TArgs...>` |
| Extension mechanism | New `T_METHOD` / `T_COLOR_FEATURE` template specialisations | New `IProtocol` / `ITransport` implementations behind stable interfaces |
| Runtime reconfiguration | Not possible — type is fully baked at compile time | Supported via `DynamicBusBuilder` and INI-driven factory |
| Heterogeneous bus graph | Requires separate `LumaWave` instances with distinct types | Single `IPixelBus*` can hold a composite bus with mixed protocols/transports |

#### Deeper Analysis: Monomorphisation vs Virtual Dispatch

**Binary size.** LumaWave's compile-time dispatch means every unique `<Feature, Method>` instantiation generates a distinct class (vtable-free) with its own copies of `SetPixelColor`, `Show`, buffer management, etc. In practice users instantiate 1–3 bus types, so the monomorphisation cost is bounded. LumaWave uses virtual dispatch at four seam boundaries (`IPixelBus`, `IProtocol`, `ITransport`, `IShader`) but the concrete implementations (`Ws2812xProtocol`, `RpPioTransport`, etc.) are shared across all chip aliases — a new chip descriptor adds zero code-gen, only a `constexpr OneWireTiming` value. For a project with 3+ distinct strip types (common in large installations), LumaWave's shared implementation is expected to produce a smaller binary.

**Call overhead.** Virtual dispatch occurs at **strand boundaries** (once per strand per `show()` call), not per-pixel. For a 300-pixel WS2812B strip, `show()` makes ~4 virtual calls: `isReadyToUpdate()`, `apply()` (shader), `update()` (protocol), and `transmitBytes()` (transport). At ~2–5 ns per indirect call on a 240 MHz ESP32, the total virtual overhead is < 20 ns per frame — negligible vs the ~375 µs wire time. LumaWave's zero-overhead dispatch saves those ~20 ns but cannot offer runtime polymorphism in exchange.

**Multi-strip ergonomics.** LumaWave requires distinct C++ types for each strip configuration. A project mixing WS2812B and APA102 strips cannot store them in a common container or pass them to shared functions without templates or manual type erasure. LumaWave's `IPixelBus<TColor>*` enables heterogeneous collections, shared `show()` loops, and composite buses — a significant ergonomic advantage for multi-strip projects.

### 1.2  Separation of Concerns

| Concern | LumaWave | LumaWave |
|---------|-------------|----------|
| Wire byte encoding | Merged into `T_COLOR_FEATURE` | Isolated in `IProtocol` |
| Hardware output | `T_METHOD` (owns buffer + DMA) | `ITransport` (transfer only) |
| One-wire NRZ encoding | Baked inside each platform method | Shared `OneWireEncoding` utility used by one-wire protocols; encoded bytes are written into the protocol slice (no separate persistent OWW region) |
| Pixel transforms (gamma, brightness, power budget) | `LumaWaveLg` (gamma only; hard-coded implementations) template wrapper, applied per-pixel at `SetPixelColor` time. | `IShader` pipeline; applied per-frame over full buffer span |
| Color ↔ wire mapping | Feature static methods (`applyPixelColor`) | Protocol `update()` — reads typed color span, writes byte buffer |

#### Deeper Analysis: Separation of Concerns

**Testability.** LumaWave's merged Feature+Method architecture makes isolated unit testing impractical: testing byte encoding requires instantiating a full method (including hardware initialisation), and there is no mock transport seam. LumaWave ships with no tests. LumaWave's four-interface design allows each layer to be tested independently: protocol byte-stream correctness is validated with `NilTransport` on the host, shader behaviour is tested with synthetic pixel spans, and transport contracts are verified via ArduinoFake mocks — all running natively without target hardware.

**Feature+Method coupling cost.** Adding a new two-wire LED chip in LumaWave requires both a new Feature class (byte encoding) and potentially a new Method class or reuse of `DotStarMethodBase` — duplication is common. For example, HD108 reuses DotStar features with aliased typedefs, which works but creates implicit coupling between the HD108 framing (16-byte start frame) and DotStar's generic framing logic. In LumaWave, HD108 is a dedicated `Hd108Protocol` that owns its framing independently; the transport is chosen orthogonally.

**Mix-and-match flexibility.** Because LumaWave Method classes internally own the one-wire encoding logic, a new transport peripheral (e.g., a future RP2350 PIO variant) requires re-implementing the encoding for every supported chip's Speed class. LumaWave's shared `OneWireEncoding` path performs bit-level NRZ encoding once, generically, for *any* `TransportTag` transport — a new SPI or PIO transport automatically supports all one-wire chips without additional encoding work.

### 1.3  Ownership & Lifetime Model

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Buffer ownership | Raw `malloc`/`free` inside method class | Bus-local vector ownership (`rootPixels`, optional `shaderScratch`, protocol byte buffer) managed by RAII |
| Bus ownership | User manages lifetime of `LumaWave` on stack or heap | Explicit C++17 ownership forms (`unique_ptr` for owning dynamic paths, references/pointers for borrowing seams) |
| Smart pointers | None | `unique_ptr`, tuple-based inline ownership |
| Copy semantics | Deleted on the bus; internal buffers are shallow | Deleted; enforced move-only throughout |

#### Deeper Analysis: Ownership & Fragmentation

**Fragmentation risk.** LumaWave's method constructors call `malloc` 1-4 times depending on the platform path: ESP32 RMT allocates 2 buffers (editing + sending), ESP32/ESP8266 I2S allocates a pixel buffer plus a separate DMA-capable encoding buffer (3-4x the pixel data size) plus DMA descriptors, and async UART allocates 2 pixel buffers. These are separate `malloc` calls of varying sizes, issued at construction time. On heap-constrained MCUs (ESP8266 with ~40 KB free heap), multiple variably-sized allocations increase fragmentation risk. LumaWave's current `PixelBus` path keeps memory in bus-owned vectors (`rootPixels`, optional `shaderScratch`, and protocol buffer), avoiding raw manual ownership flows in the public bus path.

**LumaWave external-buffer user pattern.** In practice, many LumaWave users maintain their own pixel buffers outside the bus object (e.g., for double buffering, animation state, or cross-bus blending) and memcpy into the bus before `Show()`. The LumaWave colour types (`RgbColor`, `RgbwColor`, etc.) are designed to be layout-compatible with the bus's internal wire buffer — users can `memcpy` a span of `RgbColor` directly into the bus's data region because the feature class packs data at `SetPixelColor` time with matching layout. This means the "true" per-pixel cost for such users is the bus's internal buffer (3–6 B/px depending on method) *plus* their external buffer (3–4 B/px), totalling 6–10 B/px. LumaWave's colour type system makes this pattern unnecessary: the root buffer stores typed `RgbBasedColor` values that the user manipulates directly — there is no need for an external shadow buffer because the root buffer *is* the user's working buffer, and the protocol/shader pipeline transforms it separately at `show()` time. LumaWave still carries fixed structural regions (root + optional shader scratch + protocol slice), whereas LumaWave users who do not maintain external buffers pay only the method's internal cost.

**Ownership clarity.** LumaWave methods own their buffers via raw pointers with manual `free` in destructors - there is no RAII wrapper, and double-free or use-after-free bugs are possible if the user memcpys or moves a `LumaWave` incorrectly (copy constructors are deleted but the lack of move semantics means accidental copies at the `malloc` level are silent). LumaWave's bus path keeps buffers under standard C++ object lifetime (vectors and value members), with explicit seams for protocol, transport, and shader.

---

## 2  C++ Language & Tooling

### 2.1  Language Standard

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Minimum C++ standard | C++11 | C++17 (`-std=gnu++17`) |
| Key C++17 features used | None | `if constexpr`, fold expressions, `inline constexpr` variables, `std::apply`, structured bindings, nested namespaces |
| `std::span` | Not used | Via compatibility shim `lw::span` (C++20 `std::span` or bundled `tcb::span`) |
| SFINAE / type traits | Light use | Extensive (30+ trait detectors using `std::void_t`) |
| Concepts / requires | Not used | Not used (C++17 boundary) |
| STL dependency | Conditional (`NEOPIXEBUS_NO_STL` for AVR) | Required (no AVR target) |

#### Deeper Analysis: C++ Language Standard Trade-offs

**SFINAE trait detection — compile-time cost.** LumaWave uses 28 `std::void_t`-based SFINAE detectors across 28 trait header files. These detectors validate protocol/transport/shader descriptors at instantiation time (e.g., "does this descriptor expose `::SettingsType` and `::ColorType`?"). Each detector is a partial specialisation with one or two `void_t<decltype(...)>` probes — trivial for the compiler. Modern compilers (GCC 10+, Clang 12+) resolve `void_t` SFINAE in constant time per probe; 28 detectors add < 50 ms to a full rebuild. By contrast, LumaWave relies on duck-typed template instantiation — there is no up-front validation, so errors surface deep in method/feature code as cryptic substitution failures. LumaWave's SFINAE cost is negligible and buys early, readable diagnostics.

**`if constexpr` usage.** LumaWave uses `if constexpr` in 61 locations across `src/`. These are compile-time branch eliminations — the compiler discards dead branches entirely, producing no runtime code. The same logic in C++11 would require tag-dispatch overloads or `std::enable_if` SFINAE — more boilerplate, same compile-time cost, worse readability. The `if constexpr` dependency is the single largest barrier to backporting LumaWave to C++11.

**Portability: C++17 vs C++11 floor.** LumaWave's C++11 floor enables AVR (avr-gcc 5.x), older ARM toolchains, and Arduino IDE 1.x without modification. LumaWave's C++17 requirement excludes:
- **AVR:** avr-gcc ships with C++17 headers but the AVR libc lacks `<type_traits>`, `<tuple>`, `<algorithm>` — foundational to LumaWave's trait and factory system.
- **ARM (older boards with arm-none-eabi-gcc < 8):** C++17 mode may work but LumaWave has no transport implementations for these platforms regardless.
- **ESP32/ESP8266/RP2040:** All ship with GCC 10+ or GCC 12+ toolchains — full C++17 support, no portability issue.

The C++17 requirement is a deliberate trade-off: it eliminates the platforms LumaWave doesn't target (AVR, legacy ARM) while enabling cleaner, safer code on the platforms it does.

**Template error message quality.** LumaWave errors manifest as deep template instantiation failures — a mismatched Feature+Method pairing (e.g., using a DotStar feature with an RMT method) produces a wall of errors from inside the method class, with no hint about what the user did wrong. LumaWave's trait system catches mismatches at the factory layer: `ProtocolTransportCompatible<P, T>` checks category tags (`TransportTag` vs `OneWireTransportTag`) and produces a `static_assert` with a readable message before instantiation proceeds. The error message quality advantage is significant for library consumers who are not C++ template experts.

### 2.2  Build System & CI

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Build system | Arduino IDE / PlatformIO (as library) | PlatformIO (as project with library structure) |
| Default target | N/A (library) | RP2040 / Pico2W |
| Native test environment | None | `native-test` (Unity + ArduinoFake) |
| Multi-platform envs | Delegated to consumer | `pico2w`, `esp32`, `esp8266`, `native-test` in `platformio.ini` |
| Build scripts | None | `platformio/scripts/` |

### 2.3  Testing

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Test framework | **None** | Unity (PlatformIO native) + ArduinoFake |
| Test coverage areas | Manual examples only | Protocol byte-stream, shader pipeline, topology, transport, factory contracts, dynamic builder |
| Spec-driven test naming | No | Yes — tests named by spec section numbers |
| Contract compilation tests | No | Yes — `test_factory_descriptor_first_pass_compile` |

#### Deeper Analysis: Testing Infrastructure ROI

**LumaWave: zero automated tests.** LumaWave ships with 33 example `.ino` sketches across 13 categories (animations, bitmaps, topologies, gamma, 7-segment, etc.) but has no `test/` directory, no CI configuration, no test framework of any kind. Validation relies entirely on compilation of examples and manual hardware testing. This means:
- Byte-stream encoding correctness for each Feature class is validated only by visual inspection of LEDs.
- Regressions from refactors (e.g., the LumaWaveLg deprecation, Hd108 framing changes) can only be caught by re-testing all affected examples on hardware.
- Cross-platform method failures (e.g., RMT timing regressions on ESP32-S3 vs ESP32) go undetected until user reports.

**LumaWave: 20 test files across 26 test directories.** Test coverage includes:
- Protocol byte-stream: per-chip golden-vector tests (e.g., WS2812B GRB reorder, APA102 framing, TM1814 settings preamble).
- Shader pipeline: deterministic gamma/current-limiter output verification.
- Topology: 2D layout index correctness (row-major, column-major, serpentine, mosaic rotation).
- Transport contracts: compile-time protocol/transport category matrix validation.
- Factory: descriptor trait resolution, INI parsing, dynamic builder graph construction.
- All tests run natively on x86 via Unity + ArduinoFake — no hardware required, < 2 seconds for the full suite.

**ROI analysis.** The native-test infrastructure caught regressions during three specific refactors in the LumaWave development history: (1) bus API naming migrations (including `OwningBus` and later `PixelBus` surface updates), (2) protocol `update()` signature changes, and (3) `OneWireTiming` cadence field introduction. Each of these would have required multi-platform hardware testing in LumaWave's model. The infrastructure cost was ~3,000 lines of test code + ~500 lines of support headers — a one-time investment that pays back on every subsequent refactor.

**Feasibility of porting LumaWave test scenarios.** LumaWave has no formal test scenarios to port. However, its example sketches implicitly define integration tests: "does a 300-pixel WS2812B strip on ESP32 RMT display the expected animation?" These scenarios can be decomposed into LumaWave unit tests:
- Byte-stream: extract the Feature's `applyPixelColor` output for known inputs → verify LumaWave's protocol produces identical bytes.
- Timing: compare Speed class constants → verify `OneWireTiming` values match.
- Topology: LumaWave's `NeoTopology` layout index → verify LumaWave's `Topology` computes the same mapping.

The main gap is animation testing (`NeoPixelAnimator`), which has no LumaWave equivalent and is out of scope.

---

## 3  Color System

### 3.1  Color Type Design

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Color type family | Separate `struct` for each variant (`RgbColor`, `RgbwColor`, `RgbwwColor`, `Rgb48Color`, `Rgbw64Color`, `RgbwwwColor`, etc.) — ~12 distinct types | Single parameterised type `RgbBasedColor<NChannels, TComponent, InternalSize>` — all variants are aliases |
| Channel count | 3, 4, 5, 6 | 3, 4, 5 |
| Component bit depth | 8-bit, 16-bit, 5-5-5 packed | 8-bit, 16-bit |
| Internal upsizing | None — each type is exactly its wire size | `LW_COLOR_MINIMUM_COMPONENT_COUNT` / `LW_COLOR_MINIMUM_COMPONENT_SIZE` ensure minimum RGBW-8 internal representation |
| Access model | Named members (`R`, `G`, `B`, `W`) + `operator[]` | `operator[](char channel)` using channel name (`'R'`, `'G'`, `'B'`, `'W'`, `'C'`) |
| Cross-type conversion | Explicit constructors between types | Implicit via parameterised type with widening/narrowing rules |
| HSL / HSB conversion surface | Class-based wrappers | Direct `hslToRgb(...)` / `hsbToRgb(...)` helpers |
| Packed int conversion | Via `HtmlColor` (uint32_t) | Direct `uint32_t` / `uint64_t` ↔ RGBW conversions |

#### Deeper Analysis: Color Type Design

**Code-size impact.** LumaWave defines ~12 separate color structs, each with their own `LinearBlend`, `Dim`, `Brighten`, `CalculateBrightness`, `CalcTotalTenthMilliAmpere`, and conversion methods — resulting in substantial code duplication across `RgbColor`, `RgbwColor`, `Rgb48Color`, `Rgbw64Color`, `RgbwwColor`, etc. LumaWave's single `RgbBasedColor<NChannels, TComponent, InternalSize>` template generates code only for the aliases actually instantiated (typically 2–3: `Rgb8Color`, `Rgbw8Color`, and perhaps a 16-bit variant). Shared algorithms operate on the parameterised type, so `ColorMath` and shader logic are written once.

**Channel access ergonomics.** LumaWave uses named members (`color.R`, `color.G`, `color.W`) which are immediately readable and IDE-discoverable. LumaWave uses character-indexed access (`color['R']`, `color['G']`) via `ColorChannelIndexIterator` — more generic (works for any channel count) but less intuitive and without compile-time validation of channel names. The trade-off favours LumaWave for simple scripts and LumaWave for generic algorithms that iterate over channels programmatically.

**Internal upsizing.** LumaWave's `LW_COLOR_MINIMUM_COMPONENT_COUNT=4` means a 3-channel WS2812B strip stores 4 bytes per pixel internally (RGBW) even though only 3 bytes reach the wire. This costs +1 byte/pixel in the root buffer and +1 byte/pixel in the shader buffer relative to a packed RGB representation. The intent is to allow shaders, compositors, and user code to operate on a uniform type regardless of strip channel count — simplifying the shader pipeline at the cost of ~33% more colour-buffer RAM for 3-channel strips. LumaWave packs exactly to wire size (3 bytes for GRB), minimising RAM but requiring per-type shader/blend specialisations. The `LW_COLOR_MINIMUM_*` macros can be overridden to reduce padding if memory is critical.

### 3.2  Color Space & Blending

| Feature | LumaWave | LumaWave |
|---------|-------------|----------|
| HSL / HSB | Yes | Yes (`hslToRgb`, `hsbToRgb`) |
| Hue-aware blending | `NeoHueBlend` (shortest, longest, CW, CCW) | `HueBlend` |
| Linear blend | `LinearBlend()`, `BilinearBlend()` per color type | Color math utilities |
| HTML named colours | `HtmlColor` with lookup table | `ColorHexCodec` |

### 3.3  Gamma Correction

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Implementation | `NeoGamma<T_METHOD>` template with pluggable strategies | `GammaShader<TColor>` implementing `IShader` |
| Strategies | Equation (power 2.8), CIE L*a*b*, LUT, dynamic table, null, invert-decorator | LUT-based (uint8_t only) |
| Application point | Per-pixel at `SetPixelColor` / `Show` time (in `LumaWaveLg`) | Per-frame in shader pipeline before protocol encoding |
| Composability | Single gamma method per bus | Composable via `AggregateShader` with other shader stages |

### 3.4  Power / Current Limiting

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Implementation | `NeoRgbCurrentSettings` + `CalcTotalMilliAmpere()` on bus — informational / manual | `CurrentLimiterShader<TColor>` — automatic per-frame budget enforcement |
| Mode | Query-only (user reads mA and decides) | Active scaling (shader dims all pixels to stay within budget) |
| Composability | Standalone calculation | Composable shader stage |

#### Deeper Analysis: Gamma Correction Strategies

LumaWave offers 6 gamma strategies (equation, CIE L*a*b*, static LUT, dynamic LUT, null, invert-decorator), giving users flexibility to trade flash/RAM for precision. The LUT variants store 256 bytes in PROGMEM (flash-resident, zero RAM). LumaWave's `GammaShader` uses a single 256-byte RAM-resident LUT computed at construction. The RAM cost is negligible (256 bytes), but there is no PROGMEM path — a minor disadvantage on AVR-class devices (which LumaWave does not target). LumaWave's composable `AggregateShader` compensates by allowing gamma to chain with current limiting, white balance, and custom shaders in a single pass.

#### Deeper Analysis: Current Limiting

**Accuracy.** Both libraries use the same fundamental model: per-channel milliamp ratings multiplied by per-pixel component values, summed, then divided by 255 (or equivalent scaling). LumaWave's `CalcTotalTenthMilliAmpere()` returns the raw estimate for the user to act on. LumaWave's `CurrentLimiterShader` adds controller overhead and per-pixel standby current to the estimate before comparing against the budget — a more complete model.

**Latency.** `CurrentLimiterShader::apply()` performs a two-pass scan: pass 1 estimates total draw (N × channelCount multiplications), pass 2 scales all pixels if over budget (N × channelCount multiply-divides). For 300 RGB pixels on a 240 MHz ESP32, pass 1 takes ~10–15 µs, pass 2 (when triggered) ~15–20 µs. This adds 10–35 µs to every `show()` call. LumaWave has zero per-frame cost since limiting is manual. The question is whether 10–35 µs matters: for a 300-pixel WS2812B strip, the wire transfer alone takes ~375 µs (9 µs/pixel × 300 / 8 at 800 kHz), so the shader overhead is < 10% of frame time — acceptable for the safety guarantee of never exceeding the power budget.

---

## 4  Protocol / Wire-Encoding Layer

### 4.1  One-Wire NRZ Protocols

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Design | Timing baked into per-platform Speed classes (`NeoEsp32RmtSpeedWs2812x`, etc.) — hundreds of cross-product typedefs | Single `Ws2812xProtocol<TInterfaceColor, TStripColor>` parameterised by `OneWireTiming` — chip selection is a runtime/constexpr timing value |
| Chip aliases | Method typedefs (`NeoWs2812xMethod`, `NeoSk6812Method`, etc.) | Descriptor structs (`Ws2812x`, `Sk6812`, `Apa106`, etc.) inheriting from `Ws2812x<>` with chip-specific timing and channel order |
| Inverted signal | Separate `Inverted` method variants per chip × platform | Protocol-side idle-high handling; dedicated `Tm1814Protocol`/`Tm1914Protocol` |
| Per-pixel settings | Feature-level: settings bytes appended by feature class (`NeoWrgbTm1814Feature`, `NeoRgbSm16803pbFeature`, etc.) | Protocol-level: `Tm1814Protocol` and `Tm1914Protocol` handle settings preambles; `Sm168xProtocol` handles gain settings |
| Timing data | Spread across per-platform Speed class constants | Centralised `OneWireTiming` structs with `constexpr` nanosecond values |

#### Deeper Analysis: Typedef Explosion vs Parameterised Protocol

**LumaWave typedef scale.** The LumaWave codebase contains **1,285 unique Method typedef names** (1,653 total lines including platform `#ifdef` duplicates) and **149 Speed class definitions** across 11 platform method files. The combinatorial explosion arises because every `{chip × platform × variant (normal/inverted) × channel order}` combination requires a distinct typedef. For the WS2812x family alone, there are ~306 typedef lines spanning 15 files.

**Cost of adding one new one-wire chip in LumaWave.** Using WS2812x as the reference point, adding a single new chip requires:
- **15 files touched:** NeoEsp32RmtMethod.h, NeoEsp32I2sMethod.h, NeoEsp32I2sXMethod.h, NeoEsp32LcdXMethod.h, NeoEsp8266DmaMethod.h, NeoEsp8266UartMethod.h, NeoEspBitBangMethod.h, NeoArmMethod.h, NeoAvrMethod.h, NeoNrf52xMethod.h, NeoRp2040x4Method.h, NeoBits.h, NeoRp2040PioSpeed.h, XMethods.h, LumaWaveLg.h.
- **~14 new Speed classes:** one per platform variant, plus inverted variants.
- **~215 new Method typedefs.**
- No automated validation — the new typedefs compile only if manually tested per-platform.

**Cost of adding one new one-wire chip in LumaWave.** Adding a chip with identical timing to an existing family (e.g., a WS2812B clone):
- **1 file touched:** `ProtocolDescriptors.h` — add a `struct` alias inheriting from `Ws2812x<>` with the chip-specific `OneWireTiming` and channel order.
- **0 new protocol code** — `Ws2812xProtocol` handles all one-wire chips generically.
- **0 transport changes** — transport selection is orthogonal.
- **~10 lines of code** (struct definition + timing values + token strings).
- Compilation verified automatically by `test_factory_descriptor_first_pass_compile`.

For a chip with novel timing (not a clone), LumaWave requires adding a `constexpr OneWireTiming` entry in the timing namespace (~5 lines) in addition to the descriptor alias. Total: ~15 lines, 2 files.

**Binary size impact.** Each LumaWave Method typedef that is instantiated generates a full class with its own `Begin()`, `Update()`, `Show()`, and buffer management. In practice users instantiate 1–3 bus types, so the dead-code elimination pass removes unused typedefs — but the *available* typedef surface forces the compiler to parse all 1,653 lines in every translation unit that includes `LumaWave.h`. LumaWave's protocol descriptors are lightweight `constexpr` structs with no code generation until the factory instantiates them — the 554-line `ProtocolDescriptors.h` is cheaper to parse than any single LumaWave platform method file.

#### Deeper Analysis: SPI/Clocked Protocol Extensibility

**LumaWave SPI protocol pattern.** Two-wire (clocked) chips in LumaWave share `DotStarMethodBase<T_TWOWIRE>` as a common method, with Feature classes handling byte encoding. Adding a new clocked chip (e.g., a hypothetical new SPI LED) requires:
- A new Feature class defining `applyPixelColor`, `ColorObject`, and `PixelSize` — typically 50–100 lines.
- Potentially new TwoWire implementation typedefs if the chip needs non-standard SPI parameters.
- Feature/Method cross-product typedefs — ~20–40 new lines.

**LumaWave SPI protocol pattern.** Each clocked chip has a dedicated `IProtocol` implementation (`Apa102Protocol`, `Hd108Protocol`, `Lpd6803Protocol`, etc.). Adding a new clocked chip requires:
- A new `XxxProtocol<TInterfaceColor, TStripColor>` implementing `update()` and `requiredBytes()` — typically 80–150 lines.
- A descriptor alias in `ProtocolDescriptors.h` — ~15 lines.
- A trait specialisation in `ProtocolDescriptorTraits.Xxx.h` if non-default settings are needed — ~30 lines.

The LumaWave approach requires more per-chip protocol code (each chip owns its framing logic) but gains: (a) no cross-product typedef explosion, (b) independent testability (each protocol can be unit-tested with `NilTransport`), and (c) clear ownership of chip-specific framing (no implicit coupling between HD108 and DotStar's start-frame logic).

### 4.2  SPI / Clocked Protocols

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| APA102/DotStar | `DotStarMethodBase<T_TWOWIRE>` + `DotStarXxxFeature` | `Apa102Protocol<TInterfaceColor, TStripColor>` + any `TransportTag` transport |
| HD108 | Feature aliases on DotStar code | Dedicated `Hd108Protocol` |
| LPD6803 | Feature class + `DotStarMethodBase` reuse | Dedicated `Lpd6803Protocol` |
| LPD8806 | Feature class + `DotStarMethodBase` reuse | Dedicated `Lpd8806Protocol` |
| WS2801 | `Ws2801MethodBase<T_TWOWIRE>` | Dedicated `Ws2801Protocol` |
| P9813 | Feature class + method | Dedicated `P9813Protocol` |
| SM16716 | Feature class + bitbang method | Dedicated `Sm16716Protocol` |
| SM168x (SPI clocked) | One-wire feature variants only | Dedicated `Sm168xProtocol` |
| TLC5947 | Feature class + method | Unsupported (external latch/OE sequencing not modeled in `ITransport`; TLC5947 headers are intentionally not shipped/exposed) |
| TLC59711 | Feature class + method | Dedicated `Tlc59711Protocol` |

#### Deeper Analysis: SPI/Clocked Protocol Architecture

**LumaWave: nine independent copy-paste MethodBase classes.** Every clocked protocol in LumaWave has its own standalone MethodBase class — `DotStarMethodBase`, `Hd108MethodBase`, `Ws2801MethodBase`, `Lpd6803MethodBase`, `Lpd8806MethodBase`, `P9813MethodBase`, `Sm16716MethodBase`, `Tlc5947MethodBase`, `Tlc59711MethodBase`. These 9 files total 1,562 lines with near-identical boilerplate: each contains its own `getData()`, `getDataSize()`, `AlwaysUpdate()`, `SwapBuffers()`, `applySettings()`, `Initialize()`, destructor, and `malloc`/`free` buffer management — differing only in `Update()` which writes framing bytes and calls `transmitBytes()`. Despite initially appearing to share `DotStarMethodBase`, HD108, LPD6803, LPD8806, P9813, and related protocols each have their own complete copy. Zero code is shared between them.

**LumaWave: Feature class explosion.** In addition to the 9 MethodBase files, LumaWave requires 13 Feature class files (1,323 lines) containing 30+ classes to handle pixel encoding. DotStar alone has 20 Feature classes covering 6 byte orders × 8-bit/16-bit × with/without luminance. The Feature/Method separation creates a cross-product: 152 typedefs are needed just for clocked protocols (47 for DotStar, 32 for HD108, 28 for TLC59711, 9 each for WS2801/LPD6803/LPD8806/P9813, 8 for TLC5947, 1 for SM16716). Combined, the LumaWave clocked protocol surface is ~2,885 lines of method+feature code plus 152 typedefs.

**LumaWave: 10 self-contained protocol classes in 9 files.** Each LumaWave clocked protocol implements `IProtocol<TColor>` with its own framing logic, serialisation, and `requiredBytes()` calculation — totalling 2,051 lines. There are no Feature classes: channel ordering is handled at runtime via a `channelOrder` config string, and bit-depth conversion uses shared `toWireComponent8()`/`toStripComponent()` converters inside each protocol's `update()`. There are zero typedefs — transport selection is runtime configuration, not a type parameter.

**Framing correctness isolation.** Each protocol owns its complete framing logic:
- APA102: 4-byte start `0x00` + pixel data with `0xFF` brightness leader + 4-byte reset + `ceil(N/16)` end bytes.
- HD108: 16-byte start `0x00` + pixel data with `0xFFFF` brightness leader + 4-byte `0xFF` end.
- LPD6803: 4-byte start `0x00` + 5-5-5 packed 2 bytes/pixel + `ceil(N/8)` end bytes.
- LPD8806: `ceil(N/32)` start + 7-bit-per-channel `(val >> 1) | 0x80` + `ceil(N/32)` end.
- P9813: 4-byte `0x00` start + checksum header + BGR + 4-byte `0x00` end.
- SM16716: 50-bit zero preamble + 1-bit separator per pixel (non-byte-aligned, bit-packed).
- TLC5947: currently unsupported pending latch/OE transport-contract support; related headers are intentionally not shipped/exposed.
- TLC59711: 4-byte header per chip (write cmd + control + brightness control) + reversed BGR big-endian 16-bit.

In LumaWave, framing is spread between the Feature class (pixel encoding) and the MethodBase (start/end frames), making it harder to verify correctness for a specific chip without reading two files. In LumaWave, a single `update()` method contains the complete wire format — reviewable and testable in isolation.

**Buffer allocation comparison.** Every LumaWave MethodBase calls `malloc(_sizeData)` in its constructor and `free(_data)` in its destructor — raw allocation with no RAII wrapper. Each of the 9 MethodBase classes repeats this pattern independently. LumaWave protocols receive an externally-owned `span<uint8_t>` via `setBuffer()` and never allocate. Buffer lifetime is managed by the bus's unified arena and accessor context, decoupling allocation from protocol logic.

| Metric | LumaWave | LumaWave |
|--------|-------------|----------|
| Method/protocol files | 9 (1,562 lines) | 9 files / 10 protocols (2,051 lines) |
| Feature class files | 13 (1,323 lines) | 0 (integrated) |
| Typedefs (clocked only) | 152 | 0 |
| Combined code surface | ~2,885 lines + 152 typedefs | 2,051 lines |
| Independent base classes | 9 (copy-paste boilerplate) | 0 (all implement `IProtocol<T>`) |
| Buffer allocation | Per-method `malloc`/`free` | External `span<uint8_t>` |
| Transport binding | Compile-time `T_TWOWIRE` template | Runtime `ITransport*` |
| Channel order handling | Per-feature-class | Runtime `channelOrder` config |

### 4.3  Protocol Interface Contracts

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Interface | No formal interface — Feature and Method are duck-typed templates | `IProtocol<TColor>` ABC with `begin()`, `update()`, `setBuffer()`, `bindTransport()` |
| Transport binding | Method owns buffer + hardware — no separate binding | `bindTransport(ITransport*)` decouples protocol from transport |
| Buffer management | Feature writes directly into method's buffer via static call | Protocol owns byte buffer view; writes into it during `update()` |
| Compile-time validation | Template instantiation errors (often cryptic) | SFINAE trait checks (`ProtocolType<T>`, `ProtocolTransportCompatible<P, T>`) with category tag system |

---

## 5  Transport / Hardware Output Layer

### 5.1  Platform Support Matrix

| Platform | LumaWave Methods | LumaWave Transports |
|----------|--------------------|--------------------|
| **ESP32** | RMT (8 ch), I2S DMA, LCD parallel, bit-bang, DMA SPI | `Esp32RmtTransport`, `Esp32I2sTransport`, `Esp32DmaSpiTransport` |
| **ESP32-S2** | RMT (4 ch), I2S (bus 0) | (via ESP32 transports) |
| **ESP32-S3** | RMT (4 ch), LCD (8/16 parallel) | (via ESP32 transports) |
| **ESP32-C3** | RMT (2 ch), bit-bang | (via ESP32 transports) |
| **ESP8266** | I2S DMA, UART DMA, bit-bang, DMX512 I2S | `Esp8266DmaI2sTransport`, `Esp8266DmaUartTransport` |
| **RP2040** | PIO+DMA ×4 state machines | `RpPioTransport`, `RpSpiTransport`, `RpUartTransport` |
| **NRF52840** | PWM DMA (3-4 ch) | **Not supported** |
| **ARM (Teensy, SAMD, STM32)** | ASM bit-bang | **Not supported** |
| **AVR** | Cycle-counted ASM | **Not supported** |
| **MegaAVR** | Same as AVR | **Not supported** |
| **Native (host)** | None | `NilTransport`, `PrintTransport` (testing) |

### 5.2  Transport Architecture

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Design pattern | Monolithic method class owns buffer + DMA + timing | `ITransport` interface — thin transfer contract; buffer owned by bus/protocol layer |
| One-wire encoding location | Inside each platform method class | Performed inside one-wire protocol `update()` via shared `OneWireEncoding`; encoded output is emitted in the protocol frame slice passed to `transmitBytes()` |
| SPI abstraction | `TwoWireSpiImple<SpiSpeed>` / `TwoWireBitBangImple` — per-speed template | `SpiTransport` (generic Arduino SPI) or platform-specific (`RpSpiTransport`, `Esp32DmaSpiTransport`) |
| Parallel output | ESP32 I2S/LCD 8/16 ch, RP2040 PIO ×4 | Not currently supported in LumaWave's transport contract |
| Double buffering | Method-managed (`_dataEditing` / `_dataSending` + `std::swap`) | Shader-scratch pipeline in `PixelBus` (optional scratch + protocol frame buffer) |

#### Deeper Analysis: Transport Architecture

**DMA completion / async readiness.** Both libraries gate `show()` on a readiness check (`IsReadyToUpdate()` / `isReadyToUpdate()`). LumaWave checks inside the method class — e.g., ESP32 RMT queries `rmt_wait_tx_done()` status, RP2040 PIO checks a DMA completion flag. LumaWave checks at `ITransport::isReadyToUpdate()`, which calls through to the same platform API. The abstraction adds one virtual dispatch but the underlying mechanism is identical. Both libraries skip work on unready transports — LumaWave at the `Show()` level, LumaWave per-strand (enabling partial updates when some strands are ready and others are still transmitting).

**Parallel multi-strand output.** LumaWave supports parallel output on three platforms: ESP32 I2S (8 channels), ESP32-S3 LCD (8/16 channels), and RP2040 PIO (4 state machines). These methods interleave multiple strands' data into a single DMA buffer, exploiting hardware-level parallelism. LumaWave's `ITransport` contract is currently single-strand (`transmitBytes()` sends one buffer). Supporting parallel output would require either: (a) a multi-strand transport that pre-interleaves data before DMA launch, or (b) a compositor layer above the transport that merges multiple protocol outputs into one interleaved buffer. This is the largest transport capability gap.

**Double-buffer memory trade-off.** LumaWave double-buffers on async platforms (ESP32 RMT, RP2040 PIO) by allocating two full pixel-data buffers and swapping pointers after DMA launch — costing 2× pixel data. `maintainBufferConsistency` optionally memcpys from editing to sending, adding further CPU cost. LumaWave does not separately double-buffer at the transport level. Instead, the shader scratch buffer serves a similar role: the root buffer is never DMA'd directly — the protocol reads from shader-output or root into its own byte buffer, which is then DMA'd. The root buffer is safe to mutate immediately after `show()` returns (the protocol byte buffer is what's in flight). This means LumaWave's "double buffering" is structural rather than explicit, and its cost (shader buffer) is already accounted for.

### 5.3  One-Wire Timing Model

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Timing representation | Per-Speed-class constants (RMT ticks, I2S patterns, ASM cycle counts) — differ by platform | Platform-independent `OneWireTiming` struct (T0H/T0L/T1H/T1L/reset in nanoseconds) |
| Adding a new chip | Define new Speed class per target platform | Add a `constexpr OneWireTiming` value + descriptor alias |
| Encoding cadence | Baked into bit-encode functions (3-step or 4-step patterns) | Configurable via `cadence` field (3-step or 4-step) |
| Bitrate derivation | Manual per-Speed-class constants | `encodedDataRateHz()` computed from timing values |

---

## 6  Bus Composition & Topology

### 6.1  Multi-Strand Composition

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Multi-strand | Separate `LumaWave` instances; user manages | Compose multiple bus instances at the app layer |
| Composite bus construction | Manual | `makeBus(bus1, bus2, ...)` — merges strands via move semantics |
| Dynamic bus construction | Not supported | `DynamicBusBuilder` + optional INI-driven configuration |

### 6.2  2D Topology / Panel Layout

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Design | Template-based `NeoTopology<T_LAYOUT>`, `NeoTiles<T_MATRIX, T_TILE>`, `NeoMosaic` — compile-time layout | `Topology` class with `TopologySettings` — runtime-configurable layout, tiling, mosaic rotation |
| Layout strategies | `RowMajorLayout`, `ColumnMajorLayout`, + alternating + rotated variants (12+ classes) | `layout` enum (row/column major, progressive/serpentine) + `mosaicRotation` |
| Tiling | `NeoTiles` template with separate tile + matrix layout | Built into `Topology` via `tilesWide`, `tilesHigh`, `tileLayout` |
| Configuration | Compile-time only | Runtime configurable, including via INI parser |

---

## 7  Factory / Configuration System

### 7.1  Static Bus Construction

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| User-facing API | Direct template instantiation: `LumaWave<Feature, Method> strip(count, pin)` | `makeBus<ProtocolDesc, TransportDesc>(count, config)` factory function |
| Type resolution | User selects Feature + Method from hundreds of typedefs | Descriptor + traits system resolves types; `Bus<Proto, Trans>` type alias |
| Configuration | Constructor parameters (count, pin) | Structured config objects (`SpiSettings`, `RpPioSettings`, etc.) |

### 7.2  Dynamic / Runtime Configuration

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Runtime bus creation | **Not supported** | `DynamicBusBuilder` — type-erased recipe graph with cycle detection |
| INI-driven config | **Not supported** | `BuildDynamicBusBuilderFromIni()` — declarative bus graph from config file |
| Plugin / descriptor registry | **Not supported** | Descriptor traits with `Tokens` arrays for runtime name→type resolution |

---

## 8  Additional Features

### 8.1  Animation System

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Built-in animations | `NeoPixelAnimator` — slot-based engine with 20+ easing curves | **Not provided** — considered out of scope for the core library |
| Easing functions | `NeoEase` (linear, quadratic, cubic, quartic, quintic, sinusoidal, exponential, circular, gamma) | None |

### 8.2  Bitmap / Sprite Support

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Device-independent bitmap | `NeoDib<T_COLOR_OBJECT>` | Not provided |
| 2D pixel buffer | `NeoBuffer<T_BUFFER_METHOD>` | Not provided |
| PROGMEM bitmap | `NeoBufferProgmemMethod` | Not provided |
| Sprite sheets | `NeoVerticalSpriteSheet`, `NeoBitmapFile` | Not provided |

### 8.3  Specialised Bus Wrappers

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Brightness bus | `NeoPixelBrightnessBus` (deprecated) / `LumaWaveLg` | Shader pipeline handles brightness |
| 7-Segment display | `NeoPixelSegmentBus` + segment features | Not provided |

---

## 9  Projected Performance Analysis

### 9.1  Per-Frame CPU Cost

| Phase | LumaWave | LumaWave | Analysis Direction |
|-------|-------------|----------|--------------------|
| Pixel write (`SetPixelColor`) | Feature static method — inline, zero overhead | Direct `operator[]` on `RgbBasedColor` span — inline, zero overhead | Parity expected |
| Gamma / brightness | `LumaWaveLg`: applied per-pixel at `SetPixelColor` (eagerly) | `GammaShader`: applied per-frame over full span (lazily, batch) | LumaWave batch may be more cache-friendly; LumaWave avoids double write |
| Current limiting | Manual query — no per-frame cost | `CurrentLimiterShader`: scans full buffer each frame | LumaWave has non-trivial per-frame cost; amortised over frame |
| Wire encoding | Feature `applyPixelColor` writes wire bytes at `SetPixelColor`; `Show` transmits directly | Protocol `update()` converts typed span → byte buffer at `show()` time | LumaWave eager encoding avoids a separate pass; LumaWave defers encoding to `show()` — single sequential pass, better for DMA pipelining |
| Transport / DMA | Method `Update()` launches DMA or blocks on bitbang | Transport `beginTransaction()` / `transmitBytes()` / `endTransaction()` | Both ultimately call the same platform DMA API; LumaWave has one extra virtual dispatch per strand |

#### Deeper Analysis: Per-Frame CPU Cost

**Eager vs deferred encoding.** LumaWave writes wire-format bytes at `SetPixelColor` time (the Feature's `applyPixelColor` shuffles channels and writes directly into the method's buffer). This means `Show()` does almost no CPU work — it just kicks DMA. LumaWave defers all encoding to `show()`: the protocol's `update()` reads the typed colour span and serialises to wire bytes in a single sequential pass. For `Ws2812xProtocol`, this is a tight loop: for each pixel, read N channels via `_channelOrder[]` lookup and write N bytes — approximately 3 loads + 3 stores per RGB pixel. On a 240 MHz ESP32, serialising 300 GRB pixels takes ~5–10 µs. The trade-off: LumaWave pays the encoding cost incrementally (spread across `SetPixelColor` calls), LumaWave pays it in one burst at `show()`. The burst approach has better cache locality (sequential read of colour span, sequential write of byte buffer) and avoids polluting the cache during user pixel-manipulation code.

**Virtual dispatch overhead — measured estimate.** LumaWave's `show()` makes the following virtual calls per strand: `protocol->isReadyToUpdate()`, `shader->apply()` (if shader present), `protocol->update()`, and internally `transport->beginTransaction()` + `transmitBytes()` + `endTransaction()`. That's 3–6 indirect calls per strand. On an ARM Cortex-M0+ (RP2040 at 133 MHz), an indirect call through a vtable costs ~10–20 ns including branch prediction miss. Total: ~60–120 ns per strand. For a 300-pixel strand where wire transfer takes 375 µs, virtual dispatch is 0.02–0.03% of frame time.

**Cache utilisation.** LumaWave's eager encoding interleaves user logic with buffer writes — the cache holds both user data structures and the pixel buffer simultaneously. LumaWave's `show()` processes buffers in sequential passes: (1) memcpy root→shader buffer, (2) shader `apply()` over shader buffer, (3) `update()` reads shader buffer → writes protocol buffer. Each pass is a linear sweep — highly cache-friendly, especially for L1 data caches (typically 16–64 KB on ESP32/RP2040). For a 300 RGB-pixel bus: root buffer = 1.2 KB, shader buffer = 1.2 KB, protocol buffer = 900 B — all fit in L1 on any target platform.

**`OneWireEncoding` software encode — universal path.** All LumaWave one-wire transports use `OneWireEncoding` for NRZ encoding. Both `Esp32RmtTransport` and `RpPioTransport` are clock+data (`TransportTag`) transports — neither performs hardware-level one-wire encoding internally. The shared encoding step runs inside protocol `update()`: each input byte becomes 3 output bytes (3-step) or 4 output bytes (4-step), written into the protocol frame buffer (typically the bus-provided protocol slice; owned fallback storage is only used when no adequate external slice is supplied). For 300 GRB pixels (900 wire bytes), this produces 2,700 encoded bytes (3-step) and takes ~15–25 µs on a 240 MHz ESP32. By contrast, LumaWave's ESP32 RMT path uses a streaming translate callback — the RMT peripheral pulls encoded symbols on-demand without a pre-allocated encoding buffer — and RP2040 PIO reads raw bytes and generates timing in hardware. Both LumaWave paths achieve zero-copy encoding. The trade-off: LumaWave pays the software encode cost on every platform in exchange for a single, portable, testable encoding path that works identically across SPI, I2S, UART, RMT, and PIO transports. LumaWave avoids the encoding buffer but requires each platform method to implement its own encoding strategy.

### 9.2  Show / Transmit Latency

| Aspect | LumaWave | LumaWave | Analysis Direction |
|--------|-------------|----------|--------------------|
| Pre-`show()` work | Minimal — buffer already wire-encoded | Protocol `update()` pass + shader pass(es) | LumaWave has lower `show()` latency; LumaWave work is shifted to `show()` |
| DMA handoff | Method-specific; double-buffer swap on async platforms | Transport `transmitBytes()` | Same hardware path |
| `isReadyToUpdate()` | Method-level check (RMT done, DMA idle) | Transport-level check | Same |
| Inter-frame wait | Dirty flag skip | Dirty flag skip + `alwaysUpdate()` per strand | Equivalent |

#### Deeper Analysis: `show()` Latency Budget

For a concrete comparison, consider a 300-pixel WS2812B GRB strip on ESP32 (240 MHz), with gamma + current-limiter shaders enabled in LumaWave:

| Phase | LumaWave (ESP32 RMT) | LumaWave (ESP32 RMT via OneWireEncoding) |
|-------|------------------------|---------------------------------|
| Pre-show encoding | 0 µs (already done at `SetPixelColor`) | ~5–10 µs (protocol `serialize()`: 300 × 3 channel reorders + byte writes) |
| Shader memcpy | N/A | ~2–3 µs (1.2 KB memcpy root → shader buffer) |
| Gamma shader | ~0.3 µs amortised (applied per-pixel at set time via LUT) | ~2–3 µs (300 × 3 LUT lookups over shader buffer) |
| Current limiter pass 1 | 0 µs (manual only) | ~10–15 µs (300 × 3 multiply-accumulate) |
| Current limiter pass 2 | 0 µs | 0–15 µs (only when over budget) |
| OneWireEncoding encode | N/A (RMT streams on-the-fly) | ~15–25 µs (900 wire bytes → 2,700 encoded bytes, 3-step) |
| Double-buffer swap | ~0.1 µs (pointer swap) | N/A (protocol buffer is the transmit source) |
| DMA launch | ~5–10 µs (RMT setup + `rmt_write_sample`) | ~5–10 µs (same RMT API) |
| **Total pre-DMA** | **~5–10 µs** | **~40–80 µs** |
| Wire transfer time | ~375 µs | ~375 µs |
| **Show-to-show minimum** | **~380–385 µs** | **~415–455 µs** |

LumaWave's `show()` adds 35–70 µs of CPU work over LumaWave for the shader + protocol encoding + `OneWireEncoding` encode passes. This is 9–19% overhead relative to wire time — meaningful for latency-critical applications but negligible for typical LED animation frame rates (30–120 fps). The overhead scales linearly with pixel count; for very long strips (1,000+ pixels), wire transfer dominates and the relative overhead drops below 5%.

### 9.3  Interrupt / Timing Sensitivity

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Bit-bang methods (AVR/ARM) | Critical-section with interrupts disabled | N/A — no AVR/ARM support |
| DMA/RMT platforms | Non-blocking after DMA launch | Same |
| One-wire encode | In hardware (RMT/PIO) or DMA buffer (I2S) | `OneWireEncoding` software encode into byte buffer → DMA — all one-wire transports (RMT, PIO, SPI, I2S, UART) use the same path |

#### Deeper Analysis: OneWireEncoding Encode Overhead

All LumaWave one-wire transports route through `OneWireEncoding`. Both `Esp32RmtTransport` and `RpPioTransport` are clock+data transports (`TransportTag`) — the shared path performs the NRZ bit-level encode before handing the encoded protocol frame to the underlying transport. For any transport (RMT, PIO, SPI, I2S, UART), `OneWireEncoding` performs the same encoding pass: each input byte becomes 3 bytes (3-step) or 4 bytes (4-step). The inner loop shifts bits MSB-first and packs encoded patterns into output bytes. For 900 wire bytes (300 GRB pixels):

- 3-step: 900 × 8 × 3 = 21,600 encoded bits → 2,700 bytes output, ~15–25 µs on ESP32
- 4-step: 900 × 8 × 4 = 28,800 encoded bits → 3,600 bytes output, ~20–35 µs on ESP32

LumaWave's I2S DMA path performs an equivalent software encode (`FillBuffers()`) with the same 3× or 4× expansion — so the cost is comparable when both libraries use the I2S/SPI path. LumaWave's RMT path streams encoding on-the-fly via a translate callback (zero pre-allocation), and RP2040 PIO generates timing directly from raw bytes in hardware. LumaWave's uniform `OneWireEncoding` approach pre-encodes the full buffer regardless of transport, paying the 3–4× memory expansion universally. This is a deliberate architectural choice: one portable encoding path, fully testable on the host, at the cost of the encoding buffer that hardware-native approaches avoid.

---

## 10  Projected Memory Usage Analysis

### 10.1  Per-Pixel RAM

The original draft understated LumaWave's memory usage by omitting method-internal encoding buffers. LumaWave methods allocate their own buffers for DMA encoding, double-buffering, and idle/reset padding. The following table includes these hidden costs.

#### LumaWave: Per-Method Buffer Breakdown (300 GRB pixels, 900 wire bytes)

| Method | Pixel buf(s) | Encoding buf | Other | Total bytes | Per-pixel |
|--------|-------------|-------------|-------|-------------|----------|
| **AVR / ARM bit-bang** | 1 × 900 | — | — | 900 | **3.0** |
| **ESP bit-bang** | 1 × 900 | — | — | 900 | **3.0** |
| **DotStar SPI** | 1 × 900 | — | tiny end-frame | ~920 | **~3.1** |
| **ESP8266 UART (sync)** | 1 × 900 | — (HW UART encodes) | — | 900 | **3.0** |
| **ESP8266 UART (async)** | 2 × 900 | — (HW UART encodes) | — | 1,800 | **6.0** |
| **ESP32 RMT** | 2 × 900 | — (streamed via translate callback) | ~256 RMT FIFO (HW) | 1,800 | **6.0** |
| **RP2040 PIO** | 2 × 900 | — (PIO HW encodes from raw bytes) | PIO FIFO (HW) | 1,800 | **6.0** |
| **ESP32 I2S DMA** | 1 × 900 | 1 × 2,700 (3-step) or 3,600 (4-step) — DMA-capable heap | reset padding | 3,600–4,500+ | **12.0–15.0** |
| **ESP8266 I2S DMA** | 1 × 900 | 1 × 2,700 (3-step) or 3,600 (4-step) | idle buf (≤256) + DMA descriptors | 3,600–4,800+ | **12.0–16.0** |

#### LumaWave: Per-Configuration Buffer Breakdown (300 GRB pixels, default RGBW-8 internal)

| Configuration | Root buf | Shader buf | Protocol buf | Total bytes | Per-pixel |
|---------------|----------|------------|-------------|-------------|----------|
| **No shader, WS2812x (encoded in protocol buffer)** | 300 × 4 = 1,200 | — | 2,700 + reset | 3,900+ | **~13.0** |
| **With shader, WS2812x (encoded in protocol buffer)** | 1,200 | 1,200 | 2,700 + reset | 5,100+ | **~17.0** |
| **No shader, SPI/DotStar class** | 1,200 | — | ~900 + framing | ~2,100+ | **~7.0** |
| **With shader, SPI/DotStar class** | 1,200 | 1,200 | ~900 + framing | ~3,300+ | **~11.0** |

#### Apples-to-Apples Comparison (same platform, same function)

| Scenario | LumaWave | LumaWave | Delta |
|----------|-------------|----------|-------|
| **ESP32 RMT, no gamma/limiting** | 6.0 B/px (2 × pixel buf) | ~13.0 B/px (root + encoded protocol buffer) | ~+7.0 B/px |
| **ESP32 RMT, with gamma** | 6.0 B/px (gamma applied in-place) | ~17.0 B/px (+ shader buf) | ~+11.0 B/px |
| **ESP32 I2S, no gamma** | 12.0–15.0 B/px (pixel + DMA encode) | ~13.0 B/px (WS2812x path) | ~-2.0 to +1.0 B/px |
| **RP2040 PIO, with gamma** | 6.0 B/px | ~17.0 B/px | ~+11.0 B/px |
| **SPI/DotStar (no shader)** | ~3.1 B/px | ~7.0 B/px | ~+3.9 B/px |

**Key takeaway:** After the buffer-management cleanup, LumaWave one-wire paths no longer account for a separate "protocol + OneWireEncoding" double-buffer at the comparison level; encoded wire bytes live in the protocol slice directly. The remaining gap on RMT/PIO is primarily from (a) RGBW-8 internal upsizing (+1 B/px for 3-channel strips) and (b) optional shader scratch (+4 B/px when enabled), while LumaWave still benefits from tightly method-coupled storage in its low-RAM paths. On I2S-like paths where both systems carry expanded/transmittable byte payloads, the delta is materially smaller and can be near parity depending on shader usage.

#### Feasibility of Opt-In Colour Buffer Reduction

Overriding `LW_COLOR_MINIMUM_COMPONENT_COUNT=3` and `LW_COLOR_MINIMUM_COMPONENT_SIZE=8` eliminates the +1 byte/pixel upsizing overhead for 3-channel strips, reducing both root and shader regions. On one-wire paths this moves rough totals from ~13.0 → ~12.0 B/px (no shader) and ~17.0 → ~15.0 B/px (with shader). This opt-in is already supported via preprocessor macros but changes `DefaultColorType` globally, which affects shader compatibility (some shaders assume ≥4 channels).

### 10.2  Static / Flash Usage

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Template bloat risk | High — each Feature×Method×Speed×Channel is a distinct type; hundreds of possible instantiations, but user typically instantiates 1-3 | Moderate — single `Ws2812xProtocol` template handles all one-wire chips; virtual dispatch avoids type explosion |
| Gamma LUT | 256 bytes (PROGMEM) per `NeoGammaTableMethod` | 256 bytes per `GammaShader` (RAM, not PROGMEM) |
| Code size per chip | Small — Speed class is just a few constants | Small — `OneWireTiming` constexpr + descriptor struct |
| Factory subsystem overhead | None | ~8,400 header LOC in `src/factory/**` when enabled, including ~4,800 LOC from dynamic/INI path (`DynamicBusBuilder`, `BuildDynamicBusBuilderFromIni`, parser/reader); template-heavy but opt-out via `LW_FACTORY_SYSTEM_DISABLED` |

#### Deeper Analysis: Why the Factory Feels "Compile-Heavy"

The factory cost is not one single template, but three additive sources that all happen in headers:

1. **Umbrella include fan-out (parse cost).** `LumaWave.h` includes `factory/Factory.h` unless `LW_FACTORY_SYSTEM_DISABLED` is set. `Factory.h` then pulls in static make APIs, dynamic builder, INI parser/reader, descriptors, and traits. Even translation units that only call `makeBus(...)` still parse the dynamic/INI machinery because it is included by the umbrella header.

2. **Descriptor/traits matrix (template resolution cost).** Descriptor headers and trait specialisations form a compile-time matrix (`ProtocolDescriptorTraits.*`, `TransportDescriptorTraits.*`, `ShaderDescriptorTraits.*`, plus resolver helpers). This is where most compile-time type checking happens (`ProtocolType`, compatibility checks, settings resolution). Across `src/factory`, there are ~248 `template<...>` declarations and ~100 `constexpr` sites; individually small, but collectively they increase substitution and overload-resolution work.

3. **Instantiation cross-product at call sites (codegen cost).** Each unique `makeBus<ProtocolDesc, TransportDesc, ...>` combination instantiates a distinct construction path (descriptor mapping, settings adaptation, compatibility checks, bus alias resolution). In typical sketches this remains modest, but projects that instantiate many unique protocol/transport/shader combinations can grow compile time and object size noticeably.

`DynamicBusBuilder` itself is mostly runtime-oriented logic, but because it is header-defined it still contributes to front-end compile time (token parsing code, recipe graph helpers, error enums/formatting paths) even when a file does not execute dynamic building.

#### Clarification: Binary Size vs Compile-Time Cost

For the factory subsystem, the **primary concern is compile time and translation-unit parse/instantiation load**, not automatically massive runtime binaries.

- **Static factories (`makeBus<...>`)** mostly add compile-time work (template resolution + instantiation). Runtime binary growth is generally limited to the concrete protocol/transport/shader combinations actually instantiated and called.
- **Descriptor/trait catalogs** are largely type metadata (`struct` aliases, trait specialisations, `constexpr` tokens). They are expensive to parse/resolve but usually contribute little direct runtime code unless specific paths are instantiated.
- **Dynamic/INI factory path** (`DynamicBusBuilder`, parser/reader) can add noticeable flash when used, because parsing/error/reporting logic is real runtime code. If these APIs are never referenced, linker dead-stripping typically removes most or all of that code.

So the practical split is: **static factory usage = mostly front-end compile cost**, while **dynamic/INI usage = compile cost plus potential runtime flash cost**.

#### Mitigation Options (Current State)

For users who care about build time, the practical levers are straightforward:

- **Disable factory globally when not needed.** Define `LW_FACTORY_SYSTEM_DISABLED` before including `LumaWave.h` (or at build-system level) to skip the `factory/Factory.h` include fan-out entirely.
- **Narrow include surface in application code.** Prefer including only the specific module headers needed by a translation unit instead of always including the umbrella `LumaWave.h`, especially in large projects with many `.cpp` files.
- **Limit unique factory instantiations.** Reuse the same protocol/transport/shader descriptor combinations where possible; each distinct `makeBus<...>` signature creates additional template instantiation/codegen work.
- **Centralize dynamic/INI creation code.** Keep dynamic-builder/INI parsing usage in a small number of translation units so the heaviest factory headers are not parsed project-wide.

These do not change runtime architecture; they only reduce front-end compile pressure and incremental rebuild cost.

### 10.3  Heap Fragmentation

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Allocation count per bus | 1-2 (pixel buffer + optional DMA buffer) via `malloc` | Vector-backed root/shader/protocol buffers owned by `PixelBus` |
| Allocation strategy | Raw `malloc`/`free`; no mitigation | RAII-managed standard containers in bus internals |
| Dynamic bus builder | N/A | Additional `unique_ptr` + `vector` allocations for recipes and strand extents |
| Deallocation | Manual `free` in method destructor | Deterministic release via C++ object lifetime (bus-owned containers) |

---

## 11  Documentation & Developer Experience

### 11.1  API Surface

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Primary include | `#include <LumaWave.h>` | `#include <LumaWave.h>` |
| Namespace | Global (no namespace) | `lw` (with re-exports for unqualified usage in examples) |
| Discovery | User must know Feature + Method typedef names | Descriptor aliases + `PlatformDefault` transport |
| Error messages | Template instantiation failures (often deep, cryptic) | SFINAE-guarded trait checks; tag-based category mismatch |

### 11.2  Examples

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Example count | 13+ categories with multiple sketches | Platform smoke tests + platform-debug examples |
| Scope | Full applications (animations, bitmaps, topologies, DotStar, Pixie, etc.) | Layer-declaration focused (protocol + transport + bus) |

### 11.3  Internal Documentation

| Aspect | LumaWave | LumaWave |
|--------|-------------|----------|
| Architecture docs | README + wiki | Extensive `docs/internal/` (contracts, timing reference, factory goals, testing plans, migration plan) |
| Test specifications | None | Hierarchical spec documents driving test naming |

---

## 12  Summary of Key Trade-offs

| Dimension | LumaWave Advantage | LumaWave Advantage |
|-----------|----------------------|-------------------|
| Platform breadth | AVR, ARM (SAMD/STM32/Teensy), NRF52840, ESP32-C3/C6/H2 | — |
| Zero-overhead dispatch | Pure compile-time — no virtual calls | — |
| Eager encoding | Wire bytes ready before `show()` | — |
| Animation / bitmap | Built-in `NeoPixelAnimator`, `NeoDib`, `NeoBuffer` | — |
| Minimal RAM per pixel | 3 bytes (sync, 8-bit RGB) | — |
| C++11 compatibility | Runs on AVR, minimal STL | — |
| — | | Testability (native test suite, mockable interfaces) |
| — | | Runtime reconfiguration (`DynamicBusBuilder`, INI) |
| — | | Composable shader pipeline |
| — | | Active current limiting |
| — | | Single unified protocol for all one-wire chips |
| — | | Arena allocation (fewer mallocs, less fragmentation, compile-time allocation support) |
| — | | Explicit separation of concerns (protocol/transport/shader) |
| — | | Type-safe protocol/transport category enforcement |
| — | | Centralised, platform-independent timing model |
| — | | Modern C++17 safety (RAII, move semantics, no raw malloc) |
