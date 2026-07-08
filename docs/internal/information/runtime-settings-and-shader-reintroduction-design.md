# Runtime Settings & Shader Reintroduction — Design

This document defines the design for two interrelated features:

1. **Runtime parameters** — frame-variant values like brightness and gamma, owned by the user (or `Bus`) and bound to shaders via constructor references.
2. **Shader reintroduction** — bringing back `IShader` as a pipeline-local pixel-transform seam, integrated into `ProtocolTransportPipeline`.

## Motivation

Currently, brightness is applied via an inline per-pixel loop inside `ProtocolTransportPipeline::write()`. It is the only frame-variant pipeline behavior, hard-coded into one implementation. Adding gamma correction, color temperature, or power limiting would require more inline code or a different mechanism.

Shaders were removed in backlog 003. The old integration placed shaders as a `TShader` template parameter on `Bus`, entangled `brightnessOwnership()` across three seams, and carried `AggregateShader`/`CompositeShader` template machinery. The new design avoids all of that.

## Design Principle: Reference-Bound Configuration

Instead of threading a parameter struct through the pipeline at write time, shaders bind to external configuration values via constructor references:

```cpp
// User declares and owns the variable.
BrightnessType myBrightness = 255;

// Shader binds to it at construction.
auto shader = std::make_unique<BrightnessShader>(myBrightness);

// User changes the variable directly.
myBrightness = 128;

// Next show() — shader reads the current value through its reference.
```

This means:
- **No parameter struct passed through the pipeline.** Shaders get what they need at construction time.
- **No notification hooks.** Mutating the external variable is sufficient — the reference makes the new value visible on next `apply()`.
- **No pipeline knowledge of configuration values.** The pipeline just runs shaders. It doesn't know about brightness, gamma, or anything else.
- **Explicit wiring.** The user decides which shader binds to which variable. No magic auto-injection.

## Goals

- Define `IShader` as a minimal, pipeline-local seam: `virtual void apply(span<Color>) = 0`.
- `BrightnessShader` binds to a `const BrightnessType&` at construction.
- Remove the inline brightness loop from `ProtocolTransportPipeline::write()`. All pixel transforms go through shaders.
- `ProtocolTransportPipeline` owns `vector<unique_ptr<IShader>>` and runs them in order. No auto-injection, no brightness knowledge.
- `Bus` stores `BrightnessType _brightness` (as today). `setBrightness()` updates it. A `BrightnessShader` can reference `bus.brightnessRef()` if desired.
- `IOutputPipeline::write()` takes `span<const Color>` — no brightness, no params. The pipeline is a pure shader → encode → transmit engine.
- Keep the zero-shader fast path: when `_shaders` is empty, no scratch buffer is allocated.
- `IPixelBus` and `Bus` contracts remain backward compatible.

## Non-Goals

- Do not add a shader template parameter to `Bus` or `IPixelBus`.
- Do not reintroduce `brightnessOwnership()`, `UsesShaderScratch`, `AggregateShader`, `CompositeShader`, or any old shader machinery.
- Do not add `IShader::begin()`.
- Do not pass configuration values through `write()` or `apply()` — shaders bind at construction.
- Do not change construction-time settings (protocol/transport settings structs).

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
    /// Configuration values are bound at construction via reference.
    virtual void apply(span<colors::Color> pixels) = 0;
};

} // namespace lw
```

**Design decisions:**

- **No configuration parameters in `apply()`.** Shaders get external values bound via constructor references. This keeps `apply()` stable as new parameters are added — the interface never changes.
- **In-place transform.** Shaders mutate the span directly.
- **Frame granularity.** One virtual call per shader per `show()`. Per-pixel loops are inline inside `apply()`.
- **No `begin()`.** Deferred. Add later if needed as a default no-op virtual.
- **Composition via list.** An ordered `std::vector<std::unique_ptr<IShader>>` in the pipeline.

### 2. `BrightnessShader` — Reference-Bound Brightness (`src/core/BrightnessShader.h`, new)

```cpp
namespace lw
{

class BrightnessShader : public IShader
{
public:
    /// @param brightnessRef  reference to the user-owned brightness value.
    explicit BrightnessShader(const BrightnessType& brightnessRef)
        : _brightness(brightnessRef) {}

