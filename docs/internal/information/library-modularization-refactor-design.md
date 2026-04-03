# Library Modularization Refactor Design

This document proposes a high-level refactoring of LumaWave into clearer conceptual libraries.

The goal is not to immediately split the repository into separate packages. The goal is to define a target architecture that makes future extraction, API cleanup, and dependency control straightforward.

## Goals

- Clarify which parts of the codebase are foundational versus compositional.
- Define clean dependency direction between major seams.
- Preserve the current virtual-first architecture and explicit seam contracts.
- Keep `PixelView` in the core kernel as a foundational pixel-view abstraction.
- Leave room for protocol-aware processing without collapsing protocol and shader responsibilities into one abstraction too early.

## Non-Goals

- Performing the repository split in this phase.
- Renaming public APIs purely for packaging aesthetics.
- Adding compatibility wrappers to preserve legacy surface area during the refactor.
- Reworking transport/platform code unless needed to improve library boundaries.

## Current High-Level Seams

The codebase already exposes four important seam contracts:

- `IPixelBus<TColor>`
- `IShader<TColor>`
- `IProtocol<TColor>`
- `ITransport`

Those contracts are sound, but the current umbrella layout in `src/LumaWave.h` flattens them into a single include surface. The internal directories are already close to a modular architecture:

- `src/core/`
- `src/colors/`
- `src/protocols/`
- `src/transports/`
- `src/buses/`

The main remaining question is where the pixel-buffer/view model belongs and how to evolve shader-like processing beyond pure pixel-space transforms.

## Proposed Conceptual Libraries

## 1. Core Kernel

This is the smallest foundational library.

### Responsibilities

- common compatibility helpers
- scalar utility types and aliases
- index/topology primitives that do not depend on pixel storage policy
- the minimal bus lifecycle abstraction

### Candidate contents

- `src/core/Compat.h`
- `src/core/Core.h` pieces that are true kernel-level types
- `src/core/IPixelBus.h`
- `src/core/IndexIterator.h`
- `src/core/PixelView.h`
- `src/core/Topology.h`
- `src/core/Writable.h`

### Explicit non-members

- concrete color types
- shaders
- protocols
- transports
- buses

### Rationale

The kernel should define the most stable contracts and the least opinionated types. `IPixelBus<TColor>` remains here because it is the runtime orchestration seam. `PixelView<TColor>` also belongs here because it is part of the foundational pixel-buffer access contract used across buses and composition layers. It is specialized, but still fundamental enough to the library's runtime model to justify kernel placement.

## 2. Pixel and Color Model

This library owns the color model and color-domain semantics.

### Responsibilities

- color type definitions
- channel-order and channel-map semantics
- color math
- palette models and generators

### Candidate contents

- `src/colors/Color.h`
- `src/colors/ChannelOrder.h`
- `src/colors/ChannelMap.h`
- `src/colors/ChannelSource.h`
- `src/colors/ColorMath.h`
- `src/colors/palette/*`

For the current direction, the cleaner conceptual model is:

- core kernel: lifecycle contract and topology primitives
- pixel/color model: color model and palette semantics

## 3. Shader Pipeline Library

This library owns pipeline-oriented frame processing that operates on pixel buffers before protocol encoding.

### Responsibilities

- shader contracts
- no-op shader policy
- pixel-buffer transforms that are part of the render pipeline rather than the color type system
- composition of multiple shader passes

### Candidate contents

- `src/colors/IShader.h`
- `src/colors/NilShader.h`
- `src/colors/AggregateShader.h`
- `src/colors/CurrentLimiterShader.h`
- `src/colors/GammaShader.h`
- `src/colors/AutoWhiteBalanceShader.h`
- `src/colors/CCTWhiteBalanceShader.h`

### Rationale

Shaders are pipeline concepts, not foundational color-model concepts.

They happen to operate on colors today, but that alone does not make them part of the color system. Their real role is to transform a frame buffer as part of the rendering path between pixel storage and protocol encoding.

Separating them from the color model gives cleaner ownership boundaries:

- color model owns representation and color-domain utilities
- shader pipeline owns render-time transforms over pixel data

This also makes it easier to evolve the render pipeline later without stretching the meaning of the color library.

## 4. Protocol Codec Library

This library owns conversion from colors to protocol byte streams.

### Responsibilities

- chip/frame encoding contracts
- protocol settings normalization
- protocol aliases and protocol default policy
- protocol decorators and frame-level protocol enrichment

### Candidate contents

