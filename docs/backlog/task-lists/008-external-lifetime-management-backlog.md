# External Lifetime Management Backlog

Purpose: Remove all internal ownership of dynamically-allocated components and buffers from `Bus` and `ProtocolTransportPipeline`. The caller becomes exclusively responsible for managing the lifetime of all protocol, transport, pipeline-run, scratch-buffer, and protocol-buffer objects. Neither `Bus` nor `ProtocolTransportPipeline` will allocate, own, or destroy these resources — they operate on caller-provided pointers, references, and spans.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Status

Design phase. No code changes.

## Motivation

### Problem

Currently `ProtocolTransportPipeline` internally owns its protocol and transport via `std::unique_ptr`, and manages two internal buffers (`_protocolBuffer` as `std::vector<uint8_t>`, `_scratchPixel` as `std::vector<lw::colors::Color>`) that are allocated on the heap. Similarly, `Bus` owns a `std::vector<PipelineRun>`, which in turn owns `std::unique_ptr<IOutputPipeline>` entries.

This creates several issues:

1. **Hidden allocation sites.** The heap allocations for `_protocolBuffer`, `_scratchPixel`, and the `_runs` vector happen invisibly inside constructor and `write()` paths. Users cannot control where or when these allocations occur.
2. **No static-allocation path.** Because ownership is baked in, there is no way to use these classes without the heap. A compile-time fixed-storage deployment cannot reuse `Bus` or `ProtocolTransportPipeline` without either accepting the heap or working around the ownership.
3. **Lifetime opacity.** The caller cannot reuse a protocol instance across pipelines, share a transport, or provide a pre-allocated DMA-safe buffer — the pipeline demands exclusive ownership.
4. **Construction coupling.** `Bus`'s `initializer_list<PipelineRun>` constructor moves unique_ptrs out of the initializer list entries, making the ownership transfer implicit and hard to reason about at the call site.

### Solution

Externalize all ownership. `ProtocolTransportPipeline` accepts:
- Raw pointers (or references) to protocol and transport — the caller guarantees lifetime exceeds the pipeline's.
- A caller-provided `span<uint8_t>` for the protocol encoding buffer — the caller allocates and manages it.
- A caller-provided `span<lw::colors::Color>` for the scratch pixel buffer — the caller allocates and manages it (or passes an empty span to disable the brightness/scratch path at compile time).

`Bus` accepts:
- A `span<lw::colors::Color>` for pixel storage (already a span, unchanged).
- A `span<PipelineRun>` for run definitions — the caller owns the array and the pipelines within it.
- `PipelineRun` itself changes: instead of `std::unique_ptr<IOutputPipeline>`, it holds an `IOutputPipeline*` (non-owning pointer).

## Target Shape After This Backlog

### ProtocolTransportPipeline

```cpp
class ProtocolTransportPipeline : public IOutputPipeline
{
public:
  // protocol, transport: caller-owned, lifetime must exceed pipeline
  // protocolBuffer: caller-allocated span for encoding output
  // scratchPixels: caller-allocated span for brightness/shader scratch (empty = no scratch)
  ProtocolTransportPipeline(
    protocols::IProtocol& protocol,
    transports::ITransport& transport,
    span<uint8_t> protocolBuffer,
    span<lw::colors::Color> scratchPixels);

  void begin() override;
  bool isReadyToUpdate() const override;
  bool alwaysUpdate() const override;
  void write(span<const lw::colors::Color> colors, BrightnessType brightness) override;

private:
  protocols::IProtocol& _protocol;
  transports::ITransport& _transport;
  span<uint8_t> _protocolBuffer;
  span<lw::colors::Color> _scratchPixels;
};
```

Key changes:
- `_protocol` and `_transport` become references (non-owning).
- `_protocolBuffer` and `_scratchPixels` become `span<>` (caller-allocated, non-owning).
- No `std::vector`, no `std::unique_ptr` members.
- No heap allocation in constructor or `write()`.
- No dynamic resizing of buffers — caller must provide sufficiently sized spans.

### PipelineRun

