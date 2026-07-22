# Bus Builder

> **Current state.** This document describes the `BusBuilder` API as implemented.
> For the architecture rationale, see the [Bus Builder design plan](../backlog/plan/bus-builder-lifetime-simplification.md).

## Purpose

This document defines the contract and usage rules for `lw::buses::BusBuilder` — the recommended API for constructing `IPixelBus` instances. It covers the builder's lifecycle, the fluent method chain, ownership invariants, the storage concept, and the static/dynamic allocation split.

**Audience:** Bus authors constructing `IPixelBus` instances for any topology; transport/protocol authors who need to understand what their types must support to be builder-compatible.

This document complements:

- [Bus Builder design plan](../backlog/plan/bus-builder-lifetime-simplification.md) — architecture rationale, problem analysis, open decisions
- [compilation-flags.md](./compilation-flags.md) — `LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES` flag that gates template-based bus exports

---

## 1  Lifecycle and Ownership

### Rule 1.1 — Builder is move-only

`BusBuilder` owns type-erased transport, protocol, shader, and buffer allocations internally. It must not be copied.

```cpp
auto b1 = lw::buses::BusBuilder().setPixelCount(60);
// auto b2 = b1;              // compile error — deleted copy constructor
auto b3 = std::move(b1);      // ok — b1 is now moved-from
```

### Rule 1.2 — build() consumes the builder

After `build()`, the builder is in a moved-from state. Calling any method on it (other than destruction or move-assignment) is undefined behavior.

```cpp
auto builder = lw::buses::BusBuilder()
    .setPixelCount(30)
    .setTransport(lw::transports::SpiTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {});

auto bus = builder.build();
// builder.setPixelCount(60);  // undefined — builder was consumed
```

### Rule 1.3 — The returned bus owns all resources

The `unique_ptr<IPixelBus>` returned by `build()` is the sole owner of all pixel storage, protocol buffers, scratch buffers, shaders, protocol, transport, and internal pipeline objects. Destroying the `unique_ptr` destroys everything.

```cpp
{
    auto bus = lw::buses::BusBuilder()
        .setPixelCount(30)
        .setTransport(lw::transports::RpPioTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
        .addShader(lw::shaders::BrightnessShader{128})
        .build();

    bus->begin();
    bus->pixels()[0] = lw::pixelFromRGB(255, 0, 0);
    bus->show();
}
// bus destroyed — all buffers, shaders, protocol, transport freed
```

### Rule 1.4 — buildInto() does not own the storage

When using `buildInto(storage)`, the caller owns the storage object and must keep it alive for the lifetime of the returned `IPixelBus&`. The builder transfers configuration into the storage but does not take ownership.

```cpp
lw::buses::StackBusStorage<90,
    lw::protocols::Ws2812xProtocol,
    lw::transports::RpPioTransport> storage;

{
    auto& bus = lw::buses::BusBuilder()
        .setPixelCount(90)
        .setTransport(lw::transports::RpPioTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
        .buildInto(storage);

    bus.begin();
    bus.show();
}
// storage still alive — bus reference is valid as long as storage exists
```

---

## 2  Fluent Method Chain

### Rule 2.1 — setPixelCount() must be called first

`setPixelCount()` establishes the total pixel count for the bus. All subsequent `addRun()` calls must specify ranges within `[0, pixelCount)`. The builder allocates pixel storage internally based on this value.

```cpp
// Correct
auto bus = lw::buses::BusBuilder()
    .setPixelCount(60)
    .setTransport(lw::transports::NilTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();

// Error — pixel count not set before build()
// auto bus = lw::buses::BusBuilder()
//     .setTransport(...)
//     .build();
```

### Rule 2.1b — setPixelStorage() as an external-pixel alternative

As an alternative to `setPixelCount()`, callers can provide an externally-owned pixel buffer via `setPixelStorage()`. This enables zero-copy integration with frameworks (e.g., WLED) that manage their own LED buffer arrays. When `setPixelStorage()` is used, the builder does not allocate pixel memory — it wires the external span directly into the bus. `setPixelStorage()` and `setPixelCount()` are mutually exclusive.

