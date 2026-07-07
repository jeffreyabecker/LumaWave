# Platform Library Split Backlog

Purpose: split platform-specific transport implementations into their own CMake libraries so the build graph matches the library's seam architecture, generic consumers stop inheriting platform baggage transitively, and the project is positioned for a later C++23 modules rollout.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: not started (but informed by completed color/shader simplifications).

The repository currently exposes a single top-level `LumaWave` interface target in CMake, while public headers still mix generic and platform-specific transport surfaces. The existing transport layout already groups platform code under `src/transports/rp2040/`, `src/transports/esp32/`, and `src/transports/esp8266/`, but those groupings are not yet reflected in the build graph. As a result, target boundaries, include boundaries, and architectural seams do not currently align.

The shader layer has been eliminated (backlog 003 complete), and the color layer has been simplified to a fixed 4-channel `RgbwColor<TComponent>` with no template channel-count parameter (backlog 002 phases 1-2 complete). These simplifications let the target graph collapse: `lumawave_color` merges into `lumawave_core`, and the thin transport-seam layer merges into `lumawave_protocol`. The resulting generic graph has only three targets.

This backlog focuses on introducing target-level separation for platform packs first. It does not require the full native-SDK migration or C++23 module conversion to happen in the same change set, but it should leave the codebase ready for both.

## Design Intent

- Treat platform transport packs as explicit build targets rather than preprocessor-controlled subsets of one flat interface target.
- Keep generic seam targets platform-agnostic.
- Require consumers to opt into a platform library explicitly instead of inheriting all platform code through the top-level facade.
- Keep the target graph small and meaningful; do not create one CMake target per concrete transport class.
- Use the split to prepare for eventual mapping between CMake targets and public C++23 named modules.

## Scope

- Introduce target-level separation between generic library layers and platform transport packs.
- Create explicit platform targets for RP2040 and ESP32.
- Make an explicit keep-or-defer decision for ESP8266 before finalizing its target.
- Remove platform-header leakage from generic umbrella surfaces where needed to support the split.
- Add focused validation so generic tests can build without linking or including platform packs.
- Update documentation to describe the new target layout and opt-in platform model.

## Non-Goals

- Do not perform the full RP2040 Pico SDK migration in this backlog.
- Do not perform the full ESP-IDF migration in this backlog.
- Do not convert the public surface to named modules in this backlog.
- Do not preserve old behavior by adding broad compatibility shims that keep all platform packs visible everywhere.
- Do not create per-transport CMake libraries such as one target for each UART, SPI, I2S, RMT, or PWM implementation.

## Architectural Rules

- No generic target may depend on a platform target.
- Platform targets may depend on generic seam targets, but not on unrelated platform packs.
- The top-level facade target must not force all platform targets into every consumer.
- Keep platform-specific compile definitions, include paths, and SDK hooks scoped to the platform target that owns them.
- Prefer deleting umbrella leakage over adding new preprocessor gates to preserve it.
- If a platform pack is header-only at first, it may begin as an `INTERFACE` target; if it gains bridge sources later, convert it to `STATIC` or `OBJECT` rather than pushing those details back into generic targets.

## Target Shape

The intended target shape for this backlog is:

- `lumawave_core` (absorbs `lumawave_color`)
- `lumawave_protocol` (absorbs `lumawave_transport`)
- `lumawave_bus`
- `lumawave_platform_rp2040`
- `lumawave_platform_esp32`
- `lumawave_platform_esp8266` only if intentionally retained
- `lumawave` as an optional facade target over the common public surface

This backlog does not require every generic target above to be perfect on the first pass, but the split should move the codebase toward that shape and away from a single flat `LumaWave` target.

## Target Graph Proposal

The proposed target graph for the first complete split is:

```text
lumawave_core
	-> low-level compatibility, span/type aliases, core utilities,
	   topology primitives, writable abstractions,
	   color types, channel maps, color math, palette domain/types

lumawave_protocol
	-> lumawave_core
	-> generic transport seams (ITransport, ILightDriver, NilTransport, etc.),
	   generic transport implementations, protocol seams and implementations

lumawave_bus
	-> lumawave_core
	-> lumawave_protocol
	-> PixelBus, LightBus, composite/reference bus types

lumawave_platform_rp2040
	-> lumawave_protocol
	-> lumawave_bus
	-> RP2040-specific transports, drivers, and RP2040 convenience entry points

lumawave_platform_esp32
	-> lumawave_protocol
	-> lumawave_bus
	-> ESP32-specific transports, drivers, and ESP32 convenience entry points

lumawave_platform_esp8266
	-> lumawave_protocol
	-> lumawave_bus
	-> ESP8266-specific transports, drivers, and ESP8266 convenience entry points
	-> only if explicitly retained

lumawave
	-> lumawave_core
	-> lumawave_protocol
	-> lumawave_bus
	-> optional generic facade only; must not link platform targets transitively
```

The intended ownership split is:

