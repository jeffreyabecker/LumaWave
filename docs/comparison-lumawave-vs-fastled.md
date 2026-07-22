# LumaWave vs FastLED — Architecture & Feature Comparison

> **Status:** Updated — reflects post-rearchitecture design (non-owning, direct-construction model).  
> **FastLED version analyzed:** 3.10.3 (`library.properties`)  
> **LumaWave revision:** HEAD as of 2026-07-22

---

## 1  Architectural Philosophy

### 1.1  Dispatch Model

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Primary model | Hybrid: template-instantiated chipset/platform controllers attached to runtime `fl::CLEDController` linked list | Virtual-first seam model (`IPixelBus`, `Protocol`, `Transport`) with non-owning composition |
| User-facing construction | `FastLED.addLeds<CHIPSET, ...>(...)` compile-time selection, global singleton registration | Direct `Bus(span<Pixel>, span<const PipelineRun>)` construction; or `PixelBus<Protocol, Transport, Shaders...>` owning wrapper |
| Runtime polymorphism | Yes at controller list level (`virtual show()/showColor()`) | Yes across all seam boundaries (`IPixelBus`, `Protocol`, `Transport`, `IShader`) |
| Runtime reconfiguration | Limited (enable/disable controllers, selected runtime controls on some platforms) | Via `setRuntimeConfig(RuntimeConfig, void*)` on pipeline, protocol, and shader instances |
| Heterogeneous multi-strip orchestration | Global singleton (`FastLED`) iterates registered controllers | Native via multiple `PipelineRun` entries in a caller-owned array; each run maps an `OutputPipeline*` to a pixel-count slice |

#### Deeper Analysis: Hybrid vs Virtual-First

FastLED combines two strategies: template specialization for chipset/platform implementation details, then runtime iteration via a linked list of `CLEDController` instances. This gives very good per-platform optimization while still allowing a single `FastLED.show()` call over multiple strips.

LumaWave places explicit architectural seams at bus/protocol/transport/shader boundaries. The core `Bus` class takes externally-owned pixel storage and pipeline runs — it allocates nothing itself. This external-lifetime model eliminates hidden heap usage in the hot path and makes composition fully explicit. In exchange for some virtual dispatch overhead at seam boundaries, it enables clean unit testing at every layer.

### 1.2  Separation of Concerns

| Concern | FastLED | LumaWave |
|---------|---------|----------|
| Pixel container | Caller-owned `CRGB[]` passed to controller via `setLeds` | Caller-owned `span<Pixel>` passed to `Bus` constructor |
| Protocol encoding | Typically embedded in chipset/controller templates | Isolated in `Protocol::update(span<const Color>, span<uint8_t>)` |
| Hardware transfer | Platform controller classes (RMT/SPI/clockless/etc.) | Isolated in `Transport::transmitBytes(span<uint8_t>)` |
| Protocol↔transport wiring | Implicit in controller class hierarchy | Explicit `ProtocolTransportPipeline(Protocol&, Transport&, span<uint8_t>)` |
| Color transforms | Global brightness/dither/correction pipeline + utility functions | Shader pipeline (`IShader::apply`), chained via `ShaderProtocol` |
| Public composition seam | Primarily through `FastLED` global API and controller registration | `Bus` + `PipelineRun` array, all dependencies injected by reference |

#### Deeper Analysis: Coupling Profile

FastLED’s architecture is pragmatic for Arduino-style sketches: add controller, mutate `CRGB` buffer, call `show()`. The trade-off is that protocol and transport responsibilities often live inside tightly-coupled controller families, which makes “swap just protocol” or “swap just transport” less explicit at API boundaries.

LumaWave intentionally forces these boundaries to stay independent. Every `Protocol` implementation is a standalone class that encodes pixels into a caller-provided byte buffer. Every `Transport` is a standalone class that transmits bytes. A `ProtocolTransportPipeline` wires them together with a caller-owned buffer. This means you can test a protocol with a mock transport, test a transport with known byte patterns, and swap either independently.