```cpp
// External pixel buffer — caller owns the storage
std::array<lw::Pixel, 300> ledStrip{};

auto bus = lw::buses::BusBuilder()
    .setPixelStorage(ledStrip)
    .setTransport(lw::transports::RpPioTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();

// bus->pixels() returns a span that references ledStrip directly
bus->pixels()[0] = lw::pixelFromRGB(255, 0, 0);  // writes to ledStrip[0]
```

### Rule 2.2 — setTransport() and setProtocol() form a pair

Each builder constructs exactly one strip. `setTransport()` and `setProtocol()` define the pipeline for that strip. For multi-strip configurations, use separate `BusBuilder` instances with `setPixelStorage()` over sub-spans of a shared pixel buffer.

```cpp
// Single strip
    auto bus = lw::buses::BusBuilder()
        .setPixelCount(60)
        .setTransport(lw::transports::NilTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol(60, {}))
        .build();

    // Multi-strip: shared buffer, separate builders
    lw::Pixel allPixels[90];
    auto strip1 = lw::buses::BusBuilder()
        .setPixelStorage(lw::span<lw::Pixel>{allPixels, 30})
        .setTransport(...)
        .setProtocol(lw::protocols::Ws2812xProtocol(30, {}))
        .build();
    auto strip2 = lw::buses::BusBuilder()
        .setPixelStorage(lw::span<lw::Pixel>{allPixels + 30, 60})
        .setTransport(...)
        .setProtocol(lw::protocols::Ws2812xProtocol(60, {}))

### Rule 2.3 — addShader() chains shaders in insertion order

Shaders are applied in insertion order via a `ShaderProtocol` decorator. A scratch pixel buffer is automatically allocated when shaders are present. Call `enableDestructiveShaders()` to skip scratch allocation and mutate the pixel buffer in place.

```cpp
auto bus = lw::buses::BusBuilder()
    .setPixelCount(60)
    .setTransport(lw::transports::NilTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol(60, {}))
    .addShader(lw::shaders::BrightnessShader{128})    // applied first
    .addShader(lw::shaders::GammaShader{2.2f})        // applied second
    .build();

// Destructive mode: no scratch, shaders mutate pixels in-place
auto bus2 = lw::buses::BusBuilder()
    .setPixelCount(60)
    .setTransport(lw::transports::NilTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol(60, {}))
    .addShader(lw::shaders::BrightnessShader{128})
    .enableDestructiveShaders()
    .build();
```

### Rule 2.4 — addStrip() uses presets for discoverable configuration

Protocol and transport presets live in `lw::buses::presets`. Each preset is a plain struct with a `configure(BusBuilder&)` method. Fields can be overridden inline. Aggregate headers `ProtocolPresets.h` and `TransportPresets.h` collect all presets.

```cpp
using namespace lw::buses::presets;

auto bus = lw::buses::BusBuilder()
    .setPixelCount(30)
    .addStrip(ws2812x{}, nil_transport{})
    .build();

// Inline field override
auto bus2 = lw::buses::BusBuilder()
    .setPixelCount(30)
    .addStrip(ws2812x{.channelOrder = "RGB"}, nil_transport{})
    .build();
```

---

## 3  Storage Concept

### Rule 3.1 — StackBusStorage for static allocation

`StackBusStorage<NPixelCount, TProtocol, TTransport, TShaders...>` is a POD-like struct with compile-time-sized arrays. It is non-copyable and non-movable (internal references would dangle). Use it when dynamic allocation is unavailable (`-fno-rtti -fno-exceptions`) or when stack lifetime is preferred.

```cpp
// Declare at file scope or function scope — lives on the stack
lw::buses::StackBusStorage<30,
    lw::protocols::Ws2812xProtocol,
    lw::transports::NilTransport,
    lw::shaders::BrightnessShader> storage;

