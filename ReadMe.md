# LumaWave

[![Donate](https://img.shields.io/badge/paypal-donate-yellow.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6AA97KE54UJR4)

RP2040 addressable LED library — native build and test via CMake.

A library for RP2040 boards (Raspberry Pi Pico family) to control NeoPixel-style LED strips.

For quick questions and support:  
* [GitHub Discussions](https://github.com/Makuna/NpbNext/discussions)  
* [Discord Invitation](https://discord.gg/c6FrysvZyV) or if you are already a member of [Discord Server](https://discord.com/channels/789177382221119519/789177382221119521)  

For bugs, make sure there isn't an active issue and then create one.

## Documentation
[See Wiki](https://github.com/Makuna/NpbNext/wiki)

Local references in this repo:
* [Documentation Index](docs/README.md)
* [Examples Index](examples/README.md)

## Code Formatting

This repository uses `clang-format` with project settings from `.clang-format`.

VS Code workspace settings in `.vscode/settings.json` are configured to format C/C++ files on save.

To format source files from PowerShell:

```powershell
Get-ChildItem -Recurse -Path src,include,test,examples -Include *.h,*.hpp,*.c,*.cc,*.cpp,*.ino |
    ForEach-Object { clang-format -i $_.FullName }
```

## Building And Testing (Native)

Requires CMake 3.25+ and either MSVC (Windows) or GCC (Linux).

```powershell
# Configure
cmake -S . -B build

# Build
cmake --build build --config Debug

# Run all tests
ctest --test-dir build -C Debug --output-on-failure
```

Or use the VS Code tasks (`CMake: Configure`, `CMake: Build`, `CMake: Run Tests`) via the command palette.

## Installing This Library From GitHub (advanced, you want to contribute)
Create a directory in your Arduino\Library folder named "LumaWave"
Clone (Git) this project into that folder.  
It should now show up in the import list when you restart Arduino IDE.

## Nil Transport (dry-run / host-safe)

`NilTransport` is available from `#include <LumaWave.h>` for no-op transport flows (for example dry-run rendering or protocol-only verification).

```cpp
#include <LumaWave.h>

using Protocol = lw::protocols::DotStar<>;
lw::busses::PixelBus<Protocol, lw::transports::NilTransport> bus(
    32,
    lw::transports::NilTransportSettings{});
bus.begin();
bus.show();
```





