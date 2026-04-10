# C++23 Modules Rearchitecture Design

This document outlines how to reimagine LumaWave as a C++23 modules-first library.

The assumption for this design is that Arduino build targets are no longer first-class build environments. Each supported embedded platform moves to its native SDK and toolchain, and those toolchains are treated as C++23-capable targets. That means the design can optimize for named modules, explicit package boundaries, and platform-native integration instead of preserving Arduino-era include topology.

This is not a proposal to mechanically convert every header into an `export module` file. The goal is to use modules to express the architectural seams that the library already wants to preserve:

- `IPixelBus`
- `IShader`
- `IProtocol`
- `ITransport`

## Goals

- Make module boundaries the primary architecture boundary, not just directory conventions.
- Replace the monolithic umbrella-header mental model with explicit imports between stable subsystems.
- Treat host-native and embedded-native builds as peers under a common C++23 build model.
- Remove Arduino-specific dependency assumptions from core, protocol, shader, and generic transport code.
- Keep the existing virtual-first seam design while reducing transitive compile cost and dependency leakage.
- Create a path where platform transports bind directly to Pico SDK, ESP-IDF, and other native SDKs.

## Non-Goals

- Preserving Arduino compatibility layers or Arduino-centric include ergonomics.
- Exporting every internal helper as a public module surface.
- Rewriting the runtime object model around concepts-only or template-only composition.
- Forcing every platform SDK header to become a header unit on day one.
- Solving package distribution, versioning, and registry publication in this phase.

## Core Architectural Position

The important change is not `#include` to `import`. The important change is that the library stops pretending to be one flat header surface.

The current codebase already has the right conceptual seams. A modules-first design should formalize them as importable, layered packages with narrow export surfaces:

- foundational kernel and shared contracts
- color and pixel-domain model
- shader pipeline
- protocol codecs
- transport contracts and generic transports
- platform transport packs
- bus/runtime composition
- optional facade module for ergonomic imports

This preserves the current architecture while making dependency direction explicit in the compiler.

## Target Module Graph

The desired dependency direction is:

- `lw.core`: no dependency on higher layers
- `lw.color`: depends on `lw.core`
- `lw.shader`: depends on `lw.core`, `lw.color`
- `lw.protocol`: depends on `lw.core`, `lw.color`
- `lw.transport`: depends on `lw.core`
- `lw.bus`: depends on `lw.core`, `lw.color`, `lw.shader`, `lw.protocol`, `lw.transport`
- `lw.platform.*`: depends on `lw.transport`, plus SDK bridge modules and whichever lower layers are required by the implementation
- `lw.facade`: depends on the public modules above and exports the curated convenience surface

The rule should be simple: platform modules sit at the edge, bus composition sits above the seams, and the core kernel stays clean.

## Proposed Public Module Catalog

## 1. Core Kernel

### Primary module

- `lw.core`

### Responsibilities

- foundational utility aliases and compatibility policy that still matter under C++23
- bus lifecycle contracts
- shared index and topology primitives
- writable abstractions and other small, platform-agnostic support types

### Candidate contents

- `IPixelBus`
- `PixelView`
- `Topology`
- `IndexIterator`
- `Writable`
- narrow compatibility wrappers that remain justified under mixed toolchains

### Constraints

- no platform SDK imports
- no protocol implementations
- no shader implementations
- no concrete transports beyond no-op or test-safe primitives

## 2. Color Model

### Primary module

- `lw.color`

### Responsibilities

- color type definitions
- channel order and mapping semantics
- color math backends
- palette types and palette generation primitives

### Notes

This module should own color-domain rules and scalar math policy. It should not absorb shader behavior just because shaders manipulate colors.

## 3. Shader Pipeline

### Primary module

- `lw.shader`

### Responsibilities

- `IShader`
- pipeline-oriented frame transforms
- shader aggregation/composition
- deterministic frame-processing policies that happen before protocol encoding

### Notes

The current design already points in this direction. Modules make that separation enforceable.

## 4. Protocol Codec Layer

### Primary module

- `lw.protocol`

### Responsibilities

- `IProtocol`
- protocol aliases
- framing and encoding policies
- protocol decorators and protocol-support helpers

### Notes

One-wire encoding helpers that are conceptually protocol framing support should move here or into a protocol-support partition, not remain hidden under transport.

## 5. Transport Base Layer

### Primary module

- `lw.transport`

### Responsibilities

- `ITransport`
- transport settings contracts
- generic transports such as no-op, print, and host test transports
- transport-side buffer submission lifecycle

### Notes

This module is transport contract territory, not platform territory. It must not import Pico SDK, ESP-IDF, or any Arduino-era convenience header.

## 6. Bus Runtime Layer

### Primary module

- `lw.bus`

### Responsibilities

- `PixelBus`
- aggregate/composite bus composition
- bus-owned frame storage
- shader invocation order
- protocol update orchestration
- transport write orchestration

