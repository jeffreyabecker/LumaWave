# Bus Builder Backlog

> **Purpose:** Break down the implementation of `lw::buses::BusBuilder` and associated types (type-erased holders, `BusStorage`, `StackBusStorage`) as specified in the [Bus Builder & Lifetime Simplification design plan](../plan/bus-builder-lifetime-simplification.md).

## Status Legend

| Status | Meaning |
|--------|---------|
| `todo` | Not yet started |
| `doing` | In progress |
| `done` | Completed |
| `blocked` | Waiting on dependency |
| `dropped` | Intentionally removed from scope |

## Source Documents

| Category | File | Key Symbols |
|----------|------|-------------|
| Bus — interface | `src/core/IPixelBus.h` | `lw::IPixelBus` |
| Bus — composite | `src/buses/Bus.h` | `lw::buses::Bus`, `lw::buses::PipelineRun` |
| Bus — dynamic template | `src/buses/PixelBus.h` | `lw::buses::PixelBus<TProtocol, TTransport, ...TShaders>` |
| Bus — static template | `src/buses/StackPixelBus.h` | `lw::buses::StackPixelBus<NPixelCount, TProtocol, TTransport, ...TShaders>` |
| Bus — convenience header | `src/buses/Busses.h` | — |
| Pipeline | `src/buses/ProtocolTransportPipeline.h` | `lw::buses::ProtocolTransportPipeline` |
| Output pipeline | `src/core/OutputPipeline.h` | `lw::OutputPipeline` |
| Protocol | `src/protocols/Protocol.h` | `lw::protocols::Protocol`, `lw::protocols::ProtocolSettings` |
| Shader protocol | `src/protocols/ShaderProtocol.h` | `lw::protocols::ShaderProtocol` |
| Shader interface | `src/protocols/IShader.h` | `lw::protocols::IShader` |
| Transport | `src/transports/Transport.h` | `lw::transports::Transport`, `lw::transports::TransportSettingsBase` |
| Color / span | `src/core/Compat.h` | `lw::Pixel`, `lw::span`, `lw::PixelCount` |
| Convenience header | `src/LumaWave.h` | Public include surface |
| Tests — Bus | `test/busses/test_bus/test_main.cpp` | Manual `Bus` + `PipelineRun` construction |
| Tests — StackPixelBus | `test/busses/test_pixel_bus/test_main.cpp` | Static template construction |
| Tests — PixelBus | `test/busses/test_pixel_bus_dynamic/test_main.cpp` | Dynamic template construction |
| Design plan | `docs/backlog/plan/bus-builder-lifetime-simplification.md` | Full design, usage examples, open decisions |
| Test CMake | `test/CMakeLists.txt` | Unity + ArduinoFake test infrastructure |
| Presets (planned) | `src/protocols/*Preset.h`, `src/transports/*Preset.h` | `lw::buses::presets` structs with `configure(BusBuilder&)` |
| Reference: NeoPixelBus | `NeoPixelBus.h` (Makuna) | `T_COLOR_FEATURE` + `T_METHOD` composition pattern |

## Problem Statement

Constructing a complete `IPixelBus` requires allocating and wiring 7–11 interdependent objects with strict lifetime ordering. The `PixelBus` and `StackPixelBus` templates encapsulate this but suffer from fragile member ordering (the initializer list is a single point of failure for 10+ dependencies), template explosion (shader parameter packs propagate through every layer), no multi-strip support, no transport/protocol sharing, no gradual construction, and duplicated logic between the dynamic and static variants. A builder pattern with internal type erasure eliminates these problems while leaving existing types unchanged.

### Multi-Run Decision (2026-07-22)

Multi-run (composite) bus support — where a single `Bus` holds multiple `PipelineRun`s driven by one `show()` call — was dropped from scope. The same outcome is achieved with zero additional complexity by constructing separate `BusBuilder` instances with `setPixelStorage()` over sub-spans of a shared pixel buffer:

```cpp
lw::Pixel allPixels[90];
auto strip1 = BusBuilder().setPixelStorage(span{allPixels, 30})...build();
auto strip2 = BusBuilder().setPixelStorage(span{allPixels + 30, 60})...build();
strip1->show();  // drives pixels [0..29]
strip2->show();  // drives pixels [30..89]
```

