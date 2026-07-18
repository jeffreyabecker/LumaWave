# Object Model Contracts

This document defines the current object-model contracts enforced by the virtual-first architecture in `src/`.

## Goals

- Keep protocol/transport compatibility explicit and compile-time enforced.
- Keep bus runtime behavior focused on frame flow, not runtime type validation.
- Keep protocol frame-buffer ownership outside protocol implementations.
- Keep seam contracts (`IPixelBus`, `IProtocol`, `ITransport`) minimal and stable.

---

## 1) Seam Interfaces

### 1.1 `IPixelBus<TColor>`

`IPixelBus<TColor>` is the runtime bus surface:

- `begin()`
- `show()`
- `isReadyToUpdate() const`
- `pixelBuffer()` / `pixelBuffer() const`
- `topologyOrNull() const`

Concrete bus implementations may expose additional helpers (for example pixel-count or buffer-assignment helpers), but these are not part of the virtual seam contract.

### 1.2 `IProtocol<TColor>`

`IProtocol<TColor>` is the protocol seam and owns canonical protocol pixel count via the base constructor.

Required virtual behavior:

- `begin()`
- `update(span<const TColor>)`
- `isReadyToUpdate() const`
- `alwaysUpdate() const`

Current optional/default virtual behavior:

- `setBuffer(span<uint8_t>)` (default no-op)
- `bindTransport(ITransport*)` (default no-op)

Contract metadata and markers:

- `ColorType`
- `SettingsType`
- `TransportCategory`
- `RequiresExternalBuffer` (defaults to `true`)

External frame-buffer contract (current implementation):

- Protocol byte buffers are externally supplied through `setBuffer(...)`.
- `StaticBus`/`UnifiedStaticBus` bind protocol transport and byte slices before normal updates.

### 1.3 `ITransport`

`ITransport` is the hardware/peripheral transfer seam.

Required virtual behavior:

- `begin()`
- `transmitBytes(span<uint8_t>)`

Optional/default virtual behavior:

- `beginTransaction()`
- `endTransaction()`
- `isReadyToUpdate() const` (default `true`)

Transmit lifetime invariant:

- Bytes passed to `transmitBytes(...)` must remain valid until readiness is restored.

---

## 2) Compile-Time Type Contracts

The codebase enforces the following constexpr trait contracts.

### 2.1 Protocol traits

- `ProtocolType<TProtocol>`
  - Requires `TProtocol::SettingsType` and `TProtocol::TransportCategory`.

- `ProtocolMoveConstructible<TProtocol>`
  - Requires protocol type to be move-constructible.

- `ProtocolExternalBufferRequired<TProtocol>`
  - Requires `TProtocol::RequiresExternalBuffer == true`.

- `ProtocolRequiredBufferSizeComputable<TProtocol>`
  - Requires static `requiredBufferSize(uint16_t, const SettingsType&)` convertible to `size_t`.

- `ProtocolPixelSettingsConstructible<TProtocol>`
  - Requires all of:
    - protocol type contract,
    - move-constructible protocol,
    - external-buffer-required marker,
    - static required-buffer-size contract,
    - non-`void` `SettingsType`,
    - move-constructible `SettingsType`,
    - constructor `(uint16_t, SettingsType)`.

- `ProtocolSettingsTransportBindable<TProtocol>`
  - Requires `SettingsType` with member `bus` (detected by presence).

- `ProtocolTransportCompatible<TProtocol, TTransport>`
  - Requires protocol type + `TransportLike<TTransport>` + category compatibility.

### 2.2 Transport traits

- `TransportSettingsWithInvert<TSettings>`
  - Requires public member `invert` with exact `bool` type.

- `TransportLike<TTransport>`
  - Requires aliases:
    - `TransportCategory`
    - `TransportSettingsType`
  - Requires `TTransport*` convertible to `ITransport*`.
  - Requires `TransportSettingsType` to satisfy `TransportSettingsWithInvert`.

- `TaggedTransportLike<TTransport, TTag>`
  - `TransportLike` + exact tag equality.

- `SettingsConstructibleTransportLike<TTransport>`
  - `TransportLike` + constructor `(TransportSettingsType)`.

### 2.3 Category compatibility

Transport category tags:

- `AnyTransportTag`
- `TransportTag`
- `OneWireTransportTag`

`TransportCategoryCompatible<ProtocolTag, TransportTag>` is valid when:

- both tags derive from `AnyTransportTag`, and
- protocol tag is `AnyTransportTag`, **or** transport tag exactly matches protocol tag.