### Notes

This module becomes the runtime assembly layer. It is where the seam contracts meet.

## 7. Platform Transport Packs

### Primary modules

- `lw.platform.rp2040`
- `lw.platform.esp32`
- `lw.platform.esp8266` if retained
- `lw.platform.nrf52` if retained

### Responsibilities

- hardware transport implementations
- platform clock/timer/gpio adapters
- DMA, PIO, RMT, I2S, UART, SPI, PWM, or LED-controller bindings that are inherently SDK-specific

### Rules

- platform modules are not part of the minimal public kernel
- platform modules may use a global module fragment or non-exported implementation units to include C SDK headers
- platform modules should export only LumaWave-owned types, not raw SDK headers

## 8. Facade Module

### Primary module

- `lw`

### Responsibilities

- curated convenience re-exports for common consumers
- common aliases such as `Ws2812` and default bus composition helpers where those remain part of the design

### Notes

This is the replacement for the umbrella-header mindset. It should be intentionally curated, not a dump of every public declaration.

## Module Shape And Export Policy

The module design should prefer a small set of stable named modules over hundreds of micro-modules.

Recommended shape:

- one primary interface unit per public module
- internal partitions for large subsystems where that improves compile scalability
- non-exported implementation units for heavy code or SDK-bound code

For example:

- `lw.protocol` exports public protocol contracts and aliases
- `lw.protocol:one_wire` holds non-exported or selectively exported one-wire framing helpers
- `lw.platform.rp2040:pico_bridge` contains SDK integration details not intended as public API

The rule is that partitions are for internal organization. Public module names are the stable contract.

## Source Layout Direction

The repository does not need a perfect one-file-per-module mapping, but it does need a predictable structure. A reasonable target is:

```text
src/
  modules/
    lw.core.cppm
    lw.color.cppm
    lw.shader.cppm
    lw.protocol.cppm
    lw.transport.cppm
    lw.bus.cppm
    lw.cppm
    protocol/
      one_wire_support.cppm
    platform/
      rp2040/
        lw.platform.rp2040.cppm
        pico_bridge.cpp
      esp32/
        lw.platform.esp32.cppm
        esp_idf_bridge.cpp
```

Headers do not need to disappear immediately. During migration, some implementation detail headers may remain private and be included from module implementation units. The key requirement is that public consumption shifts to modules, not that every file instantly changes form.

## Build System Direction

The build system should treat modules as first-class build artifacts.

## Native host builds

- CMake becomes the canonical build system for host-native development and tests.
- Public modules are declared with `FILE_SET CXX_MODULES`.
- Unit tests import modules directly rather than including umbrella headers.
- Header-based tests remain only where needed to validate compatibility bridges.

## Embedded platform builds

- RP2040 moves to Pico SDK plus CMake toolchain support.
- ESP32 moves to ESP-IDF plus its native CMake integration.
- ESP8266 must be explicitly evaluated; if a native SDK path is weak or C++23 module support is not credible, the platform should be deferred or isolated behind a bridge strategy rather than weakening the whole design.
- Each platform gets its own toolchain preset and build pipeline, but all consume the same public LumaWave modules where the compiler supports it.

## Packaging rule

The library should stop optimizing for one universal include path across all build systems. The real product becomes a set of module targets plus a narrow optional compatibility header layer.

## SDK Integration Strategy

Most embedded SDKs still expose C headers and macro-heavy APIs. That does not invalidate the modules plan, but it does change how platform code should be structured.

### Recommended approach

- Keep SDK includes inside global module fragments or non-exported implementation units.
- Export LumaWave-owned wrapper types and functions from the platform module interface.
- Avoid re-exporting SDK types unless there is a compelling reason.
- Use narrow bridge layers for clocks, waits, GPIO configuration, DMA submission, and peripheral ownership.

### Why this matters

If SDK macros and C headers leak into the exported interface surface, the module boundary loses most of its value. The platform module should present a stable LumaWave contract while hiding SDK noise.

## API Surface Reinterpretation

The existing seam contracts remain valid, but their packaging changes.

## `IPixelBus`

- stays in `lw.core`
- remains the runtime lifecycle seam
- should not depend on platform-specific storage or transport details

## `IShader`

- moves under `lw.shader`
- remains a frame-transform seam, not a color-model concern

## `IProtocol`

- lives in `lw.protocol`
- owns byte-stream and framing policy
- should absorb protocol-side support logic that is currently misplaced in transport-oriented locations

## `ITransport`

- lives in `lw.transport`
- expresses transport submission behavior only
- platform transport modules provide implementations, not altered base contracts

## Factory And Ergonomic Construction

The current `makeBus(...)` direction remains useful, but under modules it should be treated as a facade-level convenience API rather than the primary way to understand architecture.

Recommended policy:

- construction helpers live in `lw` or `lw.bus`
- the helper imports only the public modules it actually needs
- no hidden Arduino-dependent defaults
- platform-specific convenience constructors belong in the relevant `lw.platform.*` module, not in the generic facade

## Header Compatibility Strategy

Even with a modules-first architecture, a temporary header bridge may still be useful.

That bridge should be narrow and intentionally transitional:

- `src/LumaWave.h` may remain as an opt-in compatibility facade for a migration period
- the header facade should import or include only stable public surfaces
- the header facade must not remain the source of truth for architecture
- no new feature should be added header-first and module-second

This is not a compatibility-shim-heavy plan. It is a controlled migration tool.

## Platform Migration Assumptions

The rearchitecture only makes sense if platform ownership becomes explicit.

## RP2040

- use Pico SDK directly
- prefer PIO, DMA, UART, SPI, PWM, and timer APIs from the native SDK
- remove dependence on Arduino helpers such as `micros()`, `yield()`, or `pinMode()`
- isolate Pico SDK C headers behind the RP2040 platform module boundary

## ESP32

- use ESP-IDF directly
- bind transports to RMT, I2S, SPI, LEDC, sigma-delta, and GPIO APIs from ESP-IDF
- keep FreeRTOS or ESP-IDF scheduling details behind implementation units
- avoid Arduino `HardwareSerial`, `yield()`, and generic Arduino pin APIs in exported surfaces

## ESP8266

- require an explicit keep-or-drop decision
- if retained, it should have a native-SDK-backed design with a credible C++23 toolchain story
- if that story is weak, defer the platform rather than distorting the shared architecture

## Other platforms

- apply the same rule: platform code is a native SDK integration layer, not a special mode of the generic core

## Migration Plan

## Phase 1: Establish target module boundaries

- define the public module catalog
- identify current headers that map cleanly to each module
- identify misplaced helpers, especially protocol-support code living under transport

## Phase 2: Make public seams import-clean

- ensure seam headers and shared contracts do not depend on platform headers
- remove leftover Arduino assumptions from generic contracts
- reduce public surface leakage before converting files to interface units

## Phase 3: Introduce module interface units for host-native builds

- add `lw.core`, `lw.color`, `lw.shader`, `lw.protocol`, `lw.transport`, `lw.bus`, and `lw`
- keep implementation mostly header-backed at first if that reduces migration risk
- convert native tests to import the modules directly

## Phase 4: Split platform packs behind native SDK bridges

- create `lw.platform.rp2040` and `lw.platform.esp32`
- move SDK includes behind non-exported units or global module fragments
- replace Arduino build plumbing with native SDK builds

## Phase 5: Shrink the compatibility header path

- reduce `LumaWave.h` to a thin migration facade
- stop documenting header-first usage as the primary path
- keep only the bridges that are justified by real consumers

## Phase 6: Enforce module-first development

- require new public features to land through module surfaces first
- keep headers private unless they are intentionally part of the compatibility layer
- structure tests and examples around module imports where the toolchain supports them

## Testing And Verification Strategy

The module migration must be validated as an architecture change, not just a compile experiment.

### Host-native validation

- compile each public module independently
- compile representative consumer tests that import `lw`, `lw.bus`, and platform modules directly
- keep current native behavioral tests, but migrate them to module imports over time

### Platform validation

- per-platform build smoke using the native SDK toolchain
- targeted transport tests for timing/framing-sensitive protocols
- hardware-in-the-loop checks for DMA, PIO, RMT, I2S, UART, and SPI transports where applicable

### Contract validation

- verify that `lw.core` stays platform-agnostic
- verify that platform SDK headers do not leak through exported module interfaces
- verify that protocol helpers no longer depend on transport-owned implementation details

## Risks

- embedded compiler and build-system support for named modules may lag behind host-native support
- SDK C headers may force more global-module-fragment usage than ideal
- a naive one-header-to-one-module rewrite could create poor module boundaries and preserve current dependency mistakes
- keeping too much compatibility-header surface for too long would blunt the architectural benefit

## Open Questions

- Which platforms actually have credible module-capable C++23 toolchain support in the near term?
- Is ESP8266 worth carrying into the modules-first architecture, or should it be deliberately retired?
- Should one-wire timing support live inside `lw.protocol` proper or in a small internal support partition shared by protocols?
- How much of the current facade alias surface is still worth preserving once explicit imports become normal?

## Recommendation

The right strategy is to treat C++23 modules as the packaging and dependency model for the library's next architecture phase, not as a syntax cleanup pass.

That means:

- preserve the seam-based object model
- formalize those seams as a small public module graph
- move embedded targets to native SDK ownership
- keep SDK details behind platform-edge module boundaries
- retain only a thin, temporary header facade while consumers transition

If the project follows that path, modules become a real architectural simplifier instead of a cosmetic language upgrade.