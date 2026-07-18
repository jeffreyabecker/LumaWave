# Copilot Instructions for NpbNext

## Project Context

- Repository: `NpbNext`.
- Primary target: PlatformIO + Arduino core, with RP2040/Pico2W as the default workflow.
- Language standard for active code paths: C++17 (`-std=gnu++17`) in primary and native-test environments.
- Architecture is virtual-first and centered on explicit seams: `IPixelBus`, `Protocol`, and `Transport`.
- Project stage is alpha: API compatibility is not preserved by default and public APIs are assumed unstable unless explicitly stated otherwise.
- Do not introduce compatiblity shims or overloads to preserve old APIs; prefer direct API updates and test/example call site maintenance instead.
- For API changes, update call sites in tests/examples and remove obsolete APIs rather than introducing compatibility wrappers or alias overloads.
- For behavior or contract changes, run targeted native tests first, then broader suites as needed.




## C++ and Style Rules

- Compile target for active environments is C++17 (`-std=gnu++17`); do not introduce changes that require compiling project code as C++20/C++23.
- Follow existing project style: Allman braces, 4-space indentation, `#pragma once` in headers.
- Prefer qualified namespace declarations for nested namespaces when possible (for example `namespace lw::protocol { ... }`) instead of nested blocks (for example `namespace lw { namespace protocol { ... } }`).
- Do not introduce C++20-only surface features in active virtual-first headers.
	- Avoid `concept`, `requires`, and direct dependency on C++20-only APIs at public seam boundaries.
- Use the compatibility layer in `src/core/Compat.h`:
	- Prefer `lw::span` over direct `std::span` in project headers.
	- Prefer `lw::remove_cvref_t` over direct `std::remove_cvref_t` where compatibility matters.
- Avoid exceptions unless explicitly needed; keep hot paths simple and predictable.
- Keep virtual dispatch at seam boundaries and avoid per-pixel virtual overhead.
- Use STL algorithms and utilities where they improve clarity and maintainability without introducing unnecessary complexity or overhead.

## Architecture and Ownership Rules

- Keep responsibilities separated:
	- `IPixelBus`: storage/composition + frame lifecycle.
	- `IShader`: pixel transforms (see backlog 009).
	- `Protocol`: chip byte-stream encoding/framing.
	- `Transport`: hardware/peripheral transfer behavior.
- External lifetime model: `Bus`, `ProtocolTransportPipeline`, and `PipelineRun` are non-owning. The caller owns and manages protocol instances, transport instances, protocol encoding buffers, scratch pixel buffers, pixel storage, and pipeline run arrays. No heap allocation occurs inside these classes.
- For transport settings types, maintain required `public bool invert` contract.
- Use canonical static bus-driver naming already adopted by the codebase:
	- `PixelBus<...>`
	- `PixelBus<Protocol>` (template-default transport)
	- Do not reintroduce legacy `Owning*` bus-driver names.
  - Do not use the term `BusDriver` in new code this is a deprecacted legacy term that predates the current architecture and is not used in new code or tests.
	

## Arduino Dependency Boundary Rules

- Prefer minimizing `Arduino.h` dependency in virtual-first/core contracts.
- Keep direct platform calls (`micros`, `yield`, pin I/O, etc.) at transport/platform edges or narrow adapters.
- Do not add compatibility shims solely to preserve old Arduino-first consumer APIs during refactors.

## Bus Construction Rules

- Use direct `Bus(span<Color>, span<const PipelineRun>)` construction with caller-allocated resources.
- Pipelines: `ProtocolTransportPipeline` (protocol + transport pair) or a concrete light driver implementing `OutputPipeline`.
- `PipelineRun` = `{OutputPipeline*, size_t length}`. Single light: length=1. Strip: length=N.
- Multi-strip uses multiple `PipelineRun` entries in a caller-owned array with sub-view spans.
- `ProtocolTransportPipeline` constructor: `(Protocol&, Transport&, span<uint8_t> protocolBuffer)`.
- Wrap protocols with `ShaderProtocol` for brightness/shader support: `ShaderProtocol(Protocol&, span<IShader*>, span<Color> scratchPixels)`.
- Use `BrightnessShader` for brightness: add it to the shader chain and set via `setRuntimeConfig(RuntimeConfig::Brightness, &value)` on the pipeline protocol.
- Protocol buffer size: use `ConcreteProtocol::requiredBufferSize(pixelCount, settings)` (static method) to determine the minimum buffer size at compile time. An empty `scratchPixels` span disables the brightness/shader scratch path.
- No factory functions, no descriptor system, no template aliases.
- No `std::make_unique`, `std::unique_ptr`, or `std::initializer_list` in Bus/PipelineRun construction.

## Testing and Validation Rules

- Before creating tests or running compilation/test commands, first verify the code changes meet the intended design/behavior expectations from the relevant source-of-truth docs.
- For behavior or contract changes, run targeted native tests first, then broader suites as needed.
- Minimum high-value gates for contract-sensitive changes:
	- `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`
- For protocol byte-stream changes, validate against relevant protocol and byte-stream specs.

## Examples Guidance

- Author examples under `examples/` using direct `Bus(span, span<const PipelineRun>)` construction with caller-owned protocol, transport, and buffer instances.
- In examples, include only `#include <LumaWave.h>` (and `#include <Arduino.h>` when needed for sketch/runtime APIs).
- Do not include internal project headers (for example `transports/*`, `protocols/*`) from examples.
- If an example needs extra internal includes to compile, treat that as a public-surface gap and fix exposure through `src/LumaWave.h` instead of adding more includes in the example.
- Prefer unqualified symbols re-exported by `LumaWave.h` when suitable aliases already exist.
- If no adequate alias exists in `LumaWave.h`, fully namespace-qualified usage (for example `lw::...`) is allowed in examples instead of expanding the public alias surface just for example ergonomics.
- Unless the example is specifically about shader usage, prefer shader-less examples.
- Unless the example is about a specific protocol, use simple protocol constructors in examples to reduce visual noise and boilerplate.  

