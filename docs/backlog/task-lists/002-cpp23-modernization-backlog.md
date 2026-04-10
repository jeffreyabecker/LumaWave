# C++23 Modernization Backlog

Purpose: remove the `tcb/span.hpp` third-party shim and the companion `lw::remove_cvref_t` compat shim by adopting their standard counterparts, tighten `_v`/`_t` alias usage throughout active headers, and establish C++23 as the minimum standard for native (non-embedded) build targets. Embedded targets retain C++17 support paths where divergence is required by the toolchain.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Implementation Status

Current status: not started.

All headers are currently compiled as C++17 via CMake (`CMAKE_CXX_STANDARD 17`). The `tcb::span` backport (`src/third_party/tcb/span.hpp`) is guarded by `LW_HAS_STD_SPAN`; the gate logic lives in `src/core/Compat.h`. Two other `Compat.h`-resident shims cover `remove_cvref_t`. Additionally, `src/core/Core.h` hard-includes `third_party/tcb/span.hpp` directly, bypassing the `Compat.h` gate entirely.

Several active headers use `std::is_same<T, U>::value` and `std::is_base_of<B, D>::value` in annotation-heavy template constraints where `_v` aliases are already available in C++17, and verbose `std::enable_if` guards exist where `if constexpr` would reduce noise on recent compilers.

C++23 is the right modernization target because it gives widespread availability of `std::print`, `std::expected`, and improvements to `std::ranges`, but the span and `remove_cvref_t` removals are the primary driver here. `concept`/`requires`-based constraint rewrites are explicitly deferred to avoid public-seam C++20-only surface per project rules.

## Design Intent

- C++23 is the compile target for native (CMake) configurations going forward; embedded targets remain C++17-compatible through the Compat layer.
- The `tcb/span.hpp` third-party file is deleted. `lw::span` becomes a direct alias of `std::span`. The `LW_HAS_STD_SPAN` macro and its fallback branch in `Compat.h` are removed.
- The `lw::remove_cvref_t` shim is removed. All uses are migrated to `std::remove_cvref_t` directly.
- `std::is_same<T, U>::value` is replaced with `std::is_same_v<T, U>` and equivalents (`std::is_base_of_v`, `std::is_convertible_v`, `std::is_constructible_v`) throughout active headers, as these are C++17 and already unblocked.
- `std::enable_if_t` constraints that guard standalone free functions with no compiler-version conflict may be replaced with `if constexpr` where the function body naturally supports it; return-type-based SFINAE (`std::enable_if_t` as return type) requires case-by-case review.
- `concept` and `requires`-clause rewrites are intentionally out of scope for this backlog; they would introduce C++20-only public surface against project rules.
- The CMake standard bump to C++23 applies only to native build targets. The compatibility layer in `Compat.h` is updated to reflect the new baseline but retains embedded-safe fallback guards for the Arduino environment.

## Scope

- Deletion of `src/third_party/tcb/` directory and `span.hpp`
- Removal of `LW_HAS_STD_SPAN` gate and the `tcb::span` fallback branch in `Compat.h`
- Removal of the `#include "third_party/tcb/span.hpp"` stray direct include in `Core.h`
- Removal of the `lw::remove_cvref_t` compat shim and all its call sites migrated to `std::remove_cvref_t`
- Migration of `std::is_same<T, U>::value` to `std::is_same_v<T, U>` and equivalent `_v` aliases in all active non-embedded headers
- CMake standard bump from C++17 to C++23 for the native build target
- Native test suite verification after each phase
- Documentation update for the new minimum native standard

## Non-Goals

- Do not introduce `concept` or `requires` keyword constraints on any public seam header in this backlog; that is a separate phase
- Do not modify embedded-target transport headers (`esp32/`, `rp2040/`) beyond removing the `lw::remove_cvref_t` call-site migration if safe; flag any unsafe ones as deferred
- Do not upgrade the Arduino-facing API to require C++23-only features from consumers
- Do not add `std::expected` usage; that is a separate modernization concern
- Do not remove the `if defined(LW_HAS_ARDUINO)` or similar embedded-compat guards from `Compat.h`

## Architectural Rules

