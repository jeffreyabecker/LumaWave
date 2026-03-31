# Global Brightness Through Bus, Transport, and Drivers

## Goal

Introduce an explicit global brightness setting that is carried from the bus layer through transport and light-driver seams as a `uint16_t` value.

## Problem Summary

The library currently has several brightness-adjacent behaviors, but no single explicit global brightness control that belongs to the bus/runtime pipeline.

Observed gaps:

- `PixelBus` and `LightBus` do not expose a dedicated global brightness property.
- `ITransport` and `ILightDriver` do not currently define a common brightness contract.
- Some protocols and drivers can support hardware-side gain or output scaling, but there is no shared path for supplying that value.
- Existing shader-based dimming such as `CurrentLimiterShader` is not the same thing as an explicit user-controlled global brightness setting.
- Without a consistent contract, different buses and outputs would need ad hoc brightness behavior.

## Target Design

Define global brightness as a first-class runtime property with a full-range `uint16_t` contract.

### Brightness Contract

Represent global brightness as:

- `uint16_t brightness`
- normalized range `0..65535`
- `0` means fully off
- `65535` means full output with no additional attenuation

Expected behavior:

- buses own the user-facing brightness state
- buses forward the normalized value through transport and driver seams during output
- transports and drivers may either apply the scaling directly or pass it to a lower layer that can do so more efficiently
- the contract remains explicit and separate from shader logic

First-pass ownership rule:

- brightness is applied at exactly one authoritative layer for each output path
- `LightBus` paths treat the light driver write path as authoritative for final output scaling
- `PixelBus` paths treat bus-side scaling before protocol encoding as authoritative in the first pass
- `ITransport` receives brightness per write but remains pass-through unless a concrete transport later takes explicit ownership for a path
- protocol-side hardware gain remains optional and must replace, not stack on top of, generic scaling when enabled

### Bus Layer

Add explicit global brightness state and API to the primary bus surfaces.

Primary candidates:

- `src/core/IPixelBus.h`
- `src/buses/PixelBus.h`
- `src/buses/LightBus.h`
- related reference and aggregate/composite bus types as needed to preserve a coherent bus contract

Expected behavior:

- user code can set and query global brightness on the bus
- `show()` uses the bus brightness every frame
- default brightness is full-scale `65535`
- brightness changes do not require changing per-pixel stored color values

### Transport Layer

Thread global brightness through the transport seam even when a concrete transport does not use it directly.

Primary candidates:

- `src/transports/ITransport.h`
- concrete transports under `src/transports/`, `src/transports/esp32/`, `src/transports/esp8266/`, and `src/transports/rp2040/`

Expected behavior:

- the transport contract can receive the current `uint16_t` brightness value from the bus layer
- transports that are byte-stream only can remain pass-through implementations
- transports with hardware support for brightness-related output control can opt in without changing the bus API again later

### Driver Layer

Apply the same `uint16_t` brightness contract to analog/light-driver style outputs.

Primary candidates:

- `src/transports/ILightDriver.h`
- `src/transports/AnalogPwmLightDriver.h`
- `src/transports/NilLightDriver.h`
- `src/transports/PrintLightDriver.h`
- platform drivers such as RP2040 and ESP32 PWM/light-driver implementations

Expected behavior:

- single-pixel light buses can forward the bus brightness to the driver
- PWM-style drivers scale the final channel output using integer math
- diagnostic or nil drivers preserve the contract even if they do not materially change output

### Protocol Interaction

Keep protocol integration explicit and optional in the first pass.

Notes:

- protocol-side gain features such as TLC59711 global brightness are related but not identical to generic transport/driver scaling
- first-pass design should avoid forcing every protocol to understand brightness if bus or driver-level scaling already covers the required behavior
- where a protocol has a real hardware gain field, the backlog should leave room to map the `uint16_t` bus brightness into the protocol's native range later

## Scope

In scope:

- define a single `uint16_t` global brightness contract
- add bus-level brightness API and state
- add transport and light-driver seam support for forwarding brightness
- implement integer scaling for driver-style outputs where appropriate
- update representative concrete transports/drivers so the contract is exercised end-to-end
- add targeted tests for bus and driver behavior

Out of scope for first pass:

- gamma correction changes
- replacing or redefining `CurrentLimiterShader`
- adding floating-point brightness math
- adding backward-compatibility shims for old APIs
- protocol-by-protocol hardware gain optimization across the entire library in the same change

## Work Items

