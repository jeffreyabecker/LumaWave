# Arduino-Optional Plan

## Goal

Make the library as Arduino-optional as practical while preserving the current virtual-first architecture and C++17 public surface.

The target end-state is:

- Core and generic public headers compile without `Arduino.h` present.
- `#include <LumaWave.h>` works in native and other non-Arduino environments without relying on ArduinoFake.
- Arduino-specific behavior is isolated to explicit platform-edge headers.
- Only `AnalogPwmLightDriver` and `SpiTransport` remain intentionally Arduino-bound in the generic transport surface.

## Non-Goals

- Preserving accidental Arduino transitive includes for old call sites.
- Introducing compatibility shims solely to hide API changes.
- Requiring C++20 surface features to solve header partitioning.
- Solving every platform-specific transport in a single refactor pass.

## Architectural Rule

The correct boundary is not "guard everything with macros." The correct boundary is:

- Generic/core contracts must not depend on Arduino headers or Arduino types.
- Arduino adapters may depend on Arduino headers.
- Platform transports should prefer platform SDK APIs over Arduino wrappers when those SDK APIs are already part of the implementation model.
- Umbrella headers must only include headers that are valid for their audience.

In practice that means the repository should have a clean split between:

- generic transport and protocol contracts
- platform or Arduino adapter implementations
- umbrella includes for generic use versus Arduino convenience use

## Current Dependency Leaks

### 1. Arduino in seam headers

`src/transports/Transport.h` currently conditionally includes `Arduino.h` to obtain SPI-related constants such as `MSBFIRST` and `SPI_MODE0`.

This is the wrong layer. `Transport` is a generic seam and should own its own transport-level defaults.

### 2. Public umbrella pulls in Arduino-bound headers

`src/transports/Transports.h` currently includes a wide set of transport headers, including Arduino-bound and platform-bound implementations.

Because `src/LumaWave.h` includes `src/transports/Transports.h`, the top-level public include surface inherits those dependencies.

This is the highest-value leak to fix because it affects every consumer, including native tests and virtual-only users.

### 3. Generic print/debug facilities still expose Arduino aliases

The following headers are structurally generic but still have Arduino-centric default aliases:

- `src/protocols/DebugProtocol.h`
- `src/transports/PrintTransport.h`
- `src/transports/PrintLightDriver.h`

These should remain generic over the `Writable` concept and only provide Arduino-specific aliases when an Arduino adapter header is explicitly included.

### 4. Platform implementations use Arduino helpers directly

Several RP2040, ESP32, and ESP8266 transport or light-driver headers currently include `Arduino.h` and call Arduino functions such as:

- `pinMode`
- `yield`
- `micros`
- `Serial` / `Serial1`

That is acceptable only at the platform edge, but some of these headers can be made Arduino-optional by switching to platform SDK calls.

## Desired Header Topology

### Generic surface

These headers should be Arduino-free:

- `src/core/Compat.h`
- `src/transports/Transport.h`
- `src/transports/ILightDriver.h`
- `src/transports/NilTransport.h`
- `src/transports/NilLightDriver.h`
- `src/transports/PrintTransport.h`
- `src/transports/PrintLightDriver.h`
- `src/protocols/DebugProtocol.h`
- `src/transports/Transports.h`
- `src/protocols/Protocols.h`
- `src/LumaWave.h`

### Intentionally Arduino-bound surface

These remain Arduino-specific by design unless a later refactor changes that decision:

- `src/transports/SpiTransport.h`
- `src/transports/AnalogPwmLightDriver.h`

### Platform-specific surface

These should be evaluated individually:

- RP2040 transport and PWM driver headers
- ESP32 transport and PWM driver headers
- ESP8266 transport headers

The preferred direction is to make RP2040 and ESP32 headers Arduino-optional where the implementation already relies on Pico SDK or ESP-IDF APIs.

## Phased Plan

## Phase 1: Stop Arduino leakage from top-level includes

### Objective

Make `LumaWave.h` and the generic transport umbrella safe for non-Arduino builds.

### Changes

1. Remove Arduino dependency from `src/transports/Transport.h`.
2. Replace Arduino-derived SPI constants with library-owned defaults or enums in the generic transport layer.
3. Reduce `src/transports/Transports.h` to generic headers only.
4. Stop including `SpiTransport.h` from the generic transport umbrella.
5. Stop including platform-specific transport and light-driver headers from the generic transport umbrella.
6. Keep Arduino convenience includes behind a distinct Arduino-only umbrella or guarded inclusion path.

### Expected outcome

- `#include <LumaWave.h>` no longer drags in Arduino headers.
- Native compile tests can include the public umbrella without ArduinoFake.

## Phase 2: Make print/debug facilities generic-first

### Objective

Keep writable-based debugging and print sinks available outside Arduino.

### Changes

1. Keep `PrintTransportT` and `PrintLightDriverT` generic over `Writable`.
2. Keep `NullWritable` as the non-Arduino fallback.
3. Move Arduino-specific aliases such as `PrintTransport`, `PrintLightDriver`, and any `Serial` default wiring behind explicit Arduino-only adapter headers or guarded alias blocks that do not require `Arduino.h` in the generic header.
4. Apply the same split to `DebugProtocol` defaults.

