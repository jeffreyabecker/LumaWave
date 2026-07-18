# LumaWave vs FastLED — Architecture & Feature Comparison

> **Status:** Draft — first-pass technical comparison.  
> **FastLED version analyzed:** 3.10.3 (`library.properties`)  
> **LumaWave revision:** HEAD as of 2026-03-03

---

## 1  Architectural Philosophy

### 1.1  Dispatch Model

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Primary model | Hybrid: template-instantiated chipset/platform controllers attached to runtime `fl::CLEDController` linked list | Virtual-first seam model (`IPixelBus`, `Protocol`, `Transport`) |
| User-facing construction | `FastLED.addLeds<CHIPSET, ...>(...)` compile-time selection | `makeBus(...)` / descriptor-driven factory (static + dynamic paths) |
| Runtime polymorphism | Yes at controller list level (`virtual show()/showColor()`) | Yes across all seam boundaries |
| Runtime reconfiguration | Limited (enable/disable controllers, selected runtime controls on some platforms) | Core design goal via descriptors + `DynamicBusBuilder`/INI |
| Heterogeneous multi-strip orchestration | Global singleton (`FastLED`) iterates registered controllers | Native via shared `IPixelBus` graph/composite buses |

#### Deeper Analysis: Hybrid vs Virtual-First

FastLED combines two strategies: template specialization for chipset/platform implementation details, then runtime iteration via a linked list of `CLEDController` instances. This gives very good per-platform optimization while still allowing a single `FastLED.show()` call over multiple strips.

LumaWave places explicit architectural seams at bus/protocol/transport boundaries and keeps that abstraction first-class in both static and dynamic factory paths. In exchange for some virtual dispatch overhead, it enables cleaner runtime composition and descriptor-driven wiring.

### 1.2  Separation of Concerns

| Concern | FastLED | LumaWave |
|---------|---------|----------|
| Pixel container | Caller-owned `CRGB[]` (or span wrappers in newer internals) | Bus-owned/accessor-managed typed color buffers |
| Protocol encoding | Typically embedded in chipset/controller templates | Isolated in `Protocol::update()` implementations |
| Hardware transfer | Platform controller classes (RMT/SPI/clockless/etc.) | Isolated in `Transport` |
| Color transforms | Global brightness/dither/correction pipeline + utility functions | Shader pipeline (`IShader`, `AggregateShader`) |
| Public composition seam | Primarily through `FastLED` global API and controller registration | Explicit interface seams intended for composition/testing |

#### Deeper Analysis: Coupling Profile

FastLED’s architecture is pragmatic for Arduino-style sketches: add controller, mutate `CRGB` buffer, call `show()`. The trade-off is that protocol and transport responsibilities often live inside tightly-coupled controller families, which makes “swap just protocol” or “swap just transport” less explicit at API boundaries.

LumaWave intentionally forces these boundaries to stay independent, making protocol/transport combinations and shader stack ordering more explicit and testable as separate units.

### 1.3  Ownership & Lifetime Model

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Pixel memory ownership | External by default (user array passed to controller via `setLeds`) | Internal/accessor-based ownership models with explicit borrowing/owning paths |
| Controller registration | Runtime linked list (`m_pHead`, `m_pNext`) | Factory-created bus objects and explicit composition |
| Global singleton dependence | Strong (`FastLED`) | Optional (factory helpers), architecture is interface-first |
| Dynamic allocation profile | Platform/controller dependent, plus optional features | Concentrated in explicit bus/driver ownership constructs |

---

## 2  C++ Language, Build, and Testing

### 2.1  Language Standard and Portability

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Baseline language mode (observed build config) | C++11 in Meson config (`cpp_std=c++11`) | C++17 (`-std=gnu++17`) |
| Primary portability target | Very broad, including small AVR class devices | Focused modern MCU targets (RP2040/ESP32/ESP8266 + native tests) |
| Template usage | Extensive chipset/platform templating + compatibility wrappers | Extensive traits/factory templating with C++17 utilities |

#### Deeper Analysis: Standard Trade-off

FastLED’s C++11 baseline helps maintain very broad hardware compatibility. LumaWave’s C++17 baseline improves expressiveness and maintainability in descriptor/trait-heavy architecture, but intentionally narrows legacy-toolchain support.

