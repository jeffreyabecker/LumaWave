# Test Index

Top-level index for native test categories and spec-driven suites.

## Spec Source Documents

- [Documentation Index](../docs/README.md)
- [Object Model Contracts](../docs/internal/information/object-model-contracts.md)
- [One-Wire Timing Reference](../docs/internal/information/one-wire-timing-reference.md)
- [Bitrate Calculation Guide](../docs/internal/information/bitrate-calculation-guide.md)

## Category READMEs

- [Bus Tests](busses/README.md)
- [Transport Tests](transports/README.md)
- [Protocol Tests](protocols/README.md)
- [Topology Tests](topologies/README.md)

## Spec-Driven Suites

- [Protocol+Transport Contract Compile Suite](contracts/test_factory_descriptor_first_pass_compile)
- [Nil Template Compile Smoke](test_nil_template_compile)
- [Topology Spec Section 2](topologies/test_topology_spec_section2)
- [Protocol Spec Sections 1.1-1.4 + 1.14](protocols/test_protocol_spec_sections_1_1_to_1_4_and_1_14)
- [Protocol Spec Sections 1.5-1.13](protocols/test_protocol_spec_sections_1_5_to_1_13)

## Quick Run

- Full native suite: `ctest --test-dir build --output-on-failure`
- Configure first if not already done: `cmake -S . -B build`
- Build: `cmake --build build --config Debug`
- Run a single test by name:
  - `ctest --test-dir build -R test_factory_descriptor_first_pass_compile --output-on-failure`
- Run all tests in a category (regex filter):
  - `ctest --test-dir build -R test_protocol --output-on-failure`
  - `ctest --test-dir build -R test_shader --output-on-failure`
  - `ctest --test-dir build -R test_bus --output-on-failure`
  - `ctest --test-dir build -R test_topology --output-on-failure`