- `src/protocols/IProtocol.h`
- `src/protocols/ProtocolAliases.h`
- `src/protocols/ProtocolDecoratorBase.h`
- concrete protocol implementations in `src/protocols/`

### Rationale

This is a true codec layer. Its job is to transform pixel-domain input into transport-ready byte streams or framed payloads.

### Boundary issue to address

Today the one-wire timing and encoding helpers live under `src/transports/`, but several protocols depend on them. Conceptually those helpers are closer to protocol framing support than hardware transport behavior.

Longer-term options:

1. Move one-wire timing and encoding support into the protocol library.
2. Create a small shared framing-support library used by protocols and transports.

Either option is cleaner than keeping protocol framing rules under transport.

## 5. Transport Abstractions and Platform Packs

This area should be treated as one conceptual family with two levels.

### 5.1 Transport base library

Responsibilities:

- transport lifecycle and write contract
- transport settings contract
- no-op, print, and generic transports

Candidate contents:

- `src/transports/ITransport.h`
- `src/transports/ILightDriver.h`
- `src/transports/NilTransport.h`
- `src/transports/NilLightDriver.h`
- `src/transports/PrintTransport.h`
- `src/transports/PrintLightDriver.h`
- `src/transports/SpiTransport.h`

### 5.2 Platform transport packs

Responsibilities:

- RP2040 transport implementations
- ESP32 transport implementations
- ESP8266 transport implementations
- NRF transport implementations

Candidate contents:

- `src/transports/rp2040/*`
- `src/transports/esp32/*`
- `src/transports/esp8266/*`
- `src/transports/nrf52/*`

### Rationale

The platform transports are the most hardware-specific code in the repository and are natural extraction boundaries.

## 6. Bus Runtime Library

This library composes the seams into runnable systems.

### Responsibilities

- storage ownership for root pixels and protocol buffers
- shader invocation at the pixel stage
- protocol update orchestration
- transport transaction orchestration
- aggregate and composite bus composition

### Candidate contents

- `src/buses/PixelBus.h`
- `src/buses/LightBus.h`
- `src/buses/ReferenceBus.h`
- `src/buses/ReferenceLightBus.h`
- `src/buses/AggregateBus.h`
- `src/buses/CompositeBus.h`

### Rationale

This is the assembly layer. It should sit above pixel/color, shader, protocol, and transport libraries.

`ReferenceBus` is especially important because it demonstrates the intended runtime seam composition using abstract ownership rather than template-only composition.

## 7. Public Facade Library

This is the umbrella include and ergonomic export surface.

### Responsibilities

- end-user aliases
- grouped includes
- public convenience names and defaults

### Candidate contents

- `src/LumaWave.h`

### Rationale

This is not a foundational subsystem. It is a facade layer that depends on the other conceptual libraries.

## Proposed Dependency Direction

The target dependency direction should be:

- core kernel: no dependencies on higher layers
- pixel/color model: depends on core kernel
- shader pipeline library: depends on pixel/color model and core kernel
- protocol codec library: depends on pixel/color model and core kernel
- transport base library: depends on core kernel
- platform transport packs: depend on transport base library and core kernel
- bus runtime library: depends on pixel/color model, shader pipeline library, protocol codec library, and transport libraries
- public facade: depends on all libraries it re-exports

In short:

`core -> pixel/color`

`pixel/color -> shaders`

`pixel/color -> protocols`

and separately:

`core -> transports -> buses`

with bus runtime depending on shaders, protocols, and transports

with the public facade on top.

## Shader Placement and Future Protocol-Aware Processing

This is the main architectural decision that needs care.

## Current model

Current shaders are pixel-space transforms:

- input: `span<TColor>`
- output: mutated `span<TColor>`

That is a good fit for a shader pipeline library.

Examples:

- gamma adjustment
- current limiting
- white balance
- aggregate pixel transforms

These should remain in the shader pipeline library.

## Problem statement

There is interest in processing that is aware of protocol characteristics or interacts with protocol framing behavior.

Examples of future needs:

- protocol-dependent dithering policy
- frame encoding transforms tied to chip packing rules
- per-protocol gain shaping or bit-depth adaptation
- byte-stream inspection, instrumentation, or post-encode transforms

If these are all forced into `IShader<TColor>`, the shader contract becomes confused:

- Is it a pixel-space transform?
- Is it a protocol decorator?
- Is it a byte-stream filter?

That ambiguity would weaken the current seams.

## Recommended direction

