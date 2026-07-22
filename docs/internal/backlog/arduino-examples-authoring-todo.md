# Arduino Examples TODO

Backlog for maintaining and expanding `examples/` against the current public API.

All examples in this backlog are Arduino examples unless explicitly stated otherwise.
Example sketches should be authored as `.ino` files (not `.cpp`) unless a specific non-Arduino scenario is called out.

## 0) Current Baseline (March 2026)

- [x] `LumaWave.h` is the single public include for examples.
- [x] `makePixelBus`/`MakePixelBus.h` have been removed.
- [x] Examples should use direct bus constructors or BusBuilder.
- [x] Global aliases are optional and can be disabled via `LW_USE_EXPLICIT_NAMESPACES`.
- [ ] Maintain dual-compatibility where practical:
  - [ ] default alias mode (macro not set),
  - [ ] explicit namespace mode (`LW_USE_EXPLICIT_NAMESPACES`).

## 1) Example Inventory (Current)

Existing example set:

- [x] `examples/platform/platformio-smoke/src/main_virtual_smoke.cpp` (legacy format; migrate to `.ino`)
- [x] `examples/rp2040-cct-white-balance/src/main.cpp` (legacy format; migrate to `.ino`)
- [x] `examples/rp2040-pwm-light/src/main.cpp` (legacy format; migrate to `.ino`)

Status notes:

- [ ] Add a short header comment to each example with:
  - [ ] target platforms,
  - [ ] required transport/driver macros,
  - [ ] whether it is alias-mode-only or explicit-namespace-safe.

## 2) Authoring Rules (Updated)

- [ ] Use only `#include <LumaWave.h>` (and `#include <Arduino.h>` when needed).
- [ ] Use `.ino` sketch files for examples by default; do not add new `.cpp` sketches unless explicitly justified.
- [ ] Do not include internal headers in examples.
- [ ] Prefer direct constructor composition; do not reintroduce builder/factory helper APIs.
- [ ] Prefer existing unqualified aliases from `LumaWave.h`; when no adequate alias exists, use fully qualified `lw::...` symbols in examples rather than expanding aliases solely for examples.
- [ ] Keep examples protocol-focused unless the example is explicitly about transport or shaders.
- [ ] Keep default color type unless the example is explicitly about color depth/channels.
- [ ] Avoid compatibility shims in examples; update examples directly when APIs change.

## 3) Near-Term Additions

### Root Hello Examples

- [ ] Add `examples/hello-ws2812/hello-ws2812.ino`.
- [ ] Add `examples/hello-apa102/hello-apa102.ino`.
- [ ] For both:
  - [ ] constructor-based Strip setup,
  - [ ] no shader,
  - [ ] simple animated output,
  - [ ] clear pin/count constants at top.
- [ ] Add `examples/hello-light/hello-light.ino`
  - [ ] Use RGBCW16 colors and Platform Default driver
  - [ ] constructor-based Light setup,
  - [ ] no shader,
  - [ ] simple animated output,
  - [ ] clear pin/count constants at top.

### Bus Composition Coverage

- [x] Add multi-strip example using BusBuilder with shared pixel buffer (`setPixelStorage` sub-spans).
- [x] Document multi-strip approach: separate BusBuilder instances over sub-spans.
- [ ] Add one example demonstrating using `Topology` with a 4x4 grid of 16x16 tiles

### Platform Transport Examples
- [ ] Add one example for each platform specific Transport

### Shader Coverage

- [ ] Add one deterministic shader example using `GammaShader`.
- [x] Add shader-chaining example (`addShader` with multiple shaders).

## 4) Platform and Behavior Validation

- [ ] Verify default RP2040 compile for each example unless its platform specific.
- [ ] Add a quick explicit-namespace compile pass (`-DLW_USE_EXPLICIT_NAMESPACES`) for examples that claim compatibility.
- [ ] Keep protocol/transport behavior validation in native tests; examples should stay illustrative and minimal.

## 5) Documentation and Maintenance

- [ ] Update README examples index to reflect current folders and new hello examples.
- [ ] Remove stale references to removed builder/factory flows in docs.
- [ ] For each new example, include a one-line “API assumptions” note so future refactors can update quickly.

## 6) Completion Gates

- [ ] No example includes removed headers/APIs (`MakePixelBus.h`, `makePixelBus`).
- [ ] New/updated Arduino examples use `.ino` sketches unless explicitly documented otherwise.
- [ ] All examples compile in default RP2040 workflow.
- [ ] At least one examples sweep is run after major bus/protocol API changes.
