# Bus Builder & Lifetime Simplification

> **Purpose:** Provide a single-owner, cascading-lifetime root entity (the "Bus Builder") that simplifies construction of both single-pipeline and composite multi-pipeline `IPixelBus` instances, eliminating manual wiring of 7–10 interdependent objects.
>
> **Companion usage doc:** [bus-builder.md](../../usage/bus-builder.md) — the target-state API contract and usage rules.

## Status Legend

| Status | Meaning |
|--------|---------|
| `todo` | Not yet started |
| `doing` | In progress |
| `done` | Completed |
| `blocked` | Blocked on external dependency |
| `deferred` | Intentionally postponed |

## Source Documents

| Category | File | Key Symbols |
|----------|------|-------------|
| Bus — interface | `src/core/IPixelBus.h` | `lw::IPixelBus` |
| Bus — composite | `src/buses/Bus.h` | `lw::buses::Bus`, `lw::buses::PipelineRun` |
| Bus — dynamic template | `src/buses/PixelBus.h` | `lw::buses::PixelBus<TProtocol, TTransport, ...TShaders>` |
| Bus — static template | `src/buses/StackPixelBus.h` | `lw::buses::StackPixelBus<NPixelCount, TProtocol, TTransport, ...TShaders>` |
| Bus — convenience header | `src/buses/Busses.h` | — |
| Pipeline | `src/buses/ProtocolTransportPipeline.h` | `lw::buses::ProtocolTransportPipeline` |
| Protocol | `src/protocols/Protocol.h` | `lw::protocols::Protocol`, `lw::protocols::ProtocolSettings` |
| Shader protocol | `src/protocols/ShaderProtocol.h` | `lw::protocols::ShaderProtocol` |
| Shader interface | `src/protocols/IShader.h` | `lw::protocols::IShader` |
| Transport | `src/transports/Transport.h` | `lw::transports::Transport`, `lw::transports::TransportSettingsBase` |
| Output pipeline | `src/core/OutputPipeline.h` | `lw::OutputPipeline` |
| Color / span | `src/core/Compat.h` | `lw::Pixel`, `lw::span`, `lw::PixelCount` |
| Tests — Bus | `test/busses/test_bus/test_main.cpp` | Manual `Bus` + `PipelineRun` construction |
| Tests — StackPixelBus | `test/busses/test_pixel_bus/test_main.cpp` | Static template construction |
| Tests — PixelBus | `test/busses/test_pixel_bus_dynamic/test_main.cpp` | Dynamic template construction |
| Inventory | `docs/internal/platform-transport-inventory.md` | Full transport catalog |
| Presets (planned) | `src/buses/BusPresets.h` | `lw::buses::presets` convenience functions |
| Reference: NeoPixelBus | `NeoPixelBus.h` (Makuna) | `T_COLOR_FEATURE` + `T_METHOD` composition, `NeoGrbFeature`, `Neo800KbpsMethod` |

## Current State

### Architecture Overview

The LumaWave bus architecture composes pixels → protocol → transport through a chain of four layers:

```mermaid
graph TD
    A[IPixelBus] --> B[Bus]
    B --> C[PipelineRun array]
    C --> D[OutputPipeline]
    D --> E[ProtocolTransportPipeline]
    E --> F[Protocol]
    E --> G[Transport]
    F --> H[ShaderProtocol]
    H --> I[Inner Protocol]
    H --> J[IShader array]
```

`Bus` (`src/buses/Bus.h`) is the concrete terminal: it holds a `span<Pixel>` for pixel storage and a `span<const PipelineRun>`. Each `PipelineRun` pairs an `OutputPipeline*` with a length. On `show()`, the bus slices the pixel span by run length and delegates to each pipeline's `write()`.

`ProtocolTransportPipeline` (`src/buses/ProtocolTransportPipeline.h`) bridges protocol and transport: `write()` calls `Protocol::update()` to encode colors → bytes, then `Transport::transmitBytes()` to push bytes to hardware. It holds three references — `Protocol&`, `Transport&`, and `span<uint8_t>` protocol buffer.