Implications:

- `OneWireTransportTag` protocols require one-wire transport category.
- `TransportTag` protocols require transport category.
- `AnyTransportTag` protocols accept any category.

---

## 3) Bus Driver and Factory Wiring Contracts

### 3.1 Bus-driver constraints

`FactoryTypeConstraints.h` enforces:

- `FactoryProtocolLike<TProtocol>`
  - Requires protocol to derive from `IProtocol<typename TProtocol::ColorType>`.

- `FactoryProtocolSettingsConstructible<TProtocol, TTransport>`
  - Accepts either:
    - `ProtocolPixelSettingsConstructible<TProtocol>`, or
    - constructor `(uint16_t, SettingsType, TTransport&)`.

- `FactoryProtocolTransportCompatible<TProtocol, TTransport>`
  - Requires bus-driver protocol shape + transport compatibility by category.

### 3.2 Construction behavior

`factory::makeOwningBusProtocol(...)` construction order is:

1. If `ProtocolSettingsTransportBindable<TProtocol>` is true, assign `settings.bus = &transport` and construct `(pixelCount, settings)`.
2. Else if protocol has ctor `(pixelCount, settings, transport&)`, use it.
3. Else construct `(pixelCount, settings)`.

`StaticBus::bindProtocolBuffers()` then:

- calls `protocol.bindTransport(transport)` for each strand,
- computes total required bytes from `ConcreteProtocol::requiredBufferSize(pixelCount, settings)`,
- slices/binds protocol arena via `protocol.setBuffer(...)`.

### 3.3 Transport-binding ownership decision (2026-03-02)

Current decision: keep transport binding protocol-owned via `IProtocol::bindTransport(ITransport*)`.

Rationale:

- Existing protocol implementations call transport methods directly (`begin()`, `beginTransaction()`, `transmitBytes()`, `endTransaction()`, `isReadyToUpdate()`), so binding remains part of protocol runtime setup.
- Construction paths already centralize binding in `StaticBus`/`DynamicBus` strand wiring and factory setup; moving this responsibility into `PixelBus` now would create churn without functional gain.
- Protocol-level tests rely on direct protocol + transport pairing independent of bus ownership, and that workflow remains useful for byte-stream validation.

Re-open criteria:

- If protocol/transport interaction is refactored so protocols no longer hold transport pointers directly, revisit promoting binding ownership to `PixelBus`.

---

## 4) Current Compile Contract Coverage

Contract assertions currently live in compile-oriented suites under `test/contracts/`, especially:

- `test/contracts/test_factory_descriptor_first_pass_compile/`

These checks cover protocol/transport descriptor compatibility, one-wire wrapper compatibility paths, and protocol buffer-size computability.

Recommended targeted run:

- `pio test -e native-test --filter contracts/test_factory_descriptor_first_pass_compile`

---

## 5) Authoring Checklist

When adding a protocol:

- Inherit from `IProtocol<TColor>`.
- Define `using ColorType`, `using SettingsType`, `using TransportCategory`.
- Keep `RequiresExternalBuffer == true` unless deliberately changing global protocol-buffer policy.
- Implement static `requiredBufferSize(uint16_t, const SettingsType&)`.
- Implement `setBuffer(...)` and `bindTransport(...)` if the protocol needs external byte arena and transport injection.
- Provide either `(uint16_t, SettingsType)` or `(uint16_t, SettingsType, TTransport&)` constructor path.
- Add/update compile assertions in `test/contracts/` suites.

When adding a transport:

- Make `TTransport*` convertible to `ITransport*`.
- Define `using TransportCategory`, `using TransportSettingsType`.
- Ensure `TransportSettingsType` exposes `public bool invert`.
- Support `(TransportSettingsType)` construction for descriptor/factory paths.
- Set category tag correctly (`TransportTag` vs `OneWireTransportTag`).
- Add/update compile assertions in `test/contracts/` suites.

When adding factory descriptors/traits:

- Ensure protocol descriptor traits resolve protocol/settings/color consistently.
- Ensure transport descriptor traits resolve transport/settings and capability correctly.
- Preserve capability compatibility rules for direct vs wrapped one-wire paths.
- Add/update compile assertions in `test/contracts/` suites.

---

## 6) Practical Rule of Thumb

- Encode compatibility intent in transport tags and descriptor capabilities.
- Enforce that intent with compile-time traits and static assertions.
- Use runtime tests for behavior (timing, encoding, readiness), not for type-shape validation.