    void apply(span<colors::Color> pixels) override
    {
        const auto b = _brightness;
        if (b == std::numeric_limits<BrightnessType>::max())
            return;  // no-op fast path

        for (auto& pixel : pixels)
        {
            for (char ch : {'R', 'G', 'B', 'W'})
                pixel[ch] = static_cast<lw::colors::ColorComponent>(
                    lw::colors::applyBrightness(pixel[ch], b));
        }
    }

private:
    const BrightnessType& _brightness;
};

} // namespace lw
```

**Design decisions:**

- **`const&` to external variable.** The shader reads the current value every `apply()`. Mutating the external variable is instantaneous — no notification needed.
- **No-op when max.** If `_brightness` is max, the shader returns immediately without touching pixels. This preserves the zero-copy fast path at the pipeline level (see section 5).
- **User owns the variable.** The variable must outlive the shader. Typical patterns:
  - `Bus` member: `BrightnessShader(bus.brightnessRef())`
  - Static/global: `BrightnessShader(g_brightness)`
  - Heap-allocated config struct: `BrightnessShader(config->brightness)`

### 3. `IOutputPipeline` — Simplified (`src/core/IOutputPipeline.h`)

Remove the `BrightnessType` parameter from `write()`. The pipeline doesn't know about brightness at all.

```cpp
class IOutputPipeline
{
public:
    virtual ~IOutputPipeline() = default;
    virtual void begin() = 0;
    virtual bool isReadyToUpdate() const = 0;
    virtual void write(span<const lw::colors::Color> colors) = 0;
    virtual bool alwaysUpdate() const { return false; }
};
```

**Impact:** All `IOutputPipeline` implementations lose their `BrightnessType` parameter. Light drivers become simpler. `ProtocolTransportPipeline` becomes pure shader → encode → transmit.

### 4. `IPixelBus` and `Bus` — Minimal Change

`IPixelBus` is unchanged. `Bus` keeps `_brightness` as today. `setBrightness()` updates it. No parameters struct needed.

```cpp
class Bus : public IPixelBus
{
public:
    // ... constructors unchanged ...

    void begin() override;
    void show() override;
    bool isReadyToUpdate() const override;

    span<lw::colors::Color>& pixels() override;
    const span<lw::colors::Color>& pixels() const override;

    void setBrightness(BrightnessType brightness) override { _brightness = brightness; _dirty = true; }
    BrightnessType brightness() const override { return _brightness; }

    // Expose a reference for BrightnessShader binding.
    const BrightnessType& brightnessRef() const { return _brightness; }

private:
    span<lw::colors::Color> _pixels;
    std::vector<PipelineRun> _runs;
    BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
    bool _dirty{true};
};
```

**Usage pattern:**

```cpp
Bus bus(pixels, {{std::move(pipeline), count}});
auto brightnessShader = std::make_unique<BrightnessShader>(bus.brightnessRef());
// ... pipe must be reconstructed to include the shader, or
// the user constructs the pipeline with shaders from the start:

auto pipeline = std::make_unique<ProtocolTransportPipeline>(
    std::move(protocol), std::move(transport),
    std::make_unique<BrightnessShader>(bus.brightnessRef()));

Bus bus(pixels, {{std::move(pipeline), count}});

// Runtime: brightness change is instant.
bus.setBrightness(128);  // next show() sees it via the shader's reference.
```

### 5. `ProtocolTransportPipeline` — Pure Shader Engine

No brightness knowledge. No inline brightness loop. Just shader → encode → transmit.

```cpp
namespace lw::buses
{

class ProtocolTransportPipeline : public IOutputPipeline
{
public:
    // No-shader constructor.
    ProtocolTransportPipeline(
        std::unique_ptr<protocols::IProtocol> protocol,
        std::unique_ptr<transports::ITransport> transport);

    // Shader constructor — one or more shaders.
    ProtocolTransportPipeline(
        std::unique_ptr<protocols::IProtocol> protocol,
        std::unique_ptr<transports::ITransport> transport,
        std::vector<std::unique_ptr<IShader>> shaders);