1. Define the canonical `uint16_t` global brightness semantics and default value.
2. Add brightness setter/getter support to the core bus contract.
3. Implement bus-level storage and propagation in `PixelBus`.
4. Implement bus-level storage and propagation in `LightBus`.
5. Decide how aggregate, composite, and reference-style buses expose or forward brightness so the bus family stays coherent.
6. Extend `ITransport` with an explicit brightness path that concrete transports can accept without forcing transport-specific policy into the bus.
7. Extend `ILightDriver` with an explicit brightness path.
8. Update concrete light drivers to consume the `uint16_t` brightness value using integer math.
9. Update representative transports and nil/debug implementations so the new seam compiles cleanly across environments.
10. Decide the first-pass policy for where scaling happens for pixel buses:
    - first pass uses bus-side pixel scaling before protocol encoding as the authoritative path
    - transports receive brightness per write but remain pass-through unless a later implementation explicitly takes ownership
    - protocol-side hardware gain remains optional for selected protocols and must not stack with generic scaling
11. Ensure protocol-specific hardware gain support remains compatible with the generic `uint16_t` bus contract.
12. Add targeted tests covering:
    - default full brightness behavior
    - zero brightness output
    - intermediate `uint16_t` scaling behavior
    - `PixelBus` propagation
    - `LightBus` and driver propagation
    - representative concrete driver scaling with integer math
13. Run focused native validation first, then broader environment checks as needed.

## Implementation Plan

## Phase 1: Lock the seam contract

### Objective

Define one canonical brightness contract before touching concrete buses or drivers.

### Tasks

- [x] Confirm `uint16_t` brightness semantics remain `0..65535` with `65535` as full-scale output. -- yes
- [x] Decide whether the public brightness API belongs on `IPixelBus` in the first pass. -- yes
- [x] Decide whether `ITransport` receives brightness as a per-write argument, a cached property, or both. -- per-write-argument
- [x] Decide whether `ILightDriver` accepts brightness in `write(...)` or through a separate setter. -- at write
- [x] Document the first-pass rule that brightness must be applied at exactly one authoritative layer for each output path. -- `LightBus` scales at the driver path; `PixelBus` scales before protocol encoding; transport and protocol optimizations must replace, not stack with, the generic path

### Target files

- [ ] `src/core/IPixelBus.h`
- [ ] `src/transports/ITransport.h`
- [ ] `src/transports/ILightDriver.h`

### Exit criteria

- [x] The seam contract is explicit enough that concrete implementations can be updated without ambiguity.
- [x] The design identifies where pixel-bus scaling happens in the first pass and how double-scaling is prevented.

## Phase 2: Add bus-owned brightness state

### Objective

Make brightness a first-class runtime property owned by bus types rather than by ad hoc driver configuration.

### Tasks

- [ ] Add bus-level setter/getter support with default brightness initialized to `65535`.
- [ ] Ensure `show()` paths use the current bus brightness every frame.
- [ ] Keep brightness separate from stored pixel color values so updates do not mutate frame state.
- [ ] Decide and implement how aggregate, composite, and reference-style buses expose or forward brightness.
- [ ] Keep bus-family semantics coherent so user code does not need different brightness mental models per bus type.

### Target files

- [ ] `src/core/IPixelBus.h`
- [ ] `src/buses/PixelBus.h`
- [ ] `src/buses/LightBus.h`
- [ ] `src/buses/AggregateBus.h`
- [ ] `src/buses/CompositeBus.h`
- [ ] `src/buses/ReferenceBus.h`
- [ ] `src/buses/ReferenceLightBus.h`

### Exit criteria

- [ ] User code can set and query brightness from the relevant bus surface.
- [ ] Full-scale default behavior matches current visible output.

## Phase 3: Thread brightness through transport and driver seams

### Objective

Carry the bus-owned brightness value through the output pipeline without forcing every implementation to consume it immediately.

### Tasks

- [ ] Extend `ITransport` with the chosen brightness path.
- [ ] Extend `ILightDriver` with the chosen brightness path.
- [ ] Update nil, debug, and print-oriented implementations so the new seam compiles cleanly and preserves behavior.
- [ ] Keep pass-through transports cheap when they do not apply scaling directly.
- [ ] Preserve room for future hardware-assisted gain implementations without revisiting the public bus API.

### Target files

- [ ] `src/transports/ITransport.h`
- [ ] `src/transports/ILightDriver.h`
- [ ] `src/transports/NilLightDriver.h`
- [ ] `src/transports/PrintLightDriver.h`
- [ ] representative concrete transports under `src/transports/`

### Exit criteria

- [ ] Bus, transport, and driver seams all compile with the same `uint16_t` brightness contract.
- [ ] Pass-through implementations preserve behavior at full brightness.

## Phase 4: Implement first-pass scaling policy

### Objective

Choose one authoritative scaling point per output path and make it deterministic with integer math.

### Tasks

