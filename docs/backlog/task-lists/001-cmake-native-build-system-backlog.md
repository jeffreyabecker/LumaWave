# CMake Native Build System Backlog

Purpose: define and sequence the work needed to migrate the native build and test pipeline from PlatformIO to CMake, targeting MSVC on Windows and GCC on Linux, while leaving embedded targets (RP2040, ESP32, ESP8266) on PlatformIO unchanged.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: not started.

All native compilation and test execution currently runs through PlatformIO's `native` platform environment (`env:native-test`), which uses:

- ArduinoFake (fetched from GitHub via `lib_deps`) to stub Arduino headers
- Unity as the test framework (driven by PlatformIO's test runner)
- Build flags: `-Isrc -Iinclude -Wall -Wextra -Wno-unused-parameter`
- C++17 standard mode (`-std=gnu++17` on GCC/Clang via PlatformIO defaults)

No CMake configuration exists in the repository. Embedded targets are explicitly out of scope for this effort and will continue to be built and flashed through PlatformIO.

## Design Intent

- CMake is authoritative for native builds and native test execution on Windows and Linux.
- PlatformIO remains authoritative for all embedded target builds and uploads; it is not removed or replaced for those environments.
- MSVC is the native compiler on Windows; GCC is the native compiler on Linux. No cross-compiler targets are required.
- The test suite under `test/` runs via CTest and continues to use Unity as the assertion framework.
- ArduinoFake continues to satisfy the Arduino header dependency boundary for native compilation; it is fetched and configured via CMake instead of PlatformIO's `lib_deps`.
- The C++ standard is C++17 in both compiler environments. MSVC uses `/std:c++17`; GCC uses `-std=gnu++17`.
- Build flag parity with the PlatformIO native environment is maintained across compilers where semantically equivalent flags exist.

## Scope

- Root `CMakeLists.txt` and supporting CMake module structure
- Compiler detection and per-compiler flag configuration (MSVC / GCC)
- ArduinoFake integration as a CMake FetchContent or find-package dependency
- Unity test framework integration under CTest
- All existing test targets under `test/` compiled and passing under the new build system
- Toolchain documentation covering configure, build, and test invocations on both platforms

## Non-Goals

- Do not replace or remove PlatformIO for embedded targets (RP2040, ESP32, ESP8266)
- Do not add Clang or AppleClang as supported native toolchains in this effort
- Do not introduce a meta-build layer that drives both CMake and PlatformIO from a single command
- Do not alter existing test logic, test names, or Unity assertion patterns to fit CMake
- Do not add CI pipeline configuration; focus is on local developer workflow only

## Architectural Rules

- Keep CMake configuration minimal and flat initially; do not introduce deep target hierarchies before the need is established.
- Express compiler differences through generator expressions or per-compiler blocks, not by duplicating target definitions.
- Do not use `include_directories` or `add_definitions` globally; prefer target-scoped `target_include_directories` and `target_compile_definitions`.
- ArduinoFake and Unity are test-only dependencies; do not link them into non-test targets.
- Preserve the include path contract `src/` and `include/` from the PlatformIO native environment.
- Use `enable_testing()` and `add_test()` at the root level so `ctest` can discover all test targets without per-directory invocations.

## Work Phases

## Phase 1 - CMake Foundation

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-01 | done | Author root `CMakeLists.txt` with project declaration, C++17 standard enforcement, include path setup, and an `INTERFACE` library target consumable by downstream applications via `add_subdirectory()` | none | `cmake -S . -B build` completes without errors on both Windows (MSVC) and Linux (GCC); `LumaWave::LumaWave` INTERFACE target is defined with `src/` on its include path and `cxx_std_17` as a compile feature |
| CMAKE-02 | todo | Add per-compiler warning flag configuration equivalent to PlatformIO native flags | CMAKE-01 | MSVC receives `/W3 /wd4100` (or equivalent `-Wno-unused-parameter` mapping); GCC receives `-Wall -Wextra -Wno-unused-parameter`; flags are expressed via generator expressions or `if(MSVC)` blocks |
| CMAKE-03 | todo | Verify `src/LumaWave.h` and all headers under `src/` compile cleanly as a header-only interface target | CMAKE-01, CMAKE-02 | An empty translation unit `#include <LumaWave.h>` that links `LumaWave::LumaWave` compiles without errors or warnings on both compilers |

## Phase 2 - ArduinoFake Integration

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-04 | todo | Integrate ArduinoFake via CMake FetchContent using the same repository URL currently in `platformio/cfg/native.ini` | CMAKE-01 | `FetchContent_MakeAvailable(ArduinoFake)` succeeds and exposes an importable target or include path for Arduino stubs |
| CMAKE-05 | todo | Define a reusable CMake INTERFACE target that bundles ArduinoFake include paths with `LumaWave::LumaWave` for test targets to consume | CMAKE-03, CMAKE-04 | Test targets link one dependency and receive both ArduinoFake stubs and LumaWave includes without repeating paths |
| CMAKE-06 | todo | Confirm that ArduinoFake under MSVC compiles without modification or requires a documented patch | CMAKE-04 | Either ArduinoFake compiles cleanly under MSVC or the required patch/flag workaround is documented and applied in the CMake configuration |

## Phase 3 - Unity And Test Target Registration

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-07 | todo | Integrate Unity test framework via CMake FetchContent or find-package | CMAKE-01 | Unity headers and the Unity C translation unit are available to test targets; Unity compiles cleanly under both MSVC and GCC |
| CMAKE-08 | todo | Register one representative test directory (for example `test/contracts/`) as a CMake test executable and CTest target | CMAKE-05, CMAKE-07 | `cmake --build build --target <test_exe>` builds the target; `ctest --test-dir build -R <test_exe>` runs it and reports pass/fail |
| CMAKE-09 | todo | Enumerate and register all remaining test directories under `test/` as CTest targets | CMAKE-08 | Every test subdirectory that builds under `pio test -e native-test` has a corresponding CTest target; `ctest --test-dir build` runs all of them |
| CMAKE-10 | todo | Confirm all registered tests pass on Windows under MSVC | CMAKE-09 | `ctest --test-dir build` exits zero on a clean Windows build with no skipped or failing tests |
| CMAKE-11 | todo | Confirm all registered tests pass on Linux under GCC | CMAKE-09 | `ctest --test-dir build` exits zero on a clean Linux build with no skipped or failing tests |

## Phase 4 - Documentation And Developer Workflow

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| CMAKE-12 | todo | Document the configure, build, and test invocation sequence for Windows (MSVC) | CMAKE-10 | A doc or README section covers `cmake -S . -B build`, `cmake --build build`, and `ctest --test-dir build` for the MSVC workflow including any required VS environment setup |
| CMAKE-13 | todo | Document the configure, build, and test invocation sequence for Linux (GCC) | CMAKE-11 | The same doc covers the GCC equivalent workflow |
| CMAKE-14 | todo | Update `ReadMe.md` or a dedicated build doc to note that native testing can be driven by either PlatformIO or CMake | CMAKE-12, CMAKE-13 | The repository readme does not imply PlatformIO is the only native test path |
