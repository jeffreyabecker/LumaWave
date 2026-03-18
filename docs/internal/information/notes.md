# Internal Research Notes

This file is for reference material and durable technical observations only.
Actionable work items are tracked in [docs/internal/backlog/todo.md](../backlog/todo.md).

## Reference Links

- DotStar family comparison matrix:
	- https://www.superlightingled.com/blog/sk9822-vs-apa102-vs-apa107-vs-hd107-vs-hd107s-vs-hd108/
- DotStar white-only product/tutorial/datasheet references:
	- https://www.adafruit.com/product/2433
	- https://learn.adafruit.com/adafruit-dotstar-leds
	- https://cdn-learn.adafruit.com/assets/assets/000/084/592/original/APA102_White_LED.pdf?1574117639
- LED datasheet hub:
	- https://www.ledyilighting.com/addressable-pixel-ic-datasheet-hub/
- Candidate fixed-point library for evaluation:
	- https://github.com/MikeLankamp/fpm

## Protocol/Encoding Notes

- 3-step vs 4-step one-wire encoding cadence is a protocol-level trade-off.
- 3-step expands payload by about 1:6 and targets ~2.4 MHz encoded bit rate.
- 4-step expands payload by about 1:8, simplifies arithmetic/no carry-over path, and targets ~3.2 MHz encoded bit rate.

## Scope Notes

- Unsupported-chip triage and decisions are maintained in [docs/internal/backlog/lumawave-unsupported-chips.md](../backlog/lumawave-unsupported-chips.md) and tracked as tasks in [docs/internal/backlog/todo.md](../backlog/todo.md).

## Protocol Descriptor Alias Audit (2026-03-02)

### Scope reviewed

- `src/factory/descriptors/ProtocolDescriptors.h`
- `src/factory/traits/ProtocolDescriptorTraits.Ws2812x.h`
- `src/factory/traits/ProtocolDescriptorTraits.DotStar.h`
- `src/factory/traits/ProtocolDescriptorTraits.Hd108.h`
- Compile-first contract coverage in:
	- `test/contracts/test_protocol_aliases_first_pass_compile/test_main.cpp`
	- `test/contracts/test_factory_descriptor_first_pass_compile/test_main.cpp`

### Findings

- No channel-order mismatches were found between descriptor defaults and trait normalization behavior for the audited alias set.
- `Ws2812x`-family descriptors correctly apply `DefaultChannelOrder` through trait normalization rather than relying on protocol constructor defaults.
- DotStar/HD108 descriptor traits correctly normalize channel order from descriptor defaults.
- `Tm1829` alias is consistent with the chosen policy:
	- timing profile: `timing::Tm1829`
	- default order: `RGB`
	- idle-high/invert behavior propagated via descriptor trait transport mutation.

### Follow-up actions

- Keep compile-first coverage for new aliases mandatory (default channel-order + timing/invert assertions).
- If token-based runtime parser aliasing is expanded, add a dedicated token-to-descriptor coverage test to protect synonym mappings.



the raspberry pi pico supports memory to memory DMA transfers and dma pacing timers -- could we use this to bit-bang out gpios?
Would that appraoch work on other platforms? -- Esp* no the dma controller only allows memory to memory within ram


*nRF devices (such as nRF52/nRF53) do not have a generic "memory-to-peripheral" DMA channel for raw GPIO. Instead, they use EasyDMA linked with PPI (Programmable Peripheral Interconnect) and GPIOTE to move data from memory to GPIO ports. This requires a timer to trigger the transfer, allowing the creation of fast, low-latency pin patterns by accessing GPIO->OUT or GPIO->OUTSET registers. 


Consider if we should separate the Protocol wire buffer and the transport buffer -- this is likely needed to support bit-banging out over gipo

Can we make a shift-register based Pwm output where we clock out the 