Two `show()` calls vs. one is functionally equivalent when strips use different protocols/transports on independent hardware paths. Dropped tasks: BBL-22, BBL-26, BBL-34, BBL-43, BBL-61.

## Phase 1 — Type-Erasure Holder Infrastructure

> **Goal:** Implement the internal type-erased holder classes that `BusBuilder` uses to own transports, protocols, and shaders without propagating template parameters.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-01 | `done` | Draft `TransportHolder`: move-only, type-erased owner of a `Transport` via custom vtable (no RTTI, no exceptions) | — | Header `src/buses/detail/TransportHolder.h` exists; holds `unique_ptr<TransportBase>` with hand-rolled vtable dispatch for `begin()`, `beginTransaction()`, `transmitBytes()`, `endTransaction()`, `isReadyToUpdate()`, `setRuntimeConfig()`; unit tested in `test/buses/test_bus_builder/` |
| BBL-02 | `done` | Draft `ProtocolHolder`: move-only, type-erased owner of a `Protocol` via custom vtable | — | Header `src/buses/detail/ProtocolHolder.h` exists; holds `unique_ptr<ProtocolBase>` with hand-rolled vtable dispatch for `begin()`, `update()`, `settings()`, `pixelCount()`, `alwaysUpdate()`, `setRuntimeConfig()`; unit tested |
| BBL-03 | `done` | Draft `ShaderList`: owning, type-erased list of `IShader*` plus backing storage for shader objects | — | Header `src/buses/detail/ShaderList.h` exists; stores `vector<unique_ptr<IShader>>` and produces `span<IShader*>`; `addShader<T>()` template method constructs in place; unit tested |
| BBL-04 | `done` | Wire holders into `test/CMakeLists.txt` and add a dedicated test binary `test_bus_builder` | BBL-01, BBL-02, BBL-03 | `test/buses/test_bus_builder/` directory exists with Unity test file; CMake target registered; all holder tests pass in native build |

## Phase 2 — Buffer Management

> **Goal:** Implement buffer allocation and management so the builder has a single point of control for protocol buffers and scratch pixel buffers.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-10 | `done` | Implement `BufferManager`: owns protocol buffer(s) and scratch pixel buffer for a single run | BBL-01, BBL-02, BBL-03 | Header `src/buses/detail/BufferManager.h` exists; computes protocol buffer size from protocol type and pixel count; allocates scratch only when `ShaderList::needsScratchBuffer()` is true; stores `vector<uint8_t>` (protocol) and `vector<Pixel>` (scratch, may be empty); provides `span<uint8_t>` and `span<Pixel>` accessors; unit tested |
| BBL-11 | `done` | Extend `BufferManager` to support multiple runs (one protocol buffer per run, shared scratch) | BBL-10 | `BufferManager` can allocate N protocol buffers for N runs; accessor takes run index; unit tested with multi-run scenario |

## Phase 3 — BusBuilder Core (Heap / Dynamic Path)