- [ ] For `LightBus`, implement final channel scaling in driver-style outputs using integer math.
- [ ] For `PixelBus`, implement bus-side brightness scaling before protocol encoding.
- [ ] Verify zero brightness suppresses visible output without mutating stored pixel data.
- [ ] Verify intermediate brightness values produce deterministic scaled output.
- [ ] Document any protocol-specific exceptions where hardware gain replaces generic scaling for the affected path.

### Target files

- [ ] `src/buses/PixelBus.h`
- [ ] `src/buses/LightBus.h`
- [ ] `src/transports/AnalogPwmLightDriver.h`
- [ ] representative platform light drivers under `src/transports/esp32/` and `src/transports/rp2040/`

### Exit criteria

- [ ] At least one pixel-bus path and one light-driver path apply brightness end to end.
- [ ] Integer scaling behavior is deterministic and reviewable.

## Phase 5: Leave room for protocol-side hardware gain

### Objective

Keep the generic contract compatible with protocols that expose real hardware brightness fields.

### Tasks

- [ ] Identify whether any protocol should receive first-pass integration after the generic seam lands.
- [ ] Define how `uint16_t` bus brightness maps into narrower protocol-native gain ranges.
- [ ] Ensure protocol-side gain integration does not double-apply brightness when generic scaling is also available.
- [ ] Keep protocol support optional so unsupported protocols remain valid under the generic path.

### Target files

- [ ] `src/protocols/Tlc59711Protocol.h`
- [ ] other protocol headers only if a concrete first-pass integration is selected

### Exit criteria

- [ ] The generic bus brightness path remains valid even if no protocol-specific gain is implemented in the first pass.
- [ ] Any selected hardware-gain integration has a documented rounding and ownership policy.

## Phase 6: Validate contract behavior

### Objective

Prove the new brightness contract behaves correctly in focused native tests before broader platform checks.

### Tasks

- [ ] Add contract coverage for default full brightness behavior.
- [ ] Add coverage for zero brightness output.
- [ ] Add coverage for intermediate `uint16_t` scaling values.
- [ ] Add `PixelBus` propagation coverage.
- [ ] Add `LightBus` to driver propagation coverage.
- [ ] Add representative concrete driver scaling tests using integer math.
- [ ] Run `pio test -e native-test`.
- [ ] Run targeted contract suites for any seam-sensitive additions.
- [ ] Run broader environment validation only after native behavior is stable.

### Candidate test areas

- [ ] `test/busses/`
- [ ] `test/lights/`
- [ ] `test/transports/`
- [ ] `test/contracts/`

### Exit criteria

- [ ] Focused native tests verify propagation and scaling semantics.
- [ ] Any platform-specific follow-up work is clearly isolated from the generic contract landing.

## Acceptance Criteria

- A bus exposes an explicit global brightness setting as `uint16_t`.
- Default brightness is full scale and preserves current visible output.
- Setting brightness to `0` suppresses output intensity without mutating stored pixel data.
- Intermediate brightness values produce deterministic integer-scaled output.
- Bus, transport, and driver seams all support the same brightness contract.
- Each output path has exactly one authoritative brightness application point.
- At least one pixel-bus path and one light-driver path validate the end-to-end behavior.
- Existing shader-based dimming semantics remain separate from the new explicit global brightness control.

## Validation Plan

Minimum validation:

- `pio test -e native-test`
- targeted bus and driver contract tests for brightness propagation and scaling

Recommended follow-up validation:

- focused protocol tests for any protocol that maps bus brightness into hardware gain
- smoke validation for representative RP2040 and ESP32 driver/transport environments if concrete implementations are updated there

## Risks

- If brightness is applied at more than one layer, output may be double-scaled.
- A transport seam that is too opinionated could force protocol concerns into transports that should stay byte-stream focused.
- A driver seam that is too weak could leave `LightBus` and `PixelBus` with divergent brightness semantics.
- Mapping `uint16_t` brightness into lower native ranges such as 5-bit or 7-bit hardware gain can create rounding edge cases.
- Contract changes at the seam level will touch many concrete implementations and can cause broad compile fallout if introduced carelessly.

## Resolved Design Decisions

- `IPixelBus` owns the public brightness API in the first pass.
- `PixelBus` applies first-pass brightness scaling before protocol encoding.
- `ITransport` receives brightness as a per-write argument.
- `ILightDriver::write()` accepts brightness at write time rather than through a cached setter.
- Brightness must be applied at exactly one authoritative layer per output path to prevent double-scaling.

## Remaining Open Question

- Which protocol, if any, should be the first hardware-gain integration target after the generic contract lands?

## Done When

The library has one explicit `uint16_t` global brightness path that starts at the bus, flows through transport and driver seams, preserves full-scale output by default, and can be validated with focused native tests without conflating the feature with shader-based dimming.