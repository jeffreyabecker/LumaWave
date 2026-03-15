# Test Index

Top-level index for native test categories and spec-driven suites.

## Spec Source Documents

- [Documentation Index](../docs/README.md)
- [Object Model Contracts](../docs/internal/information/object-model-contracts.md)
- [One-Wire Timing Reference](../docs/internal/information/one-wire-timing-reference.md)
- [Bitrate Calculation Guide](../docs/internal/information/bitrate-calculation-guide.md)

## Category READMEs

- [Bus Tests](busses/README.md)
- [Shader Tests](shaders/README.md)
- [Transport Tests](transports/README.md)
- [Protocol Tests](protocols/README.md)
- [Topology Tests](topologies/README.md)

## Spec-Driven Suites

- [Protocol+Transport Contract Compile Suite](contracts/test_factory_descriptor_first_pass_compile)
- [Nil Template Compile Smoke](test_nil_template_compile)
- [Topology Spec Section 2](topologies/test_topology_spec_section2)
- [Color/Shader Spec Section 1](shaders/test_color_domain_section1)
- [Color/Shader Spec Section 3](shaders/test_current_limiter_shader_section3)
- [Color/Shader Spec Section 4](shaders/test_aggregate_shader_section4)
- [Protocol Spec Sections 1.1-1.4 + 1.14](protocols/test_protocol_spec_sections_1_1_to_1_4_and_1_14)
- [Protocol Spec Sections 1.5-1.13](protocols/test_protocol_spec_sections_1_5_to_1_13)
- [PixelBus Shader Buffer Behavior](busses/test_bus_shader_buffer_behavior)

## Quick Run

- Full native suite: `pio test -e native-test`
- Protocol+transport contract compile matrix:
  - `pio test -e native-test --filter contracts/test_factory_descriptor_first_pass_compile`
- Nil template compile smoke:
  - `pio test -e native-test --filter test_nil_template_compile`
- Bus + topology suites:
  - `pio test -e native-test --filter busses/test_bus_shader_buffer_behavior`
  - `pio test -e native-test --filter topologies/test_topology_spec_section2`
- Shader suites:
  - `pio test -e native-test --filter shaders/test_color_domain_section1`
  - `pio test -e native-test --filter shaders/test_current_limiter_shader_section3`
  - `pio test -e native-test --filter shaders/test_aggregate_shader_section4`
- Protocol suites:
  - `pio test -e native-test --filter protocols/test_protocol_spec_sections_1_1_to_1_4_and_1_14`
  - `pio test -e native-test --filter protocols/test_protocol_spec_sections_1_5_to_1_13`
