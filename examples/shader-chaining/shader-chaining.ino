#include <Arduino.h>
#include <LumaWave.h>

// ---------------------------------------------------------------------------
// Shader Chaining
//
// Demonstrates adding multiple shaders via the builder. Shaders apply in
// insertion order: each shader's output feeds the next shader's input.
//
// The scratch pixel buffer is automatically allocated by the builder when
// shaders are present. Call enableDestructiveShaders() to skip scratch
// and mutate the pixel buffer in place instead (caller must repaint every
// frame).
// ---------------------------------------------------------------------------

constexpr size_t kLedCount = 30;

void setup()
{
  auto strip = lw::buses::BusBuilder()
                   .setPixelCount(kLedCount)
                   .setTransport(lw::transports::NilTransport{})
                   .setProtocol(lw::protocols::Ws2812xProtocol(kLedCount, lw::protocols::Ws2812xProtocolSettings{lw::ChannelOrder::GRB::value}))
                   .addShader(lw::shaders::BrightnessShader{128})
                   .addShader(lw::shaders::GammaShader{2.2f})
                   .build();

  strip->begin();

  // Shaders apply in order: BrightnessShader then GammaShader.
  strip->pixels()[0] = lw::pixelFromRGB(255, 128, 64);
  strip->show();
}

void loop()
{
}