- Change one shim per task; do not combine span removal and `remove_cvref_t` removal into a single commit.
- After removing each shim, verify the native test suite passes before proceeding to the next task.
- Prefer removing code over commenting it out; the deleted shim files should not be left as stubs.
- Where `_v` alias migration is mechanical, do it in a single pass per header group rather than one file at a time.
- The `lw::span` and `lw::dynamic_extent` aliases in `Compat.h` may be retained as thin aliases over `std::span` to avoid churn at call sites; do not require a call-site sweep for `lw::span` → `std::span` unless the aliases are removed.

## Work Phases

## Phase 1 - CMake Standard Bump

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-01 | todo | Bump `CMAKE_CXX_STANDARD` from `17` to `23` in the root `CMakeLists.txt` native build configuration | none | `cmake -S . -B build && cmake --build build --config Debug` succeeds with C++23 on MSVC and GCC; `ctest` exits zero |

## Phase 2 - span Shim Removal

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-02 | todo | Remove the stray direct `#include "third_party/tcb/span.hpp"` from `src/core/Core.h` and verify `Core.h` still compiles correctly via the `Compat.h` gate | MOD-01 | `Core.h` no longer references `tcb` directly; native build and all tests still pass |
| MOD-03 | todo | Remove the `LW_HAS_STD_SPAN` detection block and the `tcb::span` fallback branch from `src/core/Compat.h`; update `lw::span` and `lw::dynamic_extent` to alias `std::span` and `std::dynamic_extent` unconditionally | MOD-02 | `Compat.h` contains no `tcb` reference; `lw::span<T>` resolves to `std::span<T>`; native build and all tests pass |
| MOD-04 | todo | Delete `src/third_party/tcb/span.hpp` and the `src/third_party/tcb/` directory | MOD-03 | The file and directory are absent from the repository; no remaining source file references `tcb` |

## Phase 3 - remove_cvref_t Shim Removal

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-05 | todo | Remove the `lw::remove_cvref_t` compat shim from `src/core/Compat.h` (both the `__cpp_lib_remove_cvref` guard and the manual fallback) | MOD-01 | The shim definition is absent from `Compat.h` |
| MOD-06 | todo | Migrate all `lw::remove_cvref_t` usages in non-embedded headers (`buses/`, `colors/`, `core/`, `protocols/`, `factory/`) to `std::remove_cvref_t` | MOD-05 | `grep -r "lw::remove_cvref_t" src/` returns no matches in the affected directories; native build and all tests pass |
| MOD-07 | todo | Audit and migrate `lw::remove_cvref_t` usages in embedded transport headers (`transports/esp32/`, `transports/rp2040/`); flag any that cannot safely adopt `std::remove_cvref_t` under their C++17 toolchain as `deferred` | MOD-05 | Each usage is either migrated or has a documented reason for deferral; file is updated in the backlog accordingly |

## Phase 4 - _v Alias Cleanup

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-08 | todo | Replace `std::is_same<T, U>::value` with `std::is_same_v<T, U>` in all active non-embedded headers | MOD-01 | No `std::is_same<[^>]+>::value` pattern remains in `src/` outside of embedded transport headers; native build and all tests pass |
| MOD-09 | todo | Replace `std::is_base_of<B, D>::value`, `std::is_convertible<F, T>::value`, and `std::is_constructible<T, ...>::value` with their `_v` equivalents in all active non-embedded headers | MOD-08 | Equivalent to MOD-08 definition of done for the additional trait families |

## Phase 5 - Verification And Documentation

| ID | Status | Task | Depends On | Definition of Done |
|---|---|---|---|---|
| MOD-10 | todo | Run the full native test suite on Windows (MSVC) after all shim removals and confirm all tests pass | MOD-04, MOD-07, MOD-09 | `ctest --test-dir build -C Debug --output-on-failure` exits zero; no test regressions introduced by removed shims |
| MOD-11 | todo | Run the full native test suite on Linux (GCC) after all shim removals and confirm all tests pass | MOD-04, MOD-07, MOD-09 | `ctest --test-dir build --output-on-failure` exits zero on GCC with `-std=gnu++23` |
| MOD-12 | todo | Update `ReadMe.md` and any compilation-flags documentation to reflect C++23 as the native build standard and note embedded targets remain C++17 | MOD-10, MOD-11 | `docs/usage/compilation-flags.md` and `ReadMe.md` correctly describe the native C++23 baseline and the Arduino/embedded C++17 compatibility layer |
