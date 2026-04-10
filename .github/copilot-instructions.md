# Copilot Instructions for NpbNext

## Project Context

- Repository: `NpbNext`.
- Primary target: PlatformIO + Arduino core, with RP2040/Pico2W as the default workflow.
- Language standard for active code paths: C++17 (`-std=gnu++17`) in primary and native-test environments.
- Architecture is virtual-first and centered on explicit seams: `IPixelBus`, `IShader`, `IProtocol`, and `ITransport`.
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
	- `IShader`: pixel transforms.
	- `IProtocol`: chip byte-stream encoding/framing.
	- `ITransport`: hardware/peripheral transfer behavior.
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

## Factory and API Authoring Rules

- Favor the single-expression `makeBus(...)` composition style with clear protocol/transport config roles.
- Use protocol aliases (for example `Ws2812`) where available to reduce boilerplate.
- Require explicit color type only when inference cannot determine it.
- Avoid adding ambiguous overloads solely to hide required explicit template information.
- In `src/factory/MakeBus.h`, use timing-first one-wire overload ordering only (`..., OneWireTiming, transportConfig, ...`) and never add timing-last overloads.
- For API changes, update call sites in tests/examples and remove obsolete APIs rather than introducing compatibility wrappers or alias overloads.
- Treat unused compatibility shims as codebase pollution; delete them instead of carrying them forward.
- Keep construction order visible: color contract -> transport -> protocol -> bus -> optional shader/topology.

## Testing and Validation Rules

- Before creating tests or running compilation/test commands, first verify the code changes meet the intended design/behavior expectations from the relevant source-of-truth docs.
- For behavior or contract changes, run targeted native tests first, then broader suites as needed.
- Minimum high-value gates for contract-sensitive changes:
	- `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`
	- `ctest --test-dir build -R test_factory_descriptor_first_pass_compile --output-on-failure`
- For protocol byte-stream changes, validate against relevant protocol and byte-stream specs.
- For shader coverage policy in strict deterministic tests:
	- In scope: `CurrentLimiterShader`, `AggregateShader`
	- Out of scope by default: `GammaShader`, `AutoWhiteBalanceShader`

## Examples Guidance

- Author examples under `examples/` with explicit layer declarations (`Protocol`, `Transport`, `BusType`).
- In examples, include only `#include <LumaWave.h>` (and `#include <Arduino.h>` when needed for sketch/runtime APIs).
- Do not include internal project headers (for example `factory/*`, `transports/*`, `protocols/*`) from examples.
- If an example needs extra internal includes to compile, treat that as a public-surface gap and fix exposure through `src/LumaWave.h` instead of adding more includes in the example.
- Prefer unqualified symbols re-exported by `LumaWave.h` when suitable aliases already exist.
- If no adequate alias exists in `LumaWave.h`, fully namespace-qualified usage (for example `lw::...`) is allowed in examples instead of expanding the public alias surface just for example ergonomics.
- Unless the example is specifically about shader usage, prefer shader-less examples.
- Unless the example is specifically about transport usage, prefer protocol-focused examples with transport defaults.
- Unless the example is specifically about Color depth, use the default color type and avoid explicit color template parameters in examples to reduce visual noise.
- Unless the example is about a specific protocol, use Ws2812 protocol aliases in examples to reduce visual noise and boilerplate.  

