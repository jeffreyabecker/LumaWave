#include <LumaWave.h>

// Compile-time instantiation check: Ws2812x protocol + NilTransport + no shaders
static lw::buses::StackPixelBus<30, lw::protocols::Ws2812xProtocol, lw::transports::NilTransport> bus_no_shaders;

// Compile-time instantiation check: Ws2812x protocol + NilTransport + BrightnessShader
static lw::buses::StackPixelBus<30, lw::protocols::Ws2812xProtocol, lw::transports::NilTransport, lw::protocols::BrightnessShader> bus_with_shaders;

// Verify IPixelBus interface is accessible
static_assert(std::is_base_of_v<lw::IPixelBus, decltype(bus_no_shaders)>);
static_assert(std::is_base_of_v<lw::IPixelBus, decltype(bus_with_shaders)>);

int main()
{
  // Verify begin/show don't crash on default-constructed bus
  bus_no_shaders.begin();
  bus_no_shaders.show();

  bus_with_shaders.begin();
  bus_with_shaders.show();

  return 0;
}