    void begin() override;
    bool isReadyToUpdate() const override;
    bool alwaysUpdate() const override;
    void write(span<const lw::colors::Color> colors) override;

private:
    std::unique_ptr<protocols::IProtocol>  _protocol;
    std::unique_ptr<transports::ITransport> _transport;
    std::vector<std::unique_ptr<IShader>>  _shaders;
    std::vector<uint8_t>                   _protocolBuffer;
    std::vector<lw::colors::Color>         _scratchPixel;
};

} // namespace lw::buses
```

#### `write()` — Data Flow

```
1. Null/empty guard (unchanged).
2. Protocol buffer resize (unchanged).
3. SHADER PASS:
   if _shaders is non-empty:
       copy colors → _scratchPixel
       for each shader in _shaders:
           shader->apply(_scratchPixel)
       protocol->update(_scratchPixel, _protocolBuffer)
   else:
       protocol->update(colors, _protocolBuffer)   (ZERO-COPY FAST PATH)
4. Transmit (unchanged).
```

**Key invariant:** When `_shaders` is empty, `_scratchPixel` is never allocated or touched — the zero-copy fast path is unconditional.

### 6. Other Shader Examples

#### CurrentLimiterShader (reference-bound)

```cpp
class CurrentLimiterShader : public IShader
{
public:
    struct Settings {
        uint32_t maxMilliamps = 0;
        uint16_t controllerMilliamps = 100;
        uint16_t standbyMilliampsPerPixel = 1;
        bool rgbwDerating = true;
        // Per-channel milliamps default to 20mA.
    };

    explicit CurrentLimiterShader(const Settings& settings)
        : _settings(settings) {}

    void apply(span<colors::Color> pixels) override { /* power-budget logic */ }

    uint32_t lastEstimatedMilliamps() const { return _lastEstimatedMilliamps; }

private:
    const Settings& _settings;   // or by-value copy if runtime mutation not needed
    uint32_t _lastEstimatedMilliamps{0};
};
```

#### GammaShader (baked at construction, no runtime reference)

```cpp
class GammaShader : public IShader
{
    std::array<uint8_t, 256> _lut;
public:
    explicit GammaShader(float gamma)
    {
        for (int i = 0; i < 256; ++i)
            _lut[i] = std::pow(i / 255.0f, gamma) * 255.0f + 0.5f;
    }

    void apply(span<colors::Color> pixels) override
    {
        for (auto& pixel : pixels)
            for (char ch : {'R', 'G', 'B'})
                pixel[ch] = _lut[pixel[ch]];
    }
};
```

GammaShader doesn't need a reference — gamma is baked into a LUT at construction. If runtime gamma is needed later, add a `const float&` constructor parameter and recompute.

#### AggregateShader

```cpp
class AggregateShader : public IShader
{
public:
    explicit AggregateShader(std::vector<std::unique_ptr<IShader>> shaders)
        : _shaders(std::move(shaders)) {}

    void apply(span<colors::Color> pixels) override
    {
        for (auto& shader : _shaders)
            shader->apply(pixels);
    }

    void add(std::unique_ptr<IShader> shader) { _shaders.push_back(std::move(shader)); }
    bool empty() const { return _shaders.empty(); }
    size_t size() const { return _shaders.size(); }
    void clear() { _shaders.clear(); }

private:
    std::vector<std::unique_ptr<IShader>> _shaders;
};
```

---

## Data Flow Comparison

### Current

```
Bus::show()
  └→ PipelineRun::write(subspan, _brightness)
       └→ ProtocolTransportPipeline::write(colors, brightness):
            1. If brightness != max: copy → scratch, apply brightness
            2. Encode
            3. Transmit
```

### Proposed

```
Bus::show()
  └→ PipelineRun::write(subspan)
       └→ ProtocolTransportPipeline::write(colors):
            1. If _shaders non-empty: copy → scratch,
               shader[0]->apply → shader[N]->apply
            2. Encode
            3. Transmit