> **Goal:** Implement the `BusBuilder` class with incremental construction, validation, and `build()` → `unique_ptr<IPixelBus>` finalization for heap-allocated buses.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-20 | `done` | Resolve open decision BBL-DEC-5 (type erasure: custom vtable vs. std::any) | — | **Custom vtable.** `TransportHolder` / `ProtocolHolder` use hand-rolled vtables + `unique_ptr<TransportBase>`; no RTTI, no exceptions. |
| BBL-21 | `done` | Implement `BusStorage` (heap variant): single-owner RAII struct that owns all members in correct dependency order for a single run | BBL-10 | Header `src/buses/BusStorage.h` exists; holds `vector<Pixel>` pixels, `ProtocolHolder`, `TransportHolder`, `ShaderList`, `BufferManager`, `ShaderProtocol`, `ProtocolTransportPipeline`, `PipelineRun`, `Bus`; constructor wires everything in order; non-copyable, non-movable (internal references); unit tested |
| BBL-22 | `dropped` | Extend `BusStorage` to support multi-run (composite) buses | BBL-11, BBL-21 | Dropped: multi-strip achieved via separate BusBuilder instances with setPixelStorage() over sub-spans of a shared pixel buffer |
| BBL-23 | `done` | Implement `BusBuilder` class with `setPixelCount()` and `setPixelStorage()` | BBL-01, BBL-02, BBL-03, BBL-20 | Header `src/buses/BusBuilder.h` exists; `setPixelCount(n)` allocates internal pixel storage; `setPixelStorage(span)` uses external pixels; calling both is rejected; unit tested |
| BBL-24 | `done` | Implement `BusBuilder::setTransport<T>()` and `BusBuilder::setProtocol<T>()` | BBL-01, BBL-02, BBL-23 | Templated setters move-construct into `TransportHolder` / `ProtocolHolder`; both accept `SettingsType` parameter; settings forwarded; unit tested |
| BBL-25 | `done` | Implement `BusBuilder::addShader<T>()` | BBL-03, BBL-23 | Templated method appends to `ShaderList`; shaders apply in insertion order; unit tested with 0, 1, and 2+ shaders |
| BBL-26 | `dropped` | Implement `BusBuilder::addRun()` (low-level) | BBL-11, BBL-23 | Dropped with multi-run; external pixel storage covers the use case |
| BBL-27 | `done` | Implement `BusBuilder::build()` | BBL-21, BBL-24, BBL-25 | Validates configuration (transport set, protocol set); constructs `BusStorage` on heap; returns `unique_ptr<IPixelBus>` (or `nullptr` on failure); builder consumed (moved-from); unit tested for single strip, with shaders, with external pixels |
| BBL-28 | `done` | Implement `BusBuilder::validate()` for early checking | BBL-27 | Returns descriptive error string without allocating; unit tested for missing transport, missing protocol, empty configuration |
| BBL-29 | `done` | Implement `is_preset<T>` SFINAE gate and `BusBuilder::addStrip()` overloads | BBL-24, BBL-25, BBL-20 | `is_preset<T>` SFINAE trait in `src/buses/detail/PresetTraits.h`; two `addStrip` overloads: (proto,trans) and (proto,trans,shader); each calls `configure()` on each preset in order; unit tested with mock presets |
| BBL-30 | `done` | Implement preset structs alongside protocol/transport headers | BBL-29 | Protocol presets in `src/protocols/` (e.g., `Ws2812xPreset.h`); transport presets in `src/transports/` (e.g., `NilTransportPreset.h`); aggregate headers `ProtocolPresets.h` / `TransportPresets.h`; namespace `lw::buses::presets`; each struct has `configure(BusBuilder&)` and public fields |
| BBL-31 | `done` | Test preset composition: single-strip, inline field override, preset+shader | BBL-29, BBL-30 | Tests: `addStrip(ws2812x{}, nil_transport{})`; `addStrip(ws2812x{.channelOrder="RGB"}, nil_transport{})`; `addStrip(ws2812x{}, nil_transport{}, brightness{})`; SFINAE gate rejects non-preset types; build-and-use through presets |

## Phase 4 — Stack / Static Allocation Path

> **Goal:** Implement `StackBusStorage` and `buildInto()` so embedded targets can construct buses with zero heap allocation.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-32 | `done` | Implement `StackBusStorage<N, TProtocol, TTransport, TShaders...>` template | BBL-21 | Header `src/buses/StackBusStorage.h` exists; compile-time-sized arrays for pixels, protocolBuffer, scratchPixels; owns protocol, transport, shaders, ShaderProtocol, ProtocolTransportPipeline, PipelineRun, Bus by value; constructor wires dependencies in order; non-copyable, non-movable; unit tested |
| BBL-33 | `done` | Implement `BusBuilder::buildInto(TStorage&)` | BBL-27, BBL-32 | Validates builder configuration is complete and pixel count matches storage; returns `IPixelBus&` referencing the storage's `Bus`; unit tested with `StackBusStorage` |
| BBL-34 | `dropped` | Add multi-run `StackBusStorage` variant or template specialization | BBL-22, BBL-32 | Dropped with multi-run |

## Phase 5 — Test Suite Completion

