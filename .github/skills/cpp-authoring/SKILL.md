---
name: cpp-authoring
description: "Use when: writing or editing C++ in LumaWave — headers, sources, tests, examples. Covers style (Allman, 4-space, #pragma once), C++17 constraints, virtual-first architecture, Bus construction, protocol/transport seams, and testing gates."
---

# C++ Authoring for LumaWave

## Language Standard

- C++17 (`-std=gnu++17`) for all active code paths. Do **not** introduce C++20/C++23 requirements.
- Avoid C++20-only surface features in virtual-first headers: no `concept`, `requires`, or C++20-only APIs at public seam boundaries.
- Use the compatibility layer in `src/core/Compat.h`:
    - Prefer `lw::span` over `std::span` in project headers.
    - Prefer `lw::remove_cvref_t` over `std::remove_cvref_t` where compatibility matters.

## Style

- Allman braces, 4-space indentation, `#pragma once` in headers.
- Prefer qualified namespace declarations: `namespace lw::protocol { ... }` over nested `namespace lw { namespace protocol { ... } }`.
- Avoid exceptions unless explicitly needed; keep hot paths simple and predictable.
- Use STL algorithms and utilities where they improve clarity without unnecessary overhead.
- Run `clang-format` (project `.clang-format` in root) before committing.

## Architecture & Seams

- Virtual-first design, centered on explicit seams:
    - `IPixelBus` — storage/composition + frame lifecycle.
    - `IShader` — pixel transforms.
    - `Protocol` — chip byte-stream encoding/framing.
    - `Transport` — hardware/peripheral transfer behavior.
- Keep virtual dispatch at seam boundaries; avoid per-pixel virtual overhead.
- External lifetime model: `Bus`, `ProtocolTransportPipeline`, and `PipelineRun` are **non-owning**. The caller owns protocol, transport, buffers, scratch pixels, pixel storage, and pipeline run arrays. No heap allocation inside these classes.

## Bus Construction

- Direct construction: `Bus(span<Color>, span<const PipelineRun>)` with caller-allocated resources.
- `PipelineRun` = `{OutputPipeline*, size_t length}`. Single light: `length=1`. Strip: `length=N`. Multi-strip uses multiple entries in a caller-owned array.
- `ProtocolTransportPipeline` constructor: `(Protocol&, Transport&, span<uint8_t> protocolBuffer)`.
- For brightness/shader support, wrap with `ShaderProtocol(Protocol&, span<IShader*>, span<Color> scratchPixels)`. Use `BrightnessShader` + `setRuntimeConfig(RuntimeConfig::Brightness, &value)`.
- Protocol buffer size: use `ConcreteProtocol::requiredBufferSize(pixelCount, settings)` (static method). Empty `scratchPixels` span disables the shader scratch path.
- **Do not** use factory functions, descriptor systems, template aliases, `std::make_unique`, `std::unique_ptr`, or `std::initializer_list` in Bus/PipelineRun construction.
- Use canonical naming: `PixelBus<...>`, `PixelBus<Protocol>`. Do **not** use legacy `Owning*` or `BusDriver` names.

## Arduino Boundary

- Minimize `Arduino.h` dependency in virtual-first/core contracts.
- Keep platform calls (`micros`, `yield`, pin I/O) at transport/platform edges or narrow adapters.
- Do not add compatibility shims solely to preserve Arduino-first consumer APIs.

## Alpha-Stage API Policy

- API compatibility is **not** preserved by default. Public APIs are unstable.
- For API changes: update call sites in tests/examples and **remove** obsolete APIs. Do not introduce compatibility wrappers or alias overloads.

## Testing Gates

- For behavior/contract changes, run targeted native tests first, then broader suites.
- Minimum gate for contract-sensitive changes:
    ```
    cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
    ```
- For protocol byte-stream changes, validate against relevant protocol specs.

## Examples

- Author under `examples/` with direct `Bus(span, span<const PipelineRun>)` construction.
- Include only `#include <LumaWave.h>` (and `#include <Arduino.h>` when needed).
- Do **not** include internal headers (`transports/*`, `protocols/*`). If an example needs one, fix exposure through `src/LumaWave.h` instead.
- Prefer shader-less examples unless the example is specifically about shaders.
- Prefer simple protocol constructors unless the example is about a specific protocol.
