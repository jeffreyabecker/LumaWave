# Runtime Settings & Shader Reintroduction — Design

This document defines the design for two interrelated features:

1. **Generalized runtime settings** — a uniform pattern for applying configuration changes to protocols and transports after construction.
2. **Shader reintroduction** — bringing back `IShader` as a pipeline-local pixel-transform seam, integrated into `ProtocolTransportPipeline`.

## Motivation

### Runtime Settings

Currently, protocol and transport settings are baked at construction and not designed to change afterward:

- `ProtocolSettings& settings()` returns a mutable reference, but there is no standard way to signal that settings have changed — no `apply()` or `reconfigure()` hook.
- `IHaveGain::setGain()` is the lone exception: a mixin interface with a dedicated virtual setter. This pattern does not scale to the diverse settings that different protocols and transports expose (timing parameters, channel order, SPI clock rate, idle-high behavior, etc.).
- The `Ws2812xProtocolSettings::applyTransportDefaults()` bridge is construction-time only.

Users need a general pattern: mutate a typed settings struct, then tell the component to rebuild internal state.

### Shader Reintroduction

Shaders were removed in backlog 003 ("Eliminate Shader Concept"). The motivation at the time was:

- The old shader integration was too tightly coupled to `PixelBus` (template parameter `TShader`, `UsesShaderScratch`, `brightnessOwnership()`).
- Brightness ownership was entangled between bus, shader, and protocol.
- Shader scratch buffers and virtual dispatch added complexity most users didn't need.

The conclusion of that backlog left the door open: *"re-add later if needed."* The key insight for reintroduction is that shaders should **not** be a bus-level concern. They belong in the pipeline, alongside protocol encoding and before transport transmission — the same place brightness already lives.

## Goals

- Define `IShader` as a minimal, pipeline-local seam for per-frame pixel transforms.
- Define `IReconfigurable` as an opt-in mixin for components that support runtime settings mutation.
- Integrate both into `ProtocolTransportPipeline` without changing `IPixelBus`, `IProtocol`, `ITransport`, or `Bus`.
- Keep the zero-shader fast path: when no shaders are present, `write()` behavior is identical to today.
- Keep brightness Bus-owned and applied in the pipeline, after shaders.
- No per-pixel virtual dispatch — shaders are called once per frame.
- No RTTI requirement — settings access uses `static_cast` with caller knowledge of concrete types.

## Non-Goals

- Do not add a shader template parameter to `Bus` or `IPixelBus`.
- Do not reintroduce `brightnessOwnership()` or ownership entanglement.
- Do not create a unified settings type — heterogeneous settings per protocol/transport is intentional and correct.
- Do not add RTTI-dependent downcast (`dynamic_cast`).
- Do not change the `IProtocol` or `ITransport` virtual seams.
- Do not change `Bus::show()` or the `PipelineRun` structure.

---

## Proposed Architecture

### 1. `IShader` — Pixel Transform Seam (`src/core/IShader.h`, new)

```cpp
namespace lw
{

class IShader
{
public:
    virtual ~IShader() = default;

    /// Apply a per-frame pixel transform in-place.
    /// Called once per show() per pipeline run.
    /// @param pixels  mutable span of the pixels to transform.
    virtual void apply(span<colors::Color> pixels) = 0;
};

} // namespace lw
```

**Design decisions:**

- **In-place transform.** Shaders mutate the span directly. No allocation, no return value, no separate input/output buffers.
- **Frame granularity.** One virtual call per shader per `show()`, not per pixel. Per-pixel loops are inline inside each shader's `apply()`.
- **No `begin()`.** Shaders needing setup (e.g., gamma lookup tables) do it in their constructor. If a future shader needs deferred init, `begin()` can be added later without breaking the interface (it would be a new virtual with a default no-op — but that changes the vtable, so we defer it).
- **Composition via list.** An ordered `std::vector<std::unique_ptr<IShader>>` replaces the old `AggregateShader`/`CompositeShader` template machinery. Shaders apply sequentially: output of shader N feeds input of shader N+1.

