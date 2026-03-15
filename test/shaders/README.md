# Shader Tests

Category folder for native shader-focused tests.

## Spec Coverage Matrix

Source spec: `docs/internal/testing-spec-colors-shaders.md`

| Spec Section | Domain | Test Folder | Status |
|---|---|---|---|
| 1 | Color Domain | `test/shaders/test_color_domain_section1` | Implemented, Passing |
| 3 | CurrentLimiterShader Domain | `test/shaders/test_current_limiter_shader_section3` | Implemented, Passing |
| 4 | AggregateShader Domain | `test/shaders/test_aggregate_shader_section4` | Implemented, Passing |
| 5 | Alternative Color Models (HSL/HSB) | `test/shaders/test_color_models_section5` | Implemented |
| 6 | Color Manipulation Primitives | `test/shaders/test_color_manipulation_section6` | Implemented |
| 8 | CCTWhiteBalanceShader Domain | `test/shaders/test_cct_white_balance_shader_section8` | Implemented |

## Run

- Full native suite: `pio test -e native-test`
- Shader-only subset:
	- `pio test -e native-test --filter shaders/test_color_domain_section1`
	- `pio test -e native-test --filter shaders/test_current_limiter_shader_section3`
	- `pio test -e native-test --filter shaders/test_aggregate_shader_section4`
	- `pio test -e native-test --filter shaders/test_color_models_section5`
	- `pio test -e native-test --filter shaders/test_color_manipulation_section6`
	- `pio test -e native-test --filter shaders/test_cct_white_balance_shader_section8`