```cpp
struct PipelineRun
{
  IOutputPipeline* pipeline;  // was unique_ptr
  size_t length;
};
```

- Raw pointer, non-owning. Caller guarantees lifetime exceeds the `Bus`.

### Bus

```cpp
class Bus : public IPixelBus
{
public:
  Bus(span<lw::colors::Color> pixelStorage, span<const PipelineRun> runs);

  // ... rest unchanged ...
private:
  span<lw::colors::Color> _pixels;
  span<const PipelineRun> _runs;  // was vector
  // _brightness, _dirty unchanged
};
```

Key changes:
- Constructor takes `span<const PipelineRun>` instead of `std::initializer_list<PipelineRun>`.
- `_runs` becomes `span<const PipelineRun>` — no vector, no heap allocation.
- The `runs()` accessor returns `span<const PipelineRun>` instead of `const vector<PipelineRun>&`.

### Caller Pattern

Before (current):
```cpp
auto pixels = std::make_unique<Color[]>(60);
auto bus = Bus(span(pixels.get(), 60), {
  {std::make_unique<ProtocolTransportPipeline>(
    std::make_unique<SomeProtocol>(),
    std::make_unique<SomeTransport>()), 60}
});
```

After:
```cpp
Color pixels[60];
SomeProtocol protocol;
SomeTransport transport;
uint8_t protocolBuffer[256];
Color scratchPixels[60];  // optional, empty span to disable

ProtocolTransportPipeline pipeline{protocol, transport, protocolBuffer, scratchPixels};
PipelineRun runs[] = {{&pipeline, 60}};

Bus bus{span{pixels}, span{runs}};
```

## Non-Goals

- Do not change `IPixelBus` or `IOutputPipeline` interfaces (except `PipelineRun` struct if referenced).
- Do not change `IProtocol` or `ITransport` interfaces.
- Do not introduce factory functions, descriptor systems, or builder patterns.
- Do not add template parameters to `Bus` or `ProtocolTransportPipeline`.
- Do not change the `PipelineRun` storage model beyond the pointer change.
- Do not retrofit existing light driver implementations — only `ProtocolTransportPipeline` and `Bus`.
- Do not add runtime safety checks (null-pointer assertions, buffer-size validation) beyond what is reasonable for debug builds.
- Do not add compatibility shims or overloads to preserve the old owning API.

## Task List

### Phase 1 — PipelineRun Pointer Conversion

- [ ] **`P1a`** — Change `PipelineRun::pipeline` from `std::unique_ptr<IOutputPipeline>` to `IOutputPipeline*` in `src/buses/Bus.h`.
- [ ] **`P1b`** — Update `Bus` constructor: remove the `const_cast` + `std::move` workaround for initializer_list; iterate over `span` of runs.
- [ ] **`P1c`** — Update `Bus::_runs` member type from `std::vector<PipelineRun>` to `span<const PipelineRun>`.
- [ ] **`P1d`** — Update `Bus::runs()` accessor return type to `span<const PipelineRun>`.
- [ ] **`P1e`** — Build and fix any compilation errors.

### Phase 2 — ProtocolTransportPipeline Reference Conversion