### 2. `IReconfigurable` — Runtime Settings Mixin (`src/core/IReconfigurable.h`, new)

```cpp
namespace lw
{

class IReconfigurable
{
public:
    virtual ~IReconfigurable() = default;

    /// Called after settings fields have been mutated at runtime.
    /// The component should rebuild any derived internal state
    /// (timing constants, hardware registers, lookup tables, etc.).
    virtual void reconfigure() {}
};

} // namespace lw
```

**Design decisions:**

- **Opt-in.** Components that don't support runtime reconfiguration simply don't inherit `IReconfigurable`. The pipeline checks with a `static_cast` (or a helper) and calls `reconfigure()` only when available.
- **Default no-op.** A component that inherits `IReconfigurable` but has nothing to rebuild can leave the default.
- **No data in the interface.** `IReconfigurable` carries no settings — it is purely a notification hook. The actual settings struct lives on the concrete class, accessed via the existing `settings()` pattern.
- **Relationship to `IHaveGain`.** `IHaveGain` remains a separate mixin for gain-specific protocols (APA102, HD108). A protocol can inherit both. `IHaveGain::setGain()` is a typed setter; `IReconfigurable::reconfigure()` is the general "apply changes" hook. They serve different purposes and do not conflict.

### 3. `IOutputPipeline` — One Addition (`src/core/IOutputPipeline.h`)

Add `reconfigure()` as a virtual with a default no-op:

```cpp
class IOutputPipeline
{
public:
    using BrightnessType = lw::colors::ColorComponent;

    virtual ~IOutputPipeline() = default;
    virtual void begin() = 0;
    virtual bool isReadyToUpdate() const = 0;
    virtual void write(span<const lw::colors::Color> colors, BrightnessType brightness) = 0;
    virtual bool alwaysUpdate() const { return false; }

    /// Propagate reconfiguration to pipeline components.
    /// Default no-op; ProtocolTransportPipeline overrides.
    virtual void reconfigure() {}
};
```

Light driver implementations (e.g., `NilLightDriver`, `RpPwmLightDriver`) don't override this — they get the default no-op. Only `ProtocolTransportPipeline` does.

### 4. `ProtocolTransportPipeline` — Shader & Settings Hub

This is the core integration point. It **owns** the shader list, runs the shader pass during `write()`, and propagates `reconfigure()`.

```cpp
namespace lw::buses
{

class ProtocolTransportPipeline : public IOutputPipeline
{
public:
    using BrightnessType = IOutputPipeline::BrightnessType;

    /// Construct with protocol + transport (no shaders).
    ProtocolTransportPipeline(
        std::unique_ptr<protocols::IProtocol> protocol,
        std::unique_ptr<transports::ITransport> transport);

    /// Construct with protocol + transport + shader list.
    ProtocolTransportPipeline(
        std::unique_ptr<protocols::IProtocol> protocol,
        std::unique_ptr<transports::ITransport> transport,
        std::vector<std::unique_ptr<IShader>> shaders);

    void begin() override;
    bool isReadyToUpdate() const override;
    bool alwaysUpdate() const override;
    void write(span<const lw::colors::Color> colors, BrightnessType brightness) override;

    /// Propagate reconfiguration to protocol and transport.
    void reconfigure() override;

    // Accessors for runtime settings manipulation.
    protocols::IProtocol*  protocol()  { return _protocol.get(); }
    transports::ITransport* transport() { return _transport.get(); }

private:
    std::unique_ptr<protocols::IProtocol>  _protocol;
    std::unique_ptr<transports::ITransport> _transport;
    std::vector<std::unique_ptr<IShader>>  _shaders;          // NEW: ordered shader pipeline
    std::vector<uint8_t>                   _protocolBuffer;   // existing
    std::vector<lw::colors::Color>         _scratchPixel;     // existing (reused for shaders)
};

} // namespace lw::buses
```

#### `write()` — Updated Data Flow

