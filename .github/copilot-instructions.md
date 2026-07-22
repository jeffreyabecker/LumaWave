# Copilot Instructions for LumaWave

## Project Context

- Repository: `LumaWave`.
- Primary target: PlatformIO + Arduino core, with RP2040/Pico2W as the default workflow.
- Language standard for active code paths: C++17 (`-std=gnu++17`) in primary and native-test environments.
- Architecture is virtual-first and centered on explicit seams: `IPixelBus`, `Protocol`, and `Transport`.
- Project stage is alpha: API compatibility is not preserved by default and public APIs are assumed unstable unless explicitly stated otherwise.

## Skills

For detailed C++ authoring rules — style, architecture patterns, Bus construction, Arduino boundaries, testing gates, and examples guidance — invoke the `/cpp-authoring` skill.