- [ ] **`P2a`** — Change `_protocol` from `std::unique_ptr<protocols::IProtocol>` to `protocols::IProtocol&` in `ProtocolTransportPipeline.h`.
- [ ] **`P2b`** — Change `_transport` from `std::unique_ptr<transports::ITransport>` to `transports::ITransport&` in `ProtocolTransportPipeline.h`.
- [ ] **`P2c** — Update constructor to accept references: `ProtocolTransportPipeline(IProtocol& protocol, ITransport& transport, ...)`.
- [ ] **`P2d`** — Update `begin()` to call `_transport.begin()` and `_protocol.begin()` (`.` not `->`).
- [ ] **`P2e`** — Update `isReadyToUpdate()` to call `_transport.isReadyToUpdate()` (`.` not `->`).
- [ ] **`P2f`** — Update `alwaysUpdate()` to call `_protocol.alwaysUpdate()` (`.` not `->`).
- [ ] **`P2g`** — Build and fix any compilation errors.

### Phase 3 — ProtocolTransportPipeline Buffer Externalization

- [ ] **`P3a`** — Replace `std::vector<uint8_t> _protocolBuffer` with `span<uint8_t> _protocolBuffer` member.
- [ ] **`P3b`** — Replace `std::vector<lw::colors::Color> _scratchPixel` with `span<lw::colors::Color> _scratchPixels` member.
- [ ] **`P3c`** — Add buffer parameters to constructor: `ProtocolTransportPipeline(IProtocol& protocol, ITransport& transport, span<uint8_t> protocolBuffer, span<lw::colors::Color> scratchPixels)`.
- [ ] **`P3d`** — Rewrite `write()`:
  - Remove all `_protocolBuffer.size() != requiredSize` resizing logic.
  - Remove all `_scratchPixel.resize()` / `assign()` logic.
  - Validate caller-provided buffer sizes (debug assertion at minimum).
  - Use `_protocolBuffer` and `_scratchPixels` spans directly.
- [ ] **`P3e`** — Remove `#include <vector>` and `#include <memory>` from `ProtocolTransportPipeline.h` if no longer needed.
- [ ] **`P3f`** — Build and fix any compilation errors.

### Phase 4 — Bus Constructor Span Conversion

- [ ] **`P4a`** — Change `Bus` constructor to accept `span<const PipelineRun>` instead of `std::initializer_list<PipelineRun>`.
- [ ] **`P4b`** — Remove the `const_cast<PipelineRun&>` + `std::move` loop in constructor (no longer needed with non-owning pointers).
- [ ] **`P4c`** — Simplify constructor body: just store the span and validate.
- [ ] **`P4d`** — Remove `#include <vector>` and `#include <memory>` from `Bus.h` if no longer needed.
- [ ] **`P4e`** — Build and fix any compilation errors.

### Phase 5 — Test and Example Updates

- [ ] **`P5a`** — Audit all test files that construct `ProtocolTransportPipeline` and update to new signature (references + caller-allocated buffers).
- [ ] **`P5b`** — Audit all test files that construct `Bus` with `initializer_list<PipelineRun>` and update to `span<const PipelineRun>`.
- [ ] **`P5c`** — Audit all example files for the same construction pattern changes.
- [ ] **`P5d`** — Build native test suite.
- [ ] **`P5e`** — Run full test suite: `ctest --test-dir build --output-on-failure`.

### Phase 6 — Documentation

- [ ] **`P6a`** — Update `.github/copilot-instructions.md` if construction patterns or ownership rules change.
- [ ] **`P6b`** — Update `docs/internal/information/object-model-contracts.md` to reflect non-owning external-lifetime model.
- [ ] **`P6c`** — Update `docs/README.md` backlog references.
- [ ] **`P6d`** — Mark this backlog as completed.

## Dependencies Between Phases

```text
Phase 1 (PipelineRun*) ──► Phase 4 (Bus span)
                                    │
Phase 2 (PTP refs) ──► Phase 3 (PTP buffers) ──► Phase 5 (Tests/Examples)
                                                          │
                                                    Phase 6 (Docs)
```

## Open Decisions

1. **Buffer size validation at construction vs write-time.** Proposal: accept buffers at construction, validate `requiredBufferSizeBytes()` vs `_protocolBuffer.size()` in `write()` with a debug assertion. Caller must ensure the buffer is large enough before calling `write()`.
2. **Empty scratch span policy.** An empty `scratchPixels` span means "no scratch buffer available" — the pipeline will skip any operation requiring a scratch (brightness/shader path) and encode directly. This is a valid configuration for pipelines that never use brightness scaling or shaders.
3. **`BrightnessType` removal from `write()`.** This backlog defers to backlog 009 (shader reintroduction), which moves brightness into a `BrightnessShader`. However, if backlog 008 lands first, the `write(span<const Color>, BrightnessType)` signature remains unchanged; the brightness path still uses the scratch span internally.