Keep `IShader<TColor>` pixel-domain only, but move it into a dedicated shader pipeline library.

Introduce a second family for protocol-aware processing when needed.

There are two viable designs.

### Option A. Protocol decorators

Use protocol decorators for behavior that wraps a protocol implementation and changes or observes protocol update behavior.

This is already aligned with:

- `src/protocols/ProtocolDecoratorBase.h`
- the existing protocol-decorator design notes

Best fit for:

- instrumentation
- policy enforcement
- protocol-specific framing changes
- protocol-side gain, dithering, or packing policy

Advantages:

- preserves a clean distinction between pixel transforms and protocol transforms
- composes naturally with protocol aliases/specs
- keeps `PixelBus` orchestration simple

### Option B. Generalized frame pipeline stages

Introduce explicit processing stages in the bus runtime:

- pixel stage: operates on `span<TColor>`
- protocol stage: operates on protocol object or protocol settings
- byte stage: operates on `span<uint8_t>` after encoding

This would likely require new interfaces rather than expanding `IShader<TColor>`.

Possible examples:

- `IPixelStage<TColor>`
- `IProtocolStage<TColor>` or decorator contract
- `IByteStreamStage`

Advantages:

- most explicit long-term pipeline model
- makes room for post-encode filters and protocol-aware transforms

Disadvantages:

- more complexity in bus construction and ownership
- easy to over-generalize before real use cases are well-defined

## Recommendation

Near term:

- keep shaders in their own shader pipeline library
- treat protocol-aware behavior as protocol decorators, not shaders

Longer term:

- only introduce a generalized multi-stage frame pipeline if multiple real features need both protocol-stage and byte-stage processing

This keeps the architecture crisp now while leaving a credible upgrade path.

## PixelView and Bus Contract Position

`PixelView<TColor>` should remain in the core kernel together with `IPixelBus<TColor>`.

Rationale:

- the bus seam already exposes `PixelView<TColor>` directly
- aggregate and composite bus logic depend on pixel-view chunk composition as a foundational runtime mechanism
- keeping the view type in kernel avoids an unnecessary cross-library inversion between `IPixelBus` and the type it returns

This means the current kernel is not a purely generic utility layer. It is a runtime core for pixel-oriented buses, and `PixelView` is part of that runtime core.

## Suggested Refactor Phases

## Phase 1. Document and stabilize boundaries

- define the conceptual libraries in documentation
- stop adding new cross-layer includes casually
- treat `PixelView` as part of the core kernel in new discussions and refactors
- treat shaders as pipeline-stage components rather than color-library components

## Phase 2. Reduce misplaced dependencies

- relocate one-wire timing/encoding support out of transport-only ownership
- reduce default-transport selection coupling inside `PixelBus`
- identify headers that can be moved without public API churn

## Phase 3. Separate facade from internals

- keep `src/LumaWave.h` as an umbrella only
- avoid letting umbrella export decisions drive internal layering

## Phase 4. Introduce protocol-aware processing through decorators

- implement real protocol decorators where needed
- validate whether decorator-based composition covers the actual use cases

## Phase 5. Reassess need for a generalized frame pipeline

- only if decorators and pixel shaders prove insufficient
- only after at least two or three concrete protocol-stage or byte-stage features exist

## Design Rules Going Forward

- `IShader<TColor>` means pixel-space pipeline transform only.
- Protocol-aware behavior should begin as a protocol decorator, not as a shader extension.
- `PixelView` belongs to the core kernel and should stay aligned with the bus seam.
- Shaders belong to a pipeline library even when their payload type is color.
- Transport libraries should own hardware transfer behavior, not protocol semantics.
- The umbrella facade should not be treated as proof that all modules belong in one layer.

## Open Questions

- Should one-wire timing/encoding support live under protocols or in a small shared framing-support library?
- Is there a genuine future need for post-encode byte-stream processing, or will protocol decorators cover the expected use cases?
- Should the shader pipeline eventually grow a more explicit stage model than `IShader<TColor>` if pre-encode processing diversifies?
- Should `LightBus` remain inside the same bus runtime library, or eventually split into a distinct single-light runtime package?

## Recommended Current Position

Use the following model as the working architecture:

- core kernel
- pixel/color model
- shader pipeline library
- protocol codec library, including protocol decorators
- transport base plus platform transport packs
- bus runtime composition layer
- public facade

Within that model, protocol-aware processing should evolve beside protocols first, not inside the shader abstraction, and shaders should be treated as a dedicated render-pipeline concern rather than part of the color system.