# Protocol Tests

Spec source: `docs/internal/testing-spec-protocols.md`

## Coverage Matrix

| Spec section | Domain | Suite | Status |
|---|---|---|---|
| 1.1 | DotStarProtocol | `test_protocol_spec_sections_1_1_to_1_4_and_1_14` | ✅ Implemented |
| 1.3 | Ws2801Protocol | `test_protocol_spec_sections_1_1_to_1_4_and_1_14` | ✅ Implemented |
| 1.4 | PixieProtocol | `test_protocol_spec_sections_1_1_to_1_4_and_1_14` | ✅ Implemented |
| 1.5 | Lpd6803Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.6 | Lpd8806Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.7 | P9813Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.8 | Sm168xProtocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.9 | Sm16716Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.11 | Tlc59711Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.12 | Tm1814Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.13 | Tm1914Protocol | `test_protocol_spec_sections_1_5_to_1_13` | ✅ Implemented |
| 1.14 | Ws2812xProtocol | `test_protocol_spec_sections_1_1_to_1_4_and_1_14` | ✅ Implemented |

## Suites

- `test_protocol_debug_pipeline`
- `test_protocol_spec_sections_1_1_to_1_4_and_1_14`
- `test_protocol_spec_sections_1_5_to_1_13`

## Run Commands

- `pio test -e native-test -f protocols/test_protocol_spec_sections_1_1_to_1_4_and_1_14`
- `pio test -e native-test -f protocols/test_protocol_spec_sections_1_5_to_1_13`
- `pio test -e native-test -f protocols/*`