```
1. Null/empty guard (unchanged).
2. Protocol buffer resize (unchanged).
3. SHADER PASS (NEW):
   if _shaders is non-empty:
       copy colors → _scratchPixel
       for each shader in _shaders:
           shader->apply(_scratchPixel)
4. BRIGHTNESS + ENCODE (updated):
   if brightness != max:
       if _scratchPixel was not already populated by shader pass:
           copy colors → _scratchPixel  (existing brightness path)
       apply brightness per-channel on _scratchPixel
       protocol->update(_scratchPixel, _protocolBuffer)
   else if shader pass populated _scratchPixel:
       protocol->update(_scratchPixel, _protocolBuffer)
   else:
       protocol->update(colors, _protocolBuffer)  (existing fast path)
5. Transmit (unchanged).
```

**Key invariants:**

- **Zero-shader fast path is preserved.** When `_shaders` is empty and brightness is max, `_scratchPixel` is never touched — pixels go directly from Bus to protocol.
- **Single scratch buffer.** `_scratchPixel` is reused for both shader output and brightness application. When shaders are present, shader output stays in `_scratchPixel` and brightness is applied on top. No second allocation.
- **Shader output feeds brightness.** This is the correct semantic order: color correction (gamma, white balance) happens before master brightness attenuation.

#### `reconfigure()` — Propagation

```cpp
void ProtocolTransportPipeline::reconfigure() override
{
    // Reconfigure protocol if it supports it.
    if (auto* r = dynamic_cast<IReconfigurable*>(_protocol.get()))
        r->reconfigure();

    // Reconfigure transport if it supports it.
    if (auto* r = dynamic_cast<IReconfigurable*>(_transport.get()))
        r->reconfigure();
}
```

Wait — this uses `dynamic_cast`, which requires RTTI. Since the project avoids RTTI, we use a different approach.

#### `reconfigure()` — RTTI-Free Approach

Since `IProtocol` and `ITransport` are abstract bases with virtual destructors, and the concrete classes are known at the pipeline construction site, we can use a pointer-to-base pattern without RTTI:

```cpp
void ProtocolTransportPipeline::reconfigure() override
{
    // _protocol and _transport are stored as unique_ptr<IProtocol>/unique_ptr<ITransport>.
    // We cannot static_cast from base to derived without knowing the concrete type.
    // Instead, we store an optional reconfigure callback per component.
}

// Alternative: store raw function pointers set at construction:
class ProtocolTransportPipeline : public IOutputPipeline
{
    // ...
private:
    void (*_protocolReconfigure)(void*) = nullptr;
    void* _protocolReconfigureArg = nullptr;
    void (*_transportReconfigure)(void*) = nullptr;
    void* _transportReconfigureArg = nullptr;
};
```

This is awkward. A cleaner approach:

**Option A: Virtual `reconfigure()` on `IProtocol` and `ITransport`.**

Add `virtual void reconfigure() {}` directly to `IProtocol` and `ITransport` as a default no-op. No new interface, no RTTI, no stored function pointers. Protocols/transports that need it override; others don't.

- Pro: Zero storage overhead. Single virtual dispatch. No new types.
- Con: Adds a virtual to the two core seams. But it's a default no-op — no behavior change for existing implementations.

**Option B: `IReconfigurable` mixin, detected at pipeline construction.**

At construction time, try a `static_cast<IReconfigurable*>` on the raw pointer before it's wrapped in `unique_ptr`. Store a `IReconfigurable*` or `nullptr` per component.

- Pro: Keeps `IProtocol` and `ITransport` clean.
- Con: Requires storage of two extra pointers (16 bytes on 32-bit, 32 bytes on 64-bit). Requires construction-time detection.

**Recommendation: Option A.** The default no-op virtual on `IProtocol` and `ITransport` is the simplest, most maintainable approach. It follows the same pattern as `ITransport::beginTransaction()` / `endTransaction()` — optional hooks with default no-ops. `IReconfigurable` as a separate type adds complexity for no real benefit.

Revised proposal:

```cpp
// In IProtocol.h
class IProtocol
{
public:
    // ... existing ...
    virtual void reconfigure() {}  // NEW: called after settings mutation
};

// In ITransport.h
class ITransport
{
public:
    // ... existing ...
    virtual void reconfigure() {}  // NEW: called after settings mutation
};
```

And `ProtocolTransportPipeline::reconfigure()` becomes trivial:

```cpp
void ProtocolTransportPipeline::reconfigure() override
{
    if (_protocol)  _protocol->reconfigure();
    if (_transport) _transport->reconfigure();
}
```

### 5. Settings Access Pattern

The user accesses typed settings through the pipeline's accessors:

```cpp
// User constructs the pipeline and bus
auto pipeline = std::make_unique<ProtocolTransportPipeline>(
    std::make_unique<Ws2812xProtocol>(count, Ws2812xProtocolSettings{...}),
    std::make_unique<RpPioTransport>(RpPioTransportSettings{...}));

Bus bus(pixels, {{std::move(pipeline), count}});
bus.begin();

// Later, at runtime: change protocol timing
auto& runs = bus.runs();
auto* ptp = static_cast<ProtocolTransportPipeline*>(runs[0].pipeline.get());
auto* ws2812 = static_cast<Ws2812xProtocol*>(ptp->protocol());

ws2812->settings().timing.idleHigh = true;
ptp->reconfigure();  // → ws2812->reconfigure() rebuilds timing state
```

**Why `static_cast` is safe:** The caller constructed the pipeline and knows the concrete protocol and transport types. The downcast from `IProtocol*` → `Ws2812xProtocol*` is deterministic. No RTTI needed.

**Alternative convenience (future):** If this pattern becomes common, a template accessor on `ProtocolTransportPipeline` could wrap the cast:

```cpp
template<typename T>
T* protocolAs() { return static_cast<T*>(_protocol.get()); }
```

But that's sugar; start without it.

### 6. Construction — Shader List is Optional

Two constructors preserve backward compatibility:

```cpp
// No shaders — identical to today's constructor
ProtocolTransportPipeline(
    std::unique_ptr<protocols::IProtocol> protocol,
    std::unique_ptr<transports::ITransport> transport);

// With shaders — shader list is moved in
ProtocolTransportPipeline(
    std::unique_ptr<protocols::IProtocol> protocol,
    std::unique_ptr<transports::ITransport> transport,
    std::vector<std::unique_ptr<IShader>> shaders);
```

Example with shaders:

```cpp
std::vector<std::unique_ptr<IShader>> shaders;
shaders.push_back(std::make_unique<GammaShader>(2.2f));
shaders.push_back(std::make_unique<WhiteBalanceShader>(6500));

auto pipeline = std::make_unique<ProtocolTransportPipeline>(
    std::make_unique<Ws2812xProtocol>(count, Ws2812xProtocolSettings{}),
    std::make_unique<RpPioTransport>(RpPioTransportSettings{}),
    std::move(shaders));

Bus bus(pixels, {{std::move(pipeline), count}});
```

### 7. Protocol/Transport Participation in `reconfigure()`

Protocols and transports opt into runtime reconfiguration by overriding `reconfigure()`:

```cpp
class Ws2812xProtocol : public IProtocol
{
    Ws2812xProtocolSettings _settings;
    Ws2812xTiming _timingNs;  // derived from _settings

public:
    void reconfigure() override
    {
        _timingNs = computeTiming(_settings);
    }
};
```

Protocols/transports that don't need runtime reconfiguration leave the default no-op.

---

## Data Flow Comparison

### Current (`show()` with brightness)

```
Bus::show()
  └→ PipelineRun::write(span, brightness)
       └→ ProtocolTransportPipeline::write():
            1. Resize protocol buffer if needed
            2. If brightness != max: copy → scratch, apply brightness, encode
               Else: encode directly
            3. Transmit
```

### Proposed (`show()` with shaders + brightness)

