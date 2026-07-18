#include <LumaWave.h>

// Compile-time instantiation: Ws2812x protocol + NilTransport + no shaders
static lw::buses::PixelBus<lw::protocols::Ws2812xProtocol, lw::transports::NilTransport> bus_no_shaders(30);

// Compile-time instantiation: Ws2812x protocol + NilTransport + BrightnessShader
static lw::buses::PixelBus<lw::protocols::Ws2812xProtocol, lw::transports::NilTransport, lw::protocols::BrightnessShader> bus_with_shaders(30);

// Shader-arg constructor: GammaShader with custom gamma via std::tuple
static lw::buses::PixelBus<lw::protocols::Ws2812xProtocol, lw::transports::NilTransport, lw::protocols::GammaShader> bus_with_gamma(30, {}, {}, std::tuple{2.2f});

// Verify IPixelBus interface is accessible
static_assert(std::is_base_of_v<lw::IPixelBus, decltype(bus_no_shaders)>);
static_assert(std::is_base_of_v<lw::IPixelBus, decltype(bus_with_shaders)>);
static_assert(std::is_base_of_v<lw::IPixelBus, decltype(bus_with_gamma)>);

int main()
{
  // Verify begin/show don't crash
  bus_no_shaders.begin();
  bus_no_shaders.show();

  bus_with_shaders.begin();
  bus_with_shaders.show();

  bus_with_gamma.begin();
  bus_with_gamma.show();

  return 0;
}