`ShaderProtocol` (`src/protocols/ShaderProtocol.h`) is a decorator that interposes shaders between the caller and the inner protocol: it applies each `IShader::apply()` in sequence through a scratch pixel buffer, then passes the result to the inner protocol.

### Current Construction Approaches

**Approach A: Manual (low-level)**

The `Bus` class only takes references/views — it owns nothing. Users must manually allocate and keep alive:

```cpp
// All of these must live as long as the Bus:
std::array<lw::Pixel, 30> pixels{};
Ws2812xProtocol protocol(30);
NilTransport transport;
std::vector<uint8_t> protoBuf(Ws2812xProtocol::requiredBufferSize(30));
ProtocolTransportPipeline pipeline(protocol, transport, protoBuf);
PipelineRun runs[] = {{&pipeline, 30}};
Bus bus(pixels, runs);
```

This is 7 distinct allocations/objects with tangled lifetimes. Adding shaders adds: a `ShaderProtocol`, a `tuple<TShaders...>`, a `vector<IShader*>`, and a `vector<Color>` scratch buffer — bringing the total to 11 objects.

**Approach B: `PixelBus` template (dynamic, heap)**

`PixelBus<TProtocol, TTransport, ...TShaders>` (`src/buses/PixelBus.h`) owns everything by value. Its constructor initializer list creates 10+ members in dependency order:

```cpp
PixelBus(size_t pixelCount, ProtocolSettings ps, TransportSettings ts)
    : _pixelCount(pixelCount)
    , _pixels(pixelCount)
    , _protocol(pixelCount, ps)
    , _transport(std::move(ts))
    , _protocolBuffer(TProtocol::requiredBufferSize(...))
    , _scratchPixels(HasShaders ? pixelCount : 0)
    , _shaders{}                                           // tuple
    , _shaderPtrs(sizeof...(TShaders))                     // vector<IShader*>
    , _shaderProto(_protocol, _shaderPtrs, _scratchPixels) // ShaderProtocol
    , _pipeline(_shaderProto, _transport, _protocolBuffer) // ProtocolTransportPipeline
    , _run{&_pipeline, pixelCount}                         // PipelineRun
    , _bus(_pixels, span{&_run, 1})                        // Bus
{}
```

Two constructor overloads exist: default-constructed shaders vs. `std::tuple`-arg shaders. The member initialization order is fragile — `_shaderProto` must come after `_shaders`, `_shaderPtrs`, and `_scratchPixels`; `_pipeline` must come after `_shaderProto`, `_transport`, and `_protocolBuffer`; `_bus` must come last.

**Approach C: `StackPixelBus` template (static, stack)**

`StackPixelBus<NPixelCount, TProtocol, TTransport, ...TShaders>` (`src/buses/StackPixelBus.h`) is identical to `PixelBus` but uses `std::array` and C-arrays instead of `std::vector`, with all sizes known at compile time.

### Pain Points

1. **Fragile init-order dependency.** The 10+ members in `PixelBus`/`StackPixelBus` must be declared in strict order. A reordering breaks the initializer list — a subtle bug that compiles but produces dangling references at runtime.

2. **Duplicate constructor overloads.** Every addition to the shader pipeline requires two constructor overloads (default + tuple-arg). The template machinery (`std::make_from_tuple`, `std::index_sequence`) is opaque.

3. **No multi-strip support in PixelBus/StackPixelBus.** These templates hardcode a single `PipelineRun` and a single `ProtocolTransportPipeline`. To drive multiple strips from one pixel buffer, the user must drop down to manual `Bus` construction.

4. **No transport/protocol sharing.** Each `PixelBus`/`StackPixelBus` owns its protocol and transport by value. Two strips sharing a clock line cannot share a transport instance.

5. **No gradual construction.** The entire bus must be fully specified at construction time. There is no way to add shaders, change settings, or add runs after construction.

## Problem Statement