- `lumawave_core`: `src/core/` (Compat.h, Core.h, IPixelBus.h, PixelView.h, Topology.h, Writable.h, etc.) and `src/colors/` (Color.h, ChannelMap.h, ChannelOrder.h, ChannelSource.h, ColorChannelIndexIterator.h, ColorMath.h, palette/).
- `lumawave_protocol`: `src/transports/` generic seam headers (ITransport.h, ILightDriver.h, NilTransport.h, NilLightDriver.h, PrintTransport.h, PrintLightDriver.h, OneWireTiming.h, OneWireEncoding.h, generic SpiTransport.h) plus `src/protocols/` (protocol seams and implementations, protocol aliases).
- `lumawave_bus`: `src/buses/` and any generic factory or facade helpers that compose protocol-layer primitives without naming a concrete platform.
- `lumawave_platform_*`: only the concrete platform implementations and platform-specific convenience headers.

The graph should remain acyclic and intentional:

- generic targets may include other generic targets only in the dependency direction above.
- platform targets may depend on generic layers, but generic layers may not depend on platform targets.
- the generic `lumawave` facade should stay thin enough that a native-only consumer can link it without platform SDK headers becoming part of the translation unit surface.

## Public Header Split Plan

The public header split should make platform inclusion opt-in instead of implicit.

### Generic public surface

The generic surface should remain reachable from:

- `src/LumaWave.h`
- `src/core/Core.h`
- `src/colors/Colors.h`
- `src/transports/Transports.h`
- `src/protocols/Protocols.h`
- `src/buses/Busses.h`

The generic surface should own:

- all seam interfaces and generic utilities.
- all protocol declarations and aliases that do not require a platform implementation header.
- all bus types that operate on explicitly selected transport or driver types.
- generic defaults such as `NilTransport` or host-safe behavior.

The generic surface should stop owning:

- conditional includes of `src/transports/rp2040/*`, `src/transports/esp32/*`, and `src/transports/esp8266/*` from `src/transports/Transports.h`.
- platform-default type aliases in generic umbrella headers.
- convenience aliases in `src/LumaWave.h` that silently select concrete RP2040, ESP32, or ESP8266 transport implementations.

### Platform entry points

Each retained platform pack should get explicit public entry points. The exact file names can be adjusted, but the shape should look like:

- `src/platform/rp2040/Rp2040.h`
- `src/platform/esp32/Esp32.h`
- `src/platform/esp8266/Esp8266.h` if retained

Each platform entry point should:

- include the generic `LumaWave.h` facade or the narrower generic headers it needs.
- include only that platform's transport and driver headers.
- define any platform-local convenience aliases such as `PlatformDefaultTransport`, `PlatformDefaultLightDriver`, or platform-oriented `Strip` aliases if those concepts are retained.

### Alias migration rules

The split should move alias ownership as follows:

- `lw::busses::PlatformDefaultTransport` should stop existing in generic bus headers.
- `lw::transports::PlatformDefaultLightDriver` should stop existing in generic transport headers.
- generic `Strip` and `Light` aliases in `src/LumaWave.h` should either require explicit transport/driver types or degrade to host-safe generic defaults.
- if ergonomic platform defaults are still wanted, they should be reintroduced under explicit platform entry points rather than in the generic facade.

### Header transition sequence

The intended order of operations is:

1. make `src/transports/Transports.h` generic-only.
2. remove platform default aliases from `src/buses/PixelBus.h` and `src/LumaWave.h`.
3. add platform-specific umbrella headers that restore convenience for consumers who explicitly opt in.
4. update examples and platform-focused docs to include the new explicit platform entry points.
5. add compile smoke coverage that proves the generic headers no longer leak platform implementation headers.

## Monorepo Migration Plan

This backlog should treat the current repository as a monorepo with layered packages rather than immediately splitting into separate repositories.

### Stage 0 - Baseline inventory

- identify all generic headers that still include platform headers directly or indirectly.
- identify all aliases and defaults that currently select concrete platform implementations from generic headers.
- identify all tests and examples that assume `LumaWave.h` exposes every retained platform surface.

### Stage 1 - Internal target split

- replace the single `LumaWave` interface target with the layered generic targets plus explicit platform targets.
- keep the repository layout intact while changing only target ownership and include discipline.
- preserve a compatibility `lumawave` facade target that represents the generic surface only.

Definition of success for this stage:

- native tests can link generic targets without any platform pack transitively linked.
- retained platform packs can be linked independently by focused compile coverage.

### Stage 2 - Public header split inside the monorepo

- introduce explicit platform umbrella headers under a platform-specific public path.
- move platform-default aliases and convenience types behind those platform headers.
- update examples so platform-focused sketches opt into the correct platform umbrella instead of relying on generic leakage.

Definition of success for this stage:

- `src/LumaWave.h` remains a generic facade.
- platform examples and platform consumers compile only when they include their selected platform entry point.

### Stage 3 - Packaging shape inside the monorepo

- document the intended package boundaries even if the build still ships from one repository.
- treat `lumawave_core`, `lumawave_protocol`, `lumawave_bus`, and each retained `lumawave_platform_*` target as the canonical packaging units.
- ensure installation, export, and documentation paths can describe those units cleanly.

Definition of success for this stage:

- the repo can publish one source archive while still expressing multiple coherent consumer-facing targets.
- future extraction to package feeds or separate repos becomes a packaging decision rather than an architectural refactor.

### Stage 4 - Optional later extraction

Once the monorepo target and header split is stable, the project can make a later decision to:

- keep a single repository with multiple published packages.
- keep a single repository with one release artifact but explicit target-level consumption.
- extract selected platform packs into separate repositories if release cadence or SDK ownership demands it.

This stage is intentionally deferred. The backlog should optimize first for clean boundaries inside the current repo, because that reduces risk and preserves the ability to choose a later packaging model based on real maintenance pressure instead of early guesswork.

## Work Phases

## Phase 1 - Baseline Target Graph Cleanup

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| PLS-01 | todo | Replace the single flat `LumaWave` interface target with a small layered target graph for generic code (`lumawave_core`, `lumawave_protocol`, `lumawave_bus`, optional facade `lumawave`) | none | CMake exposes the generic targets and existing native tests still configure and build against the facade or the relevant lower-layer targets |
| PLS-02 | todo | Remove platform-target dependency assumptions from generic targets so no generic layer pulls in `rp2040`, `esp32`, or `esp8266` headers transitively | PLS-01 | Generic targets configure without platform include paths or platform compile definitions |

## Phase 2 - Public Surface Cleanup For Opt-In Platforms

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| PLS-03 | todo | Reduce `src/transports/Transports.h` to generic transport headers only and move platform-specific includes behind explicit platform entry points | PLS-02 | `Transports.h` no longer includes RP2040, ESP32, or ESP8266 headers |
| PLS-04 | todo | Remove platform-default alias coupling from the generic facade so `src/LumaWave.h` does not implicitly select or expose all platform transport implementations | PLS-03 | The generic facade remains usable without dragging platform implementation headers into non-platform consumers |

## Phase 3 - RP2040 And ESP32 Platform Targets

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| PLS-05 | todo | Create `lumawave_platform_rp2040` and assign ownership of the headers under `src/transports/rp2040/` to that target | PLS-03 | RP2040 transport headers are reachable through the RP2040 target and are no longer part of the generic target surface |
| PLS-06 | todo | Create `lumawave_platform_esp32` and assign ownership of the headers under `src/transports/esp32/` to that target | PLS-03 | ESP32 transport headers are reachable through the ESP32 target and are no longer part of the generic target surface |
| PLS-07 | todo | Scope platform-specific compile definitions, include paths, and warnings to the RP2040 and ESP32 targets instead of global or generic target settings | PLS-05, PLS-06 | RP2040 and ESP32 target requirements are expressed locally in CMake and do not leak into unrelated consumers |

## Phase 4 - ESP8266 Decision And Target Ownership

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| PLS-08 | todo | Make an explicit keep-or-defer decision for the ESP8266 transport pack based on whether the platform still has a credible forward path under the new architecture | PLS-03 | The backlog and docs clearly mark ESP8266 as retained or deferred |
| PLS-09 | todo | If ESP8266 is retained, create `lumawave_platform_esp8266` and assign ownership of the headers under `src/transports/esp8266/` to that target | PLS-08 | ESP8266 transport headers live behind an explicit target and are not part of the generic target surface |

## Phase 5 - Tests And Validation

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| PLS-10 | todo | Update native tests so generic suites link only the generic library targets unless a platform pack is explicitly under test | PLS-04, PLS-07 | Generic native tests build without platform targets linked transitively |
| PLS-11 | todo | Add focused compile smoke coverage that verifies generic consumers can include or link the generic facade without platform implementation headers | PLS-04 | A dedicated compile test fails if platform headers leak back into the generic surface |
| PLS-12 | todo | Add targeted platform-pack compile coverage for RP2040 and ESP32 target wiring | PLS-07 | Each platform target has at least one focused build gate that validates its include and dependency wiring |

## Phase 6 - Documentation And Module Alignment

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| PLS-13 | todo | Update architecture docs to describe the target split, opt-in platform consumption, and the relationship between CMake targets and planned C++23 modules | PLS-07, PLS-08 | The target graph and platform-pack policy are documented in the relevant internal design docs |
| PLS-14 | todo | Record the target-to-module mapping so `lumawave_platform_rp2040` and `lumawave_platform_esp32` have a defined migration path to `lw.platform.rp2040` and `lw.platform.esp32` | PLS-13 | The documentation defines the intended correspondence between CMake targets and future module interface units |

## Verification Gates

- `cmake -S . -B build`
- `cmake --build build --config Debug`
- `ctest --test-dir build -C Debug --output-on-failure`
- focused generic compile smoke that proves no platform headers leak into the generic facade
- focused compile coverage for each retained platform target

## Open Questions

- Should `PlatformDefaultLightDriver` remain a generic facade concept at all, or should platform defaults move behind explicit platform entry points?
- Does `SpiTransport.h` remain in the combined `lumawave_protocol` target, or should it become a platform- or adapter-owned surface as the Arduino boundary disappears?
- Is ESP8266 still strategic enough to justify a dedicated target and future SDK work?
- Should platform-specific convenience headers live under `src/platform/` rather than under `src/transports/` once the target split is in place?