```
Bus::show()
  └→ PipelineRun::write(span, brightness)
       └→ ProtocolTransportPipeline::write():
            1. Resize protocol buffer if needed
            2. If shaders present: copy → scratch, shader[0]→...→shader[N]
            3. If brightness != max:
                 copy → scratch (if not already there from step 2)
                 apply brightness per-channel
                 encode from scratch
               Else if scratch populated (shaders ran):
                 encode from scratch
               Else:
                 encode directly from input (ZERO-COPY FAST PATH)
            4. Transmit
```

The zero-copy fast path (no shaders, brightness=max) is preserved exactly.

---

## Summary of Changes

| Component | File | Change |
|---|---|---|
| `IShader` | `src/core/IShader.h` (new) | `apply(span<Color>)` virtual |
| `IProtocol` | `src/protocols/IProtocol.h` | +`virtual void reconfigure() {}` |
| `ITransport` | `src/transports/ITransport.h` | +`virtual void reconfigure() {}` |
| `IOutputPipeline` | `src/core/IOutputPipeline.h` | +`virtual void reconfigure() {}` |
| `ProtocolTransportPipeline` | `src/buses/ProtocolTransportPipeline.h` | +shader list, +`reconfigure()` override, +`protocol()`/`transport()` accessors, +shader-aware `write()` |
| `IPixelBus` | `src/core/IPixelBus.h` | No change |
| `Bus` | `src/buses/Bus.h` | No change |
| Concrete protocols | `src/protocols/*.h` | Opt-in `reconfigure()` overrides |
| Concrete transports | `src/transports/*.h` | Opt-in `reconfigure()` overrides |

---

## Implementation Phases

### Phase 1 — Interfaces

1. Create `src/core/IShader.h` with `IShader` class.
2. Add `virtual void reconfigure() {}` to `IProtocol`, `ITransport`, and `IOutputPipeline`.
3. Update `LumaWave.h` to include `IShader.h` and expose `using IShader = lw::IShader;`.
4. Build and run tests — verify no breakage (all changes are additive).

### Phase 2 — ProtocolTransportPipeline Integration

1. Add `_shaders` member and second constructor to `ProtocolTransportPipeline`.
2. Add `protocol()` and `transport()` accessors.
3. Implement `reconfigure()` override.
4. Update `write()` with shader pass logic.
5. Add unit tests for `ProtocolTransportPipeline` with 0, 1, and N shaders.
6. Add unit test for `reconfigure()` propagation.

### Phase 3 — Concrete Shader Implementations (stretch)

1. Implement `GammaShader` as first concrete shader.
2. Implement `WhiteBalanceShader` or `KelvinShader`.
3. Add integration tests and examples.

### Phase 4 — Protocol/Transport `reconfigure()` Opt-Ins (stretch)

1. Add `reconfigure()` overrides to protocols with mutable settings (Ws2812x, APA102, etc.).
2. Add `reconfigure()` overrides to transports with mutable settings (SPI clock, pins, etc.).
3. Add tests for runtime settings mutation + reconfigure.

---

## Open Decisions

| Decision | Status | Notes |
|---|---|---|
| `IShader::begin()` | **Defer.** | Start without it. Add later if a concrete shader needs deferred initialization. Adding a new pure virtual would break existing shaders; adding a default no-op virtual is safe but changes vtable. |
| Scratch buffer strategy for shader+brightness | **Single buffer.** | `_scratchPixel` serves both. Shaders write to it; brightness is applied on top. This is correct ordering (color correction before master attenuation). |
| `reconfigure()` on `IProtocol`/`ITransport` vs. separate `IReconfigurable` mixin | **On the seams directly.** | Simpler, follows `beginTransaction()`/`endTransaction()` precedent, no extra storage. |
| Shader composition (ordered list vs. tree) | **Ordered list.** | `vector<unique_ptr<IShader>>`. Simple, predictable, matches 99% of use cases. Old `CompositeShader` template variadic composition is not reintroduced. |
| Exposure of concrete shader types in `LumaWave.h` | **Per-implementation decision.** | `IShader` is exposed. Concrete shaders like `GammaShader` are exposed when they exist and are stable. |