```

No pipeline knows about brightness. If you want brightness, add a `BrightnessShader` to the shader list.

---

## Summary of Changes

| Component | Change | Impact |
|---|---|---|
| `IShader` (new) | `apply(span<Color>)` — pure pixels, no config params | New seam |
| `BrightnessShader` (new) | Binds to external `const BrightnessType&` at construction | Reference-based config |
| `IOutputPipeline` | `write(colors)` — no brightness parameter | Breaking change to all impls |
| `IPixelBus` | No change | |
| `Bus` | Keeps `_brightness`. Adds `brightnessRef()`. `setBrightness()` unchanged. | Backward compatible |
| `ProtocolTransportPipeline` | `_shaders` list. No inline brightness. `write(colors)` only. | Core integration point |
| `IProtocol` | No change | |
| `ITransport` | No change | |
| Light drivers | `write()` loses brightness param — mechanical change only | |

---

## Implementation Phases

### Phase 1 — IShader Interface + Pipeline Signature

1. Create `src/core/IShader.h` with `IShader` class.
2. Update `IOutputPipeline.h`: remove `BrightnessType` from `write()`. Signature becomes `write(span<const Color>)`.
3. Update all `IOutputPipeline` implementations (light drivers + `ProtocolTransportPipeline`): match new signature.
4. Create `src/core/BrightnessShader.h` with `BrightnessShader`.
5. Update `LumaWave.h`: add includes and `using` declarations for `IShader`, `BrightnessShader`.
6. Build and run tests.

### Phase 2 — ProtocolTransportPipeline Shader Integration

1. Add `_shaders` member and second constructor (takes `vector<unique_ptr<IShader>>`).
2. Rewrite `write()`: shader pass if non-empty (zero-copy fast path if empty), then encode, transmit. No inline brightness.
3. Build and run tests.

### Phase 3 — Bus Brightness Ref

1. Add `brightnessRef()` to `Bus`.
2. Build and run tests.

### Phase 4 — AggregateShader

1. Create `src/buses/AggregateShader.h`.
2. Add to `LumaWave.h`.
3. Build and verify compilation.

### Phase 5 — CurrentLimiterShader

1. Create `src/core/CurrentLimiterShader.h` with `CurrentLimiterShaderSettings` and `CurrentLimiterShader`.
2. Add to `LumaWave.h`.
3. Build and verify compilation.

### Phase 6 — Tests

1. Test `ProtocolTransportPipeline` with 0 shaders (zero-copy fast path).
2. Test `ProtocolTransportPipeline` with `BrightnessShader` — brightness scales correctly.
3. Test `BrightnessShader` standalone at various levels (0, 128, max).
4. Test `BrightnessShader` with max — verify no-op.
5. Test `AggregateShader` with 0, 1, N shaders.
6. Test `CurrentLimiterShader` — budget enforcement, RGBW derating, `lastEstimatedMilliamps()`.
7. Test `Bus::brightnessRef()` — reference tracks mutations.
8. Test end-to-end: Bus + ProtocolTransportPipeline + BrightnessShader + CurrentLimiterShader.
9. Run full suite.

### Phase 7 — Documentation

1. Update `.github/copilot-instructions.md`: Add `IShader` back to seam list.
2. Update `docs/internal/information/object-model-contracts.md`.
3. Mark backlog completed.

---

## Open Decisions

| Decision | Status | Notes |
|---|---|---|
| `IShader::begin()` | **Defer.** | No concrete shader needs it yet. Add later as default no-op if required. |
| BrightnessShader auto-injection | **No.** | Pipeline is pure shader engine. User explicitly adds shaders. |
| Zero-copy fast path | **`_shaders.empty()`.** | When no shaders are installed, input colors go directly to `protocol->update()` without scratch buffer allocation. |
| BrightnessShader reference vs. value | **`const&`** | Shader reads the current value every frame. Mutating the external variable is the notification mechanism. |
| `CompositeShader` variadic template | **Not reintroduced.** | `AggregateShader` + `vector` covers all use cases. |
| `CurrentLimiterShader` template parameter | **Removed.** | Works on `colors::Color` directly, not templated on `TColor`. |