Constructing a complete `IPixelBus` requires allocating and wiring 7–11 interdependent objects with strict lifetime ordering. The `PixelBus` and `StackPixelBus` templates encapsulate this but at the cost of:

- **Fragile member ordering** — the initializer list is a single point of failure for 10+ dependencies.
- **Template explosion** — the shader parameter pack propagates through every layer, making error messages and IDE tooltips unwieldy.
- **No composition reuse** — a multi-strip scenario requires abandoning the templates entirely and hand-wiring with raw `Bus`.
- **Duplicated logic** — `PixelBus` and `StackPixelBus` differ only in allocation strategy (vector vs. array) but have near-identical code.

These costs make it harder to add new bus topologies (e.g., matrix, serpentine), harder to write examples, and harder to test in isolation.

## Design Goals

1. **Single-owner cascade.** A single root object owns all sub-objects. Destroying the root cleanly destroys every owned buffer, shader, protocol, transport, and pipeline. No dangling references.

2. **Builder-style incremental construction.** Users can add shaders, set settings, and attach runs step-by-step before finalizing. Finalization validates the configuration and produces an `IPixelBus`.

3. **Unified static/dynamic allocation.** A single API surface supports both compile-time-sized (stack/static) and runtime-sized (heap/dynamic) allocation, without duplicating all template logic.

4. **Multi-run (composite) support.** The same API should handle single-strip and multi-strip (aggregate) cases, including the ability to share a transport or protocol across runs.

5. **Non-breaking.** Existing `PixelBus`, `StackPixelBus`, `Bus`, `ProtocolTransportPipeline`, `Protocol`, `Transport`, and `IShader` types remain unchanged and fully functional. The new system is additive.

6. **Testable in isolation.** Each layer (storage allocation, pipeline construction, bus finalization) should be independently testable via the native CMake test suite.

7. **Clear error messages.** Misconfiguration (e.g., forgetting to attach a transport) should produce a comprehensible compile-time or runtime error, not a cryptic template backtrace.

8. **External pixel buffer support.** Allow callers to provide a pre-allocated pixel buffer that the bus writes into directly, enabling zero-copy integration with frameworks (e.g., WLED) that manage their own LED buffer arrays. The external buffer path must not force a heap allocation for pixel storage.

9. **Discoverable protocol/transport presets.** Common chip+transport combinations should be available as named convenience functions so users don't need to know which `Protocol` subclass and `Transport` subclass to pair. Inspired by NeoPixelBus's `NeoGrbFeature` + `Neo800KbpsMethod` pattern, presets collapse the two-axis choice (what chip? what hardware?) into discoverable one-liners.

## Non-Goals

- **No modification to `Protocol` or `Transport` base classes.** These are stable seams; the builder wraps them, it does not change them.
- **No runtime polymorphism overhead on the hot path.** After construction/finalization, the resulting `IPixelBus` should have the same call-graph cost as today's `PixelBus`/`Bus`.
- **No dynamic allocation forced on embedded targets.** Stack-only construction must remain available for `-fno-rtti -fno-exceptions` embedded builds.
- **No replacement of `IShader`.** The shader interface and `ShaderProtocol` decorator remain unchanged.
- **No serialization or configuration-file support.** The builder is a C++ API, not a JSON/YAML parser.

## Recommended End State

### New Type: `lw::buses::BusBuilder`

A builder class that incrementally accumulates configuration and owns all heap-allocated intermediates. Finalization produces an `IPixelBus` backed by a single-owner storage object.