auto& bus = lw::buses::BusBuilder()
    .setPixelCount(30)
    .setTransport(lw::transports::NilTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .addShader(lw::shaders::BrightnessShader{128})
    .buildInto(storage);
```

`StackBusStorage` template parameters must match the builder configuration at compile time. Mismatch is a compile error.

### Rule 3.2 — build() for dynamic allocation

Use `build()` when pixel count is not known until runtime, or when ownership transfer via `unique_ptr` is desired. The returned `unique_ptr<IPixelBus>` can be stored, moved, or returned from a factory function.

```cpp
std::unique_ptr<lw::IPixelBus> makeStrip(int count)
{
    return lw::buses::BusBuilder()
        .setPixelCount(count)
        .setTransport(lw::transports::SpiTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
        .build();
}
```

### Rule 3.3 — Storage lifetime equals bus lifetime

Whether using `build()` (heap) or `buildInto()` (stack), the storage object must outlive all use of the `IPixelBus`. The `IPixelBus::pixels()` span points into storage-owned memory.

```cpp
lw::span<lw::Pixel> dangling()
{
    auto bus = lw::buses::BusBuilder()
        .setPixelCount(30)
        .setTransport(lw::transports::NilTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
        .build();

    return bus->pixels();  // BUG: span dangles after bus is destroyed
}
```

### Rule 3.4 — External pixel buffer lifetime

When using `setPixelStorage()`, the caller owns the pixel buffer. The external buffer must outlive the `IPixelBus` — the bus holds a non-owning `span` into it. This is the same lifetime contract as the low-level `Bus` constructor: the bus references external memory and does not manage its lifetime.

```cpp
// Correct — buffer declared before bus, destroyed after
std::array<lw::Pixel, 60> pixels{};
{
    auto bus = lw::buses::BusBuilder()
        .setPixelStorage(pixels)
        .setTransport(lw::transports::SpiTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
        .build();

    bus->show();  // ok — pixels still alive
}
// bus destroyed, pixels still valid for reuse

// Error — buffer destroyed before bus
// auto bus = []() {
//     std::array<lw::Pixel, 60> localPixels{};
//     return lw::buses::BusBuilder()
//         .setPixelStorage(localPixels)
//         .setTransport(...)
//         .setProtocol(...)
//         .build();
// }();  // BUG: localPixels destroyed, bus holds dangling span
```

When using `setPixelStorage()` with `build()`, only the bus machinery (pipelines, protocol buffers, shaders) is heap-allocated. The pixel buffer itself incurs no allocation — it is the caller's responsibility.

---

## 4  Transport and Protocol Requirements

### Rule 4.1 — Transport must be move-constructible

The builder takes transports by value (move). All built-in transports (`SpiTransport`, `RpPioTransport`, `RpSpiTransport`, `NilTransport`, etc.) satisfy this. Custom transports must also be move-constructible.

```cpp
// All built-in transports work:
builder.setTransport(lw::transports::SpiTransport{});           // default
builder.setTransport(lw::transports::RpPioTransport{});         // default
builder.setTransport(lw::transports::NilTransport{});           // default

// With settings:
lw::transports::TransportSettingsBase spiSettings;
spiSettings.dataPin = 7;
spiSettings.clockPin = 8;
spiSettings.clockRateHz = 4'000'000;
builder.setTransport(lw::transports::SpiTransport{spiSettings});
```

### Rule 4.2 — Protocol must expose `SettingsType` and `requiredBufferSize()`

The builder queries `TProtocol::SettingsType` for the settings parameter type and `TProtocol::requiredBufferSize(pixelCount, settings)` for buffer allocation. All built-in protocols satisfy this.

### Rule 4.3 — Shaders must be move-constructible and implement `IShader`

Shaders are stored in a type-erased list. They must derive from `lw::shaders::IShader` (or satisfy its duck-type: `apply(span<const Color>, span<Pixel>)`).

---

## 5  Comparison with Legacy Approaches

### Rule 5.1 — Prefer BusBuilder over manual Bus wiring

Do not manually allocate `Protocol`, `Transport`, `ProtocolTransportPipeline`, `ShaderProtocol`, protocol buffers, scratch buffers, shader pointer arrays, `PipelineRun` arrays, and `Bus` separately. Use `BusBuilder` instead.

```cpp
// ❌ Manual wiring — 7 objects, fragile lifetimes
std::array<lw::Pixel, 30> pixels{};
Ws2812xProtocol protocol(30);
NilTransport transport;
std::vector<uint8_t> buf(Ws2812xProtocol::requiredBufferSize(30));
ProtocolTransportPipeline pipeline(protocol, transport, buf);
PipelineRun runs[] = {{&pipeline, 30}};
Bus bus(pixels, runs);

// ✅ BusBuilder — 1 object, safe lifetimes
auto bus = lw::buses::BusBuilder()
    .setPixelCount(30)
    .setTransport(lw::transports::NilTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();
```

### Rule 5.2 — Prefer BusBuilder over PixelBus / StackPixelBus for new code

`PixelBus` and `StackPixelBus` remain available for backward compatibility but `BusBuilder` is the recommended API for new bus construction. The builder provides the same zero-overhead result with a simpler calling convention.

### Rule 5.3 — BusBuilder does not replace Bus for low-level composition

`Bus` remains the correct choice when you need to wire `OutputPipeline` subtypes that are not protocol+transport pairs (e.g., `PwmOutputPipeline` light drivers, or custom `OutputPipeline` implementations). `BusBuilder` is specifically for the `Protocol` → `Transport` pipeline pattern.

```cpp
// Bus with PwmOutputPipeline — BusBuilder does not cover this case
lw::Pixel pixels[1]{};
MockPipeline pwm;  // light driver, not protocol+transport
lw::buses::PipelineRun runs[] = {{&pwm, 1}};
lw::buses::Bus bus(lw::span<lw::Pixel>{pixels}, lw::span<const lw::buses::PipelineRun>{runs});
```

---

## 6  Anti-Patterns

### Anti-Pattern 6.1 — Storing a reference to a temporary builder

The fluent chain returns `BusBuilder&`. Do not store a reference and continue chaining elsewhere — the builder is move-only and the chain is meant to be consumed inline.

```cpp
// ❌ Storing a reference
auto& builder = lw::buses::BusBuilder().setPixelCount(30);
// builder is a dangling reference — the temporary is destroyed

// ✅ Consume inline
auto bus = lw::buses::BusBuilder()
    .setPixelCount(30)
    .setTransport(lw::transports::NilTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();
```

### Anti-Pattern 6.2 — Reusing a moved-from builder

```cpp
// ❌
auto builder = lw::buses::BusBuilder().setPixelCount(30);
auto bus1 = builder.build();
auto bus2 = builder.build();  // undefined behavior — builder was consumed

// ✅ Create a new builder for each bus
auto bus1 = lw::buses::BusBuilder().setPixelCount(30)...build();
auto bus2 = lw::buses::BusBuilder().setPixelCount(60)...build();
```

### Anti-Pattern 6.3 — Letting an external pixel buffer go out of scope before the bus

When using `setPixelStorage()`, the external buffer must outlive the `IPixelBus`. Destroying the buffer while the bus is still in use produces a dangling span — reads and writes corrupt unrelated memory.

```cpp
// ❌ External buffer destroyed before bus
std::unique_ptr<lw::IPixelBus> makeBus()
{
    std::array<lw::Pixel, 60> pixels{};  // local — will be destroyed on return
    return lw::buses::BusBuilder()
        .setPixelStorage(pixels)
        .setTransport(lw::transports::SpiTransport{})
        .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
        .build();  // BUG: bus references local 'pixels' that dies with this frame
}

// ✅ External buffer at stable scope
std::array<lw::Pixel, 60> gPixels{};  // file-scope or long-lived object

auto bus = lw::buses::BusBuilder()
    .setPixelStorage(gPixels)
    .setTransport(lw::transports::SpiTransport{})
    .setProtocol(lw::protocols::Ws2812xProtocol{}, {})
    .build();
```

---

## 7  Build Validation and Error Reporting

### Rule 7.1 — validate() checks configuration before build()

`validate()` returns an empty string on success, or an error description on failure. It does not allocate or consume the builder. `build()` calls `validate()` internally and returns `nullptr` on failure.

Checks performed:
1. Pixel count is set (`setPixelCount()` or `setPixelStorage()`)
2. A transport is set
3. A protocol is set
4. External pixel storage is non-empty (if used)

```cpp
auto err = lw::buses::BusBuilder()
    .setPixelCount(30)
    .setTransport(lw::transports::NilTransport{})
    .validate();
// err == "protocol not set"
```
3. All `addRun()` offsets are non-overlapping and within `[0, pixelCount)`
4. The sum of run lengths equals `pixelCount` (warning, not error)

Validation failures produce a clear error message via the project's error reporting mechanism.

### Rule 7.2 — build() is noexcept-false

`build()` may throw `std::bad_alloc` if dynamic allocation fails. On embedded targets without exceptions, `build()` is `[[nodiscard]]` and returns `nullptr` on allocation failure. Callers must check the result.