### 2.2  Build and CI Posture

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Build ecosystem | Arduino + PlatformIO + CMake/Meson/test tooling in repo | PlatformIO-centric project workflow |
| Public CI footprint | Broad multi-platform workflows and native test lanes | Focused native + target environments in `platformio.ini` |
| Host/native emphasis | Strong (`tests/` with Meson + wrappers + many suites) | Strong (`native-test` Unity + ArduinoFake) |

### 2.3  Testing Model

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Automated tests | Extensive `tests/` tree and CI workflows | Extensive spec-driven tests under `test/` |
| Contract compile tests | Implicit through broad compile matrix + targeted suites | Explicit contract test suites for factory/protocol/transport seams |
| Testability seams | Controller-level + platform stubs/mocks | Explicit seam-level tests (`Protocol`, `Transport`, shader, factory) |

---

## 3  Color and Rendering Pipeline

### 3.1  Color Types and API Ergonomics

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Core color type | `CRGB` (+ HSV utilities, palettes, etc.) | Parameterized/aliased RGB-based color family |
| Typical app model | Mutate a user `CRGB[]` frame buffer | Mutate bus buffer, then protocol/shader pipeline on `show()` |
| 2D/effects utilities | Rich built-in effects/math/noise/palette ecosystem | Core architecture-first, with composable shader/topology building blocks |

### 3.2  Dithering, Brightness, and Correction

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Brightness handling | Global `setBrightness()` scaling and show-time application | Shader- and bus-pipeline oriented control |
| Dithering | Built-in controller-level temporal dithering model | Deterministic shader pipeline focus (feature-dependent) |
| Composition style | Global pipeline knobs + utility functions | Explicit shader stage composition (`AggregateShader`) |

### 3.3  Power Management

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Core capability | Built-in power budgeting APIs (`setMaxPowerInVoltsAndMilliamps`, related calculators) | `CurrentLimiterShader` active frame-level limiting |
| Application model | Brightness constrained around `show()` lifecycle | Shader-enforced budget inside pipeline |
| Model extensibility | Power model structs (`PowerModelRGB`, RGBW/RGBWW placeholders) | Shader-level policy extensibility |

---

## 4  Protocol and Transport Layering

### 4.1  One-Wire and Clocked Families

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Timing strategy | Centralized timing definitions plus platform-specialized controllers | Protocol descriptors + shared one-wire timing/encoding abstractions |
| Protocol/transport separability | Varies by controller family; often coupled in controller implementation | First-class separation (`Protocol` + `Transport`) |
| New chipset onboarding shape | Usually add/extend chipset/controller templates and platform paths | Usually add protocol descriptor/protocol class; reuse transport seam |

### 4.2  Runtime Driver Selection

| Aspect | FastLED | LumaWave |
|--------|---------|----------|
| Runtime driver selection | Emerging/feature-specific (not uniform across all platforms) | Descriptor/runtime builder model is central architecture |
| Cross-platform behavior contract | Platform-dependent implementation depth | Contract-first via transport tags and compatibility checks |

---

## 5  Practical Fit Guidance

### Favor **FastLED** when:

- You want the classic Arduino sketch workflow with a large ecosystem of examples/effects.
- You need extremely broad board/platform coverage, including older AVR-class targets.
- You prefer a single global orchestration API and direct `CRGB` buffer manipulation.

### Favor **LumaWave** when:

- You need explicit protocol/transport/shader seams for modular architecture.
- You want descriptor-driven composition and dynamic bus construction patterns.
- You want compile-time contract validation that mirrors the virtual-first runtime design.

---

## 6  Key Takeaway

Both libraries are mature but optimize for different engineering priorities:

- **FastLED** optimizes for ecosystem breadth, platform reach, and familiar sketch ergonomics.
- **LumaWave** optimizes for explicit architecture seams, composability, and contract-driven extensibility.

For teams building long-lived, multi-protocol systems with evolving runtime composition needs, LumaWave’s seam-first design is typically the better structural fit. For rapid sketch-driven visual work across diverse boards, FastLED remains a strong default.