### 1.3  Ownership & Lifetime Model

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Pixel memory ownership | External by default (user array passed via `setLeds`) | External: caller passes `span<Pixel>` to `Bus`; `PixelBus<...>` owns internally for convenience |
| Controller/pipeline ownership | FastLED global owns linked list of controllers | Caller owns all protocol, transport, buffer, shader, and pipeline run instances |
| Global singleton dependence | Strong (`FastLED`) | None — architecture is interface-first, no global state |
| Heap allocation profile | Platform/controller dependent, plus optional features | Zero in `Bus`/`ProtocolTransportPipeline` hot path; `PixelBus<...>` uses `std::vector` for owning convenience path |

---

## 2  C++ Language, Build, and Testing

### 2.1  Language Standard and Portability

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Baseline language mode (observed build config) | C++11 in Meson config (`cpp_std=c++11`) | C++17 (`-std=gnu++17`) |
| Primary portability target | Very broad, including small AVR class devices | Focused modern MCU targets (RP2040/ESP32/ESP8266 + native tests) |
| Template usage | Extensive chipset/platform templating + compatibility wrappers | Template-driven `PixelBus<Protocol, Transport, Shaders...>` owning wrapper; variadic shader construction |

#### Deeper Analysis: Standard Trade-off

FastLED's C++11 baseline helps maintain very broad hardware compatibility. LumaWave's C++17 baseline enables `if constexpr`, fold expressions, and `std::void_t` for cleaner trait-based dispatch in the template layer, but intentionally narrows legacy-toolchain support.

### 2.2  Build and CI Posture

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Build ecosystem | Arduino + PlatformIO + CMake/Meson/test tooling in repo | CMake for native tests; PlatformIO for target builds |
| Public CI footprint | Broad multi-platform workflows and native test lanes | Focused native + target environments in `platformio.ini` |
| Host/native emphasis | Strong (`tests/` with Meson + wrappers + many suites) | Strong (Unity + ArduinoFake, contract test suites per seam) |

### 2.3  Testing Model

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Automated tests | Extensive `tests/` tree and CI workflows | Spec-driven tests under `test/`, organized by seam (`busses/`, `protocols/`, `transports/`, `contracts/`) |
| Test seams | Controller-level + platform stubs/mocks | Explicit per-interface tests (`IPixelBus`, `Protocol`, `Transport`, `IShader`) |
| Contract compile tests | Implicit through broad compile matrix | Explicit contract test suites validating template constraints and type traits |

---

## 3  Bus Construction and Multi-Strip Composition

### 3.1  Construction Models

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Simple single-strip | `FastLED.addLeds<WS2812, DATA_PIN>(leds, NUM_LEDS)` | `PixelBus<Ws2812Protocol, RmtTransport>(pixelCount)` — owning wrapper |
| Advanced / non-owning | Not a first-class API pattern | `Bus(span<Pixel>, span<const PipelineRun>)` — caller owns all resources |
| Multi-strip | Multiple `addLeds` calls, single `FastLED.show()` | Array of `PipelineRun` entries, each with `{OutputPipeline*, length}`, passed to single `Bus` |
| Protocol+transport pairing | Implicit in chipset template selection | Explicit `ProtocolTransportPipeline(protocol, transport, buffer)` |
| Shader integration | Global brightness/dither pipeline | `ShaderProtocol(protocol, span<IShader*>, scratchPixels)` wraps protocol with shader chain |

### 3.2  `PixelBus<...>` Template Convenience

LumaWave provides `PixelBus<Protocol, Transport, Shaders...>` as an owning convenience wrapper that internally constructs and owns:

- The protocol instance
- The transport instance
- The protocol encoding buffer (sized via `Protocol::requiredBufferSize()`)
- Scratch pixel buffer (when shaders are present)
- Shader instances (constructed from per-shader tuple arguments)
- `ShaderProtocol` → `ProtocolTransportPipeline` → `PipelineRun` → `Bus` chain

This is the closest analog to FastLED's `addLeds<...>()` — a single-line constructor that wires everything. For maximum control, users can drop to the non-owning `Bus` + `PipelineRun` level and manage every instance themselves.

---

## 4  Color and Rendering Pipeline

### 4.1  Color Types and API Ergonomics

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Core color type | `CRGB` (+ HSV utilities, palettes, etc.) | `lw::Pixel` (RGB/RGBW with channel-order abstraction) |
| Typical app model | Mutate user `CRGB[]` frame buffer, call `FastLED.show()` | Mutate bus pixels via `bus.pixels()`, call `bus.show()` |
| Color utilities | Rich inline functions on `CRGB` struct | Free functions: `pixelR()`, `setColorR()`, `fillPixels()`, palette sampling |

### 4.2  Shader Pipeline

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Brightness | Global `FastLED.setBrightness()` applied at show-time | `BrightnessShader` in shader chain, configured via `setRuntimeConfig(RuntimeConfig::Brightness, &value)` |
| Gamma correction | Built-in correction tables, configurable | `GammaShader` as an explicit shader stage |
| Shader composition | Implicit global pipeline | Explicit ordered `IShader*` array passed to `ShaderProtocol` |
| Extensibility | Fixed pipeline stages | Any `IShader` implementation can be inserted at any position in the chain |

### 4.3  Power Management

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Core capability | Built-in power budgeting (`setMaxPowerInVoltsAndMilliamps`) | `CurrentLimiterShader` as an explicit shader stage |
| Application model | Brightness constrained around `show()` lifecycle | Shader-enforced budget inside pipeline, applied before protocol encoding |
| Extensibility | Power model structs (`PowerModelRGB`, RGBW/RGBWW placeholders) | Any shader can implement current-limiting policy; protocols with native per-LED current control (e.g. TLC59711, SM168x) do not require a shader |

---

## 5  Protocol and Transport Layering

### 5.1  Protocol Separation

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Protocol interface | Embedded in chipset controller templates | `Protocol` base class with `update(span<const Color>, span<uint8_t>)` |
| Buffer management | Internal to controller | Caller provides `span<uint8_t>` protocol buffer; size via `requiredBufferSize(pixelCount, settings)` |
| Settings type | Chipset-specific, varied patterns | Each protocol defines `SettingsType`; passed to constructor |
| Supported protocols | WS2812, APA102, SK9822, TM1809, and many more | Ws2812, DotStar (APA102), Lpd8806, Lpd6803, P9813, Sm16716, Sm168x, Tm1814, Tm1914, Tlc59711, Pixie |

### 5.2  Transport Separation

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Transport interface | Platform-specific controller implementations | `Transport` base class with `transmitBytes(span<uint8_t>)` |
| Settings contract | Platform-dependent | All transport settings must expose `public bool invert` |
| Supported transports | RMT, SPI, bit-banged (clockless), I2S, etc. | RMT, SPI, PWM output, Print (debug), Nil (no-op) |
| Platform calls | Dispersed through controller hierarchy | Confined to `Transport` implementations; core contracts are `Arduino.h`-free |

### 5.3  Protocol/Transport Independence

LumaWave's protocol and transport implementations have zero knowledge of each other. A protocol converts colors to bytes; a transport sends bytes. Any protocol can pair with any transport via `ProtocolTransportPipeline`. This means:

- New protocols only need to implement `Protocol::update()`
- New transports only need to implement `Transport::transmitBytes()`
- All existing protocol↔transport combinations work automatically

---

## 6  Practical Fit Guidance

### Favor **FastLED** when:

- You want the classic Arduino sketch workflow with a large ecosystem of examples and effects.
- You need extremely broad board/platform coverage, including older AVR-class targets.
- You prefer a single global orchestration API and direct `CRGB` buffer manipulation.

### Favor **LumaWave** when:

- You need explicit protocol/transport/shader seams for modular architecture and testing.
- You want zero-heap-allocation hot paths with caller-controlled lifetime for all resources.
- You need to mix protocols and transports freely, or insert custom shader stages at arbitrary positions.
- You want compile-time contract validation that mirrors the virtual-first runtime design.
- You need multi-strip composition where each strip can have a different protocol/transport pair.

---

## 7  Key Takeaway

Both libraries serve LED animation but optimize for fundamentally different engineering priorities:

- **FastLED** optimizes for ecosystem breadth, platform reach, and familiar sketch ergonomics — add a controller, mutate a buffer, call `show()`.
- **LumaWave** optimizes for explicit architecture seams, caller-controlled lifetime, and contract-driven composability — every protocol, transport, shader, and buffer is an independently testable, swappable unit.

For teams building long-lived, multi-protocol systems with evolving runtime composition needs, LumaWave's seam-first, non-owning design is typically the better structural fit. For rapid sketch-driven visual work across diverse boards, FastLED remains a strong default.