> **Goal:** Comprehensive test coverage for all builder paths, edge cases, and error conditions.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-40 | `done` | Test: single strip, no shaders, heap path | BBL-27 | Test constructs bus via builder, calls `begin()` / `pixels()` / `show()` — covered by test_builder_simple_build |
| BBL-41 | `done` | Test: single strip with 1 shader | BBL-27 | Shader applies correctly before protocol encoding — covered by test_builder_with_shader |
| BBL-42 | `done` | Test: single strip with 2+ shaders chained in order | BBL-27 | Shaders apply in insertion order — test_shaders_chained_in_insertion_order verifies via order-tracking shaders |
| BBL-43 | `dropped` | Test: multi-strip with different protocols per strip | BBL-27 | Dropped with multi-run; use separate BusBuilder instances with setPixelStorage() |
| BBL-45 | `done` | Test: external pixel storage (`setPixelStorage`) | BBL-27 | External buffer is used directly; writes to bus->pixels() modify external buffer — BusStorage and BusBuilder updated to support external pixel constructor |
| BBL-46 | `done` | Test: static path (`StackBusStorage` + `buildInto`) | BBL-33 | Compile-time allocation; bus operates correctly — covered by test_build_into_matching_pixel_count |
| BBL-47 | `done` | Test: validation failures (missing transport, missing protocol, missing pixel count) | BBL-28 | Each error condition produces expected error; build() returns nullptr — covered by existing validation tests |
| BBL-48 | `done` | Test: builder consumed after `build()` (moved-from state) | BBL-27 | Using builder after `build()` returns nullptr — test_builder_double_build_returns_null |
| BBL-49 | `done` | Test: runtime config passthrough (`setRuntimeConfig`) | BBL-27 | `setRuntimeConfig` on resulting bus is a no-op at Bus level (passthrough to shaders/transport not yet wired through OutputPipeline); test verifies no-crash |
| BBL-50 | `done` | Verify no heap allocation in static path (compile-time check) | BBL-33, BBL-34 | `StackBusStorage` uses compile-time C arrays; test_stack_bus_storage_no_heap_types confirms template compiles without heap types |

## Phase 6 — Examples Migration

> **Goal:** Port existing examples to use `BusBuilder` and add new multi-strip examples.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-60 | `done` | Port `examples/hello/` to use `BusBuilder` | BBL-27 | Example `examples/hello/builder/builder.ino` compiles and runs with builder API; simpler than current manual construction |
| BBL-61 | `dropped` | Port `examples/multi-strip/` to use `BusBuilder::addStrip()` | BBL-27, BBL-30 | Dropped with multi-run; multi-strip uses separate BusBuilder instances with setPixelStorage() over sub-spans |
| BBL-62 | `done` | Add example demonstrating external pixel storage (zero-copy / WLED pattern) | BBL-27 | `examples/external-pixels/external-pixels.ino` shows `setPixelStorage()` with caller-owned array |
| BBL-63 | `done` | Add example demonstrating static/stack allocation with `buildInto()` | BBL-33 | `examples/stack-allocation/stack-allocation.ino` shows `StackBusStorage` + `buildInto()` for no-heap embedded use |
| BBL-64 | `done` | Add example demonstrating shader chaining via builder | BBL-27 | `examples/shader-chaining/shader-chaining.ino` chains BrightnessShader + GammaShader via `addShader()` |
| BBL-65 | `done` | Add example demonstrating presets (`ws2812x`, `nil_transport`, inline override) | BBL-30 | `examples/presets/presets.ino` shows `addStrip` with protocol+transport presets, inline field override, and preset + explicit shader |

## Phase 7 — Documentation & Cleanup

> **Goal:** Document the new API, add deprecation notices, and ensure cross-references are consistent.

| ID | Status | Task | Depends On | Definition of Done |
|----|--------|------|------------|-------------------|
| BBL-70 | `todo` | Write usage doc `docs/usage/bus-builder.md` per usage-doc-authoring conventions | BBL-27, BBL-33 | Document covers all builder methods, ownership model, external pixels, static path, error handling; linked from design plan |
| BBL-71 | `todo` | Update `src/LumaWave.h` to expose `BusBuilder`, `BusStorage`, `StackBusStorage`, `ProtocolPresets`, `TransportPresets` | BBL-27, BBL-30, BBL-33 | Public headers included in convenience header; examples can `#include <LumaWave.h>` only |
| BBL-72 | `todo` | Add deprecation notice to `PixelBus.h` and `StackPixelBus.h` recommending `BusBuilder` | BBL-27, BBL-33 | `@deprecated` doxygen comments added; migration guide references builder |
| BBL-73 | `todo` | Update cross-references in design plan and related docs | BBL-70 | Design plan links to usage doc; source document table updated; complement references consistent |
| BBL-74 | `todo` | Run full test suite and clang-format | BBL-49 | `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure` passes; `clang-format` clean |