### Expected outcome

- Debug and print features remain available in native builds using non-Arduino writable sinks.
- Arduino ergonomics remain available without contaminating the generic surface.

## Phase 3: De-Arduino the RP2040 implementation set

### Objective

Make RP2040 transports depend on Pico SDK or internal platform helpers instead of Arduino helpers.

### Target headers

- `src/transports/rp2040/RpDmaManager.h`
- `src/transports/rp2040/RpPioTransport.h`
- `src/transports/rp2040/RpSpiTransport.h`
- `src/transports/rp2040/RpUartTransport.h`
- `src/transports/rp2040/RpPwmLightDriver.h`

### Changes

1. Replace `micros()` usage with a Pico SDK timer source or a narrow internal timing helper.
2. Replace `yield()` with a platform-appropriate wait or cooperative hook.
3. Replace `pinMode()` with direct SDK GPIO configuration where appropriate.
4. Remove unnecessary `SPI.h` inclusion where only transport constants or settings are needed.

### Expected outcome

- RP2040 transport headers become Arduino-optional.
- Native and non-Arduino RP2040-oriented builds stop requiring Arduino compatibility headers.

## Phase 4: De-Arduino the ESP32 implementation set

### Objective

Use ESP-IDF-level APIs directly where the implementation already depends on ESP-IDF facilities.

### Target headers

- `src/transports/esp32/Esp32LedcLightDriver.h`
- `src/transports/esp32/Esp32SigmaDeltaLightDriver.h`
- `src/transports/esp32/Esp32DmaSpiTransport.h`
- `src/transports/esp32/Esp32I2sTransport.h`
- `src/transports/esp32/Esp32RmtTransport.h`

### Changes

1. Replace `pinMode()` calls with direct GPIO configuration where appropriate.
2. Replace `yield()` with a lower-level wait or scheduler-safe platform hook.
3. Remove `Arduino.h` includes unless a header truly needs an Arduino-only API.

### Expected outcome

- ESP32 transport headers become Arduino-optional or at least dramatically narrower in their Arduino dependence.

## Phase 5: Make an explicit ESP8266 decision

### Objective

Avoid carrying an ambiguous architecture where ESP8266 remains accidentally Arduino-only.

### Observed issue

The ESP8266 DMA UART and I2S implementations currently rely on Arduino-specific APIs and objects more heavily than the RP2040 and ESP32 implementations, including `Serial`, `Serial1`, special pin modes, and Arduino timing helpers.

### Options

1. Rewrite ESP8266 transports against lower-level SDK facilities.
2. Declare ESP8266 transport support Arduino-only and isolate it behind the Arduino umbrella.

### Recommendation

Do not decide this implicitly during unrelated cleanup. Make it a documented design choice.

## Phase 6: Keep the remaining Arduino-only surface narrow

### Objective

Finish with an obvious, intentional Arduino boundary.

### Rules

1. `src/transports/SpiTransport.h` may remain Arduino-only.
2. `src/transports/AnalogPwmLightDriver.h` may remain Arduino-only.
3. If possible, keep settings types generic even when the implementation class is Arduino-specific.
4. Do not let Arduino-only headers re-enter `src/LumaWave.h` through convenience umbrellas.

## Verification Plan

### Compile gates

Run these existing gates after each contract-sensitive phase:

- `pio test -e native-test`
- `pio test -e native-test --filter contracts/test_factory_descriptor_first_pass_compile`

### New compile smoke to add

Add a native suite that:

1. includes `LumaWave.h`
2. does not include ArduinoFake
3. instantiates only generic protocol and bus types
4. verifies that the public umbrella stays Arduino-free

### Additional focused checks

- Include-only compile smoke for `src/transports/Transports.h`
- Include-only compile smoke for `src/protocols/Protocols.h`
- Targeted native compile checks for any new Arduino-only adapter umbrellas

## Suggested Execution Order

The recommended order is:

1. generic umbrella cleanup
2. generic print/debug cleanup
3. RP2040 refactor
4. ESP32 refactor
5. ESP8266 decision and implementation
6. docs and examples cleanup

This order delivers the highest-value user-facing improvement first: non-Arduino consumers can include the public library surface without inheriting Arduino dependencies.

## Design Constraints To Preserve

- Keep the public code path compatible with C++17.
- Preserve the virtual-first seam model: `IPixelBus`, `Protocol`, `Transport`.
- Do not add compatibility overloads or shims merely to preserve previous accidental include behavior.
- Keep transport settings types satisfying the required `public bool invert` contract.
- Keep platform details at the transport/platform edge, not in the generic protocol or bus layers.

## Immediate First Tasks

The first implementation pass should do only the following:

1. remove Arduino from `src/transports/Transport.h`
2. split generic versus Arduino transport umbrellas
3. stop `src/LumaWave.h` from including Arduino-only transport headers transitively
4. add a native include-only smoke test for `LumaWave.h`

If those four tasks are complete, the repository will have a real Arduino-optional foundation instead of a macro-based approximation.