```cpp
namespace lw::buses
{

class BusBuilder
{
public:
    // --- Pixel storage ---

    /// Set dynamic pixel count. Allocates pixel storage internally.
    BusBuilder& setPixelCount(size_t count);

    /// Use externally-owned pixel storage. The builder does not allocate pixel memory.
    /// Mutually exclusive with setPixelCount(). The provided span must outlive
    /// the returned IPixelBus. Pixel count is inferred from span.size().
    BusBuilder& setPixelStorage(span<Pixel> externalPixels);

    // --- Transport ---

    /// Attach a transport by move. The builder takes ownership.
    template<typename TTransport>
    BusBuilder& setTransport(TTransport transport);

    // --- Protocol ---

    /// Attach a protocol by move. The builder takes ownership.
    template<typename TProtocol>
    BusBuilder& setProtocol(TProtocol protocol, typename TProtocol::SettingsType settings = {});

    // --- Shaders ---

    /// Add a shader. Shaders apply in insertion order.
    template<typename TShader>
    BusBuilder& addShader(TShader shader);

    // --- Runs (for multi-strip) ---

    /// Add a run referencing the most recently set protocol+transport pair.
    /// For single-strip, this is called implicitly by build().
    BusBuilder& addRun(size_t pixelOffset, size_t length);

    // --- Finalization ---

    /// Validate and build. Returns a heap-allocated IPixelBus.
    /// The builder is consumed (moved-from) after this call.
    std::unique_ptr<IPixelBus> build();

    // --- Static variant ---

    /// Build into caller-provided storage. Returns a reference to the bus.
    /// Storage type must satisfy BusStorage concept (pixels, buffers, etc.).
    template<typename TStorage>
    IPixelBus& buildInto(TStorage& storage);

private:
    // ... internal type-erased storage for transport, protocol, shaders, buffers ...
};

} // namespace lw::buses
```

### Usage Examples

**Single strip (simplest case):**

```cpp
auto bus = lw::buses::BusBuilder()
    .setPixelCount(30)
    .setTransport(lw::transports::SpiTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();

bus->begin();
bus->pixels()[0] = lw::pixelFromRGB(255, 0, 0);
bus->show();
```

**Single strip with shaders:**

```cpp
auto bus = lw::buses::BusBuilder()
    .setPixelCount(60)
    .setTransport(lw::transports::RpPioTransport{})
    .setProtocol(lw::protocols::Apa102Protocol{}, {})
    .addShader(lw::protocols::BrightnessShader{128})
    .addShader(lw::protocols::GammaShader{2.2f})
    .build();
```

**Multi-strip (aggregate bus):**

```cpp
auto bus = lw::buses::BusBuilder()
    .setPixelCount(90)              // 30 + 60 total
    // Strip 1: pixels [0, 30) via SPI
    .setTransport(lw::transports::SpiTransport{})
    .setProtocol(lw::protocols::Ws2801Protocol{}, {})
    .addRun(0, 30)
    // Strip 2: pixels [30, 90) via PIO
    .setTransport(lw::transports::RpPioTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .addRun(30, 60)
    .build();
```

**External pixel storage (zero-copy, WLED-compatible):**

```cpp
// Caller owns the pixel buffer; BusBuilder wires everything else.
std::array<lw::Pixel, 90> ledStrip{};

auto bus = lw::buses::BusBuilder()
    .setPixelStorage(ledStrip)
    .setTransport(lw::transports::RpPioTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();

// bus->pixels() returns a span referencing ledStrip directly.
// Writes to bus->pixels()[n] write to ledStrip[n] — no copy.
bus->begin();
bus->pixels()[0] = lw::pixelFromRGB(255, 0, 0);
bus->show();
// ledStrip must outlive bus
```

**Static/stack allocation:**

```cpp
// User provides static storage; builder validates sizes at compile time.
lw::buses::StackBusStorage<90,
    lw::protocols::Ws2812xProtocol,
    lw::transports::RpPioTransport> storage;

auto& bus = lw::buses::BusBuilder()
    .setPixelCount(90)
    .setTransport(lw::transports::RpPioTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .buildInto(storage);

bus.begin();
bus.pixels()[0] = lw::pixelFromRGB(255, 0, 255);
bus.show();
```

### Protocol/Transport Presets

NeoPixelBus composes two orthogonal template parameters — **Color Feature** (RGB, GRB, BGR, WRGB byte order) and **Method** (800Kbps, 400Kbps, DotStar SPI, DMA) — into well-known combinations like `NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>`. Users pick from a catalog of named types rather than wiring raw protocol settings.

LumaWave's analog uses **preset functions** that return a partially-configured `BusBuilder`. Because LumaWave's protocols already bundle color feature + byte encoding, and transports are a separate choice, presets collapse the common pairings into discoverable factory functions.

**Design:** Presets live in `lw::buses::presets` as free functions returning `BusBuilder` by value.

```cpp
namespace lw::buses::presets
{

// --- Protocol-only presets (caller adds transport) ---

/// WS2812-family (one-wire, GRB color order).
BusBuilder ws2812x(size_t pixelCount);
/// WS2812 with RGB color order.
BusBuilder ws2812xRgb(size_t pixelCount);
/// WS2812 with WRGB (RGB+White) color order.
BusBuilder ws2812xWrgb(size_t pixelCount);

/// DotStar / APA102 / SK9822 (two-wire SPI, BGR color order).
BusBuilder dotstar(size_t pixelCount);
/// APA102 with RGB color order.
BusBuilder apa102Rgb(size_t pixelCount);

/// WS2801 (two-wire SPI, RGB color order).
BusBuilder ws2801(size_t pixelCount);

/// LPD8806 (two-wire SPI, GRB color order).
BusBuilder lpd8806(size_t pixelCount);

/// TM1814 (one-wire, WRGB color order, current-control capable).
BusBuilder tm1814(size_t pixelCount);

/// P9813 (two-wire SPI, BGR color order with checksum).
BusBuilder p9813(size_t pixelCount);

// --- Full bundles (protocol + transport + pixel count) ---

/// WS2812 on RP2040 PIO.
BusBuilder ws2812xPio(size_t pixelCount, int pin);
/// WS2812 on hardware SPI.
BusBuilder ws2812xSpi(size_t pixelCount);
/// DotStar on hardware SPI.
BusBuilder dotstarSpi(size_t pixelCount);
/// APA102 on hardware SPI.
BusBuilder apa102Spi(size_t pixelCount);
/// WS2801 on hardware SPI.
BusBuilder ws2801Spi(size_t pixelCount);

} // namespace lw::buses::presets
```

**Usage:** Presets return a `BusBuilder` by value, so the caller can continue the chain:

```cpp
// Full preset: WS2812 strip on RP2040 PIO pin 2 with brightness

auto bus = lw::buses::presets::ws2812xPio(60, 2)
    .addShader(lw::protocols::BrightnessShader{128})
    .build();

// Protocol-only preset: caller supplies transport and shaders

auto bus = lw::buses::presets::dotstar(144)
    .setTransport(lw::transports::SpiTransport{})
    .addShader(lw::protocols::GammaShader{2.2f})
    .build();

// Multi-strip with presets

auto bus = lw::buses::presets::ws2801Spi(30)   // strip 1: WS2801 via SPI
    .addRun(0, 30)                                // pixels [0, 30)
    .setTransport(lw::transports::RpPioTransport{3})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .addRun(30, 60)                               // strip 2: pixels [30, 90)
    .build();
```

**Key design properties:**

- **Non-breaking:** Presets are additive. `setTransport`/`setProtocol` remain the explicit path.
- **Discoverable:** Each chip family has named presets — no need to memorize protocol class names.
- **Composable:** Preset return values are ordinary `BusBuilder` objects; shaders, runs, and transport can be added after.
- **Testable:** Each preset is a thin wrapper that can be tested by verifying the resulting builder configuration.
- **No template propagation:** Presets are plain functions, not templates (they internally use the templated `setTransport`/`setProtocol` but the preset signature is non-template).

**Comparison to NeoPixelBus:**

| Concept | NeoPixelBus | LumaWave |
|---------|-------------|----------|
| Color order | `T_COLOR_FEATURE` template param (`NeoGrbFeature`) | `channelOrder` in `ProtocolSettings` (runtime) |
| Protocol timing | `T_METHOD` template param (`Neo800KbpsMethod`) | Chosen by `Protocol` subclass (`Ws2812xProtocol`) |
| Transport hardware | Bundled inside `T_METHOD` | Separate `Transport` subclass (`RpPioTransport`) |
| Convenience | `NeoPixelBus<Feature, Method>` typedef | `presets::ws2812xPio(count, pin)` function |

### Internal Architecture

The builder uses type erasure internally to avoid template propagation across the builder chain:

```mermaid
graph TD
    subgraph "User-facing (templated)"
        A[BusBuilder::setTransport&lt;T&gt;]
        B[BusBuilder::setProtocol&lt;T&gt;]
        C[BusBuilder::addShader&lt;T&gt;]
        D[BusBuilder::addRun]
        E[BusBuilder::build / buildInto]
    end

    subgraph "Internal (type-erased)"
        F[TransportHolder]
        G[ProtocolHolder]
        H[ShaderList]
        I[BufferManager]
        J[BusStorage concept]
    end

    subgraph "Result"
        K[IPixelBus*]
        L[Bus + PipelineRun array + ProtocolTransportPipeline(s)]
    end

    A --> F
    B --> G
    C --> H
    D --> I
    E --> J --> L --> K
```

### Ownership Model

```
BusBuilder (during construction)
├── TransportHolder ── owns unique_ptr<Transport>
├── ProtocolHolder ── owns unique_ptr<Protocol>
├── ShaderList ── owns vector<unique_ptr<IShader>>
├── BufferManager ── owns protocol buffer(s) + scratch pixel buffer
├── Pipeline descriptions ── vector of {offset, length, transport_index, protocol_index}
└── Pixel storage ── vector<Color>

After build():
  BusStorage (single-owner RAII object)
  ├── vector<Color> pixels
  ├── vector<uint8_t> protocolBuffer(s)
  ├── vector<Color> scratchPixels
  ├── tuple<TShaders...> shaders
  ├── vector<IShader*> shaderPtrs
  ├── ShaderProtocol
  ├── ProtocolTransportPipeline (one per run)
  ├── vector<PipelineRun> runs
  └── Bus
```

The `BusStorage` object is heap-allocated by `build()` and returned as `unique_ptr<IPixelBus>`. For `buildInto()`, the caller provides a `StackBusStorage<N, ...>` which is a POD-like struct with compile-time-sized arrays.

### StackBusStorage Concept

```cpp
template<size_t NPixelCount, typename TProtocol, typename TTransport, typename... TShaders>
struct StackBusStorage
{
    // Compile-time-computed sizes
    static constexpr size_t kProtocolBufferSize = TProtocol::requiredBufferSize(NPixelCount, {});

    lw::Pixel pixels[NPixelCount]{};
    uint8_t protocolBuffer[kProtocolBufferSize]{};
    lw::Pixel scratchPixels[sizeof...(TShaders) > 0 ? NPixelCount : 1]{};
    std::tuple<TShaders...> shaders{};
    lw::protocols::IShader* shaderPtrs[sizeof...(TShaders)]{};
    TProtocol protocol;
    TTransport transport;
    lw::protocols::ShaderProtocol shaderProto;
    lw::buses::ProtocolTransportPipeline pipeline;
    lw::buses::PipelineRun runs[1];
    lw::buses::Bus bus;

    StackBusStorage()
        : protocol(NPixelCount)
        , shaderProto(protocol, shaderPtrs, scratchPixels)
        , pipeline(shaderProto, transport, protocolBuffer)
        , runs{{&pipeline, NPixelCount}}
        , bus(lw::span<lw::Pixel>{pixels}, lw::span<const lw::buses::PipelineRun>{runs})
    {}

    // Non-copyable, non-movable (internal references would dangle)
    StackBusStorage(const StackBusStorage&) = delete;
    StackBusStorage& operator=(const StackBusStorage&) = delete;
};
```

`StackBusStorage` is a code-gen candidate (could be produced by a macro or `constexpr` generator) since it mirrors the exact member dependency order of today's `StackPixelBus`. The key improvement is that it's decoupled from the builder — the builder populates it via `buildInto()` rather than the struct being a self-contained template monolith.

### Migration Path

The plan is additive:

1. Introduce `BusBuilder` and `BusStorage` alongside existing types.
2. Port examples (`examples/hello/ws2812/`, `examples/hello/apa102/`) to use the builder. Validate ergonomics.
3. Add multi-strip examples using `addRun()`.
4. Optionally deprecate `PixelBus` and `StackPixelBus` once the builder has equivalent coverage.
5. The existing `Bus`, `ProtocolTransportPipeline`, `Protocol`, `Transport`, `IShader` remain unchanged throughout.

## Open Decisions

| ID | Status | Decision | Notes |
|----|--------|----------|-------|
| BBL-DEC-1 | `todo` | Should `BusBuilder` be move-only (unique ownership) or support copy? | Move-only is simpler and matches the single-owner goal. Copy would require deep-cloning type-erased transports, which is problematic for hardware resources. |
| BBL-DEC-2 | `todo` | Should `StackBusStorage` be hand-authored per permutation or generated? | Hand-authoring the first one is fine. A code-gen script or `constexpr` factory could follow if the pattern stabilizes. |
| BBL-DEC-3 | `todo` | Should `build()` return `unique_ptr<IPixelBus>` or a concrete `BusStorage` by value? | `unique_ptr<IPixelBus>` preserves interface abstraction and allows the storage object to be opaque. Returning by value would expose the storage type to the caller, breaking encapsulation. |
| BBL-DEC-4 | `todo` | Should the builder support sharing transports across runs? | Current `ProtocolTransportPipeline` holds references — sharing is already possible at the low level. The builder should allow runs to reference the same transport index. |
| BBL-DEC-5 | `todo` | Should `BusBuilder` use `std::any` / `std::function` for type erasure, or a custom vtable approach? | Custom vtable avoids RTTI and exception overhead, which matters for `-fno-rtti -fno-exceptions` embedded targets. A hand-rolled `TransportHolder` with a `unique_ptr<TransportBase>` + move-only semantics is preferred. |
| BBL-DEC-6 | `todo` | What is the error handling strategy for `build()`? | Options: (a) return `nullptr` on failure, (b) return `expected<unique_ptr<IPixelBus>, Error>` (C++23 style), (c) assert/abort. Given embedded constraints, option (a) or (c) is most practical. The builder can also have a `validate()` method for early checking. |
| BBL-DEC-7 | `todo` | Should `setPixelStorage()` and `setPixelCount()` be mutually exclusive, or should `setPixelStorage()` override `setPixelCount()`? | Mutual exclusion is clearer and avoids ambiguity about which allocation is authoritative. The builder should reject (at runtime) a configuration that calls both. Should `buildInto()` with `StackBusStorage` also support external pixels, or is that only for `build()`? If `StackBusStorage` owns pixel memory by definition, external pixels would need a different storage concept (e.g., `StackBusStorageNoPixels`). |
| BBL-DEC-8 | `todo` | Should presets live as static methods on `BusBuilder` or as free functions in `lw::buses::presets`? | Free functions in a `presets` namespace keep `BusBuilder` focused and avoid coupling the builder to every protocol/transport header. Presets can be included selectively via a `BusPresets.h` convenience header. |
| BBL-DEC-9 | `todo` | Should full-bundle presets (protocol+transport) set default pin assignments, or require explicit pin arguments? | Transport-only presets (e.g., `ws2812xSpi`) can use default SPI pins. PIO/bitbang presets should require explicit pin arguments since there is no universal default. |
| BBL-DEC-10 | `todo` | How should presets be organized — one header per chip family, a single `BusPresets.h`, or inline in protocol headers? | A single `src/buses/BusPresets.h` that includes only the presets the user requests (via `#include` of specific protocol headers) keeps compile times low and discoverability high. Protocol headers themselves should not depend on `BusBuilder`. |
