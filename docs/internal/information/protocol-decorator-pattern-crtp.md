# Protocol Decorator Pattern (CRTP)

This document defines the standard pattern for protocol decorators in LumaWave.

Decorator protocols are protocol classes that wrap another protocol and enrich behavior (for example tracing, instrumentation, transforms, policy checks) without changing the wrapped protocol implementation.

## Goals

- Keep decorators protocol-compatible (`Protocol<TColor>`).
- Avoid per-update virtual fan-out inside decorator internals.
- Keep settings explicit and composable.
- Preserve existing protocol construction and factory expectations.

## Required Contract

A protocol decorator must:

- derive from `Protocol<TColor>`;
- expose `using SettingsType = ...` where settings derives from `ProtocolSettings`;
- implement `ProtocolSettings& settings() override`;
- keep constructor shape `(uint16_t pixelCount, SettingsType settings)`;
- forward required base behavior (`begin`, `update`, `alwaysUpdate`, `requiredBufferSizeBytes` as appropriate).

## Standard CRTP Shape

Use CRTP to factor shared decorator plumbing into a reusable base.

```cpp
#pragma once

#include "protocols/Protocol.h"

namespace lw
{

    template <typename TColor>
    struct ProtocolDecoratorSettings : public ProtocolSettings
    {
    };

    template <typename TDerived,
              typename TWrappedProtocol,
              typename TColor,
              typename TSettings>
    class ProtocolDecoratorBase : public Protocol<TColor>
    {
    public:
        using WrappedProtocolType = TWrappedProtocol;
        using SettingsType = TSettings;

        ProtocolDecoratorBase(uint16_t pixelCount,
                              WrappedProtocolType wrapped,
                              SettingsType settings)
            : Protocol<TColor>(pixelCount)
            , _wrapped{std::move(wrapped)}
            , _settings{std::move(settings)}
        {
        }

        ProtocolSettings &settings() override
        {
            return _settings;
        }

        void begin() override
        {
            _wrapped.begin();
            static_cast<TDerived *>(this)->afterBegin();
        }

        void update(span<const TColor> colors,
                    span<uint8_t> buffer = span<uint8_t>{}) override
        {
            static_cast<TDerived *>(this)->beforeUpdate(colors, buffer);
            _wrapped.update(colors, buffer);
            static_cast<TDerived *>(this)->afterUpdate(colors, buffer);
        }

        bool alwaysUpdate() const override
        {
            return _wrapped.alwaysUpdate();
        }

        size_t requiredBufferSizeBytes() const override
        {
            return _wrapped.requiredBufferSizeBytes();
        }

    protected:
        WrappedProtocolType &_wrappedProtocol()
        {
            return _wrapped;
        }

        const WrappedProtocolType &_wrappedProtocol() const
        {
            return _wrapped;
        }

        SettingsType _settings;

    private:
        WrappedProtocolType _wrapped;
    };

} // namespace lw
```

## Derived Decorator Expectations

A derived CRTP decorator should provide no-op-safe hooks:

- `afterBegin()`
- `beforeUpdate(span<const TColor>, span<uint8_t>)`
- `afterUpdate(span<const TColor>, span<uint8_t>)`

Keep hooks side-effect minimal and deterministic.

## Buffer/Ownership Rules

- Do not assume ownership of protocol frame buffers unless the decorator explicitly documents ownership.
- If forwarding an external `buffer`, preserve the same buffer contract as wrapped protocol.
- If decorator needs extra scratch memory, expose this in settings and update `requiredBufferSizeBytes()` behavior accordingly.

## Settings Rules

- Decorator settings type must derive from `ProtocolSettings`.
- Keep settings POD-like when possible for easy movement/copying.
- If settings are mutable at runtime, mutate through typed access after obtaining `settings()` and casting.

## Composition Guidance

- Prefer thin, single-responsibility decorators.
- Compose multiple decorators by nesting wrapped protocol types.
- Keep protocol category/transport compatibility defined by the wrapped protocol path.
- Avoid introducing decorator-only protocol timing semantics unless explicitly required.

## Suggested First Adopters

- Debug/trace decorator (text output around begin/update).
- Metrics decorator (frame count, byte count, timing).
- Policy decorator (update throttling, guard conditions).

## Validation Checklist

- Compiles in `native-test` contract suites.
- Maintains `(uint16_t, SettingsType)` construction path.
- Correctly forwards or documents `requiredBufferSizeBytes()` behavior.
- `settings()` returns the concrete settings object via `ProtocolSettings&`.
