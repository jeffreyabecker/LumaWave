#include <Arduino.h>
#include <LumaWave.h>

// ---------------------------------------------------------------------------
// Hello World with BusBuilder
//
// Demonstrates the simplest way to construct a pixel strip using the builder
// API. Replaces the manual construction of protocol, transport, buffers,
// shaders, pipeline, and bus with a single chained builder expression.
//
// Compare with examples/hello/ws2812/ws2812.ino for the manual equivalent.
// ---------------------------------------------------------------------------

constexpr size_t kLedCount = 30;

void setup()
{
  // One-shot builder: all resources owned by the returned IPixelBus.
  auto strip = lw::buses::BusBuilder()
                   .setPixelCount(kLedCount)
                   .setTransport(lw::transports::NilTransport{})
                   .setProtocol(lw::protocols::Ws2812xProtocol(kLedCount, lw::protocols::Ws2812xProtocolSettings{lw::ChannelOrder::GRB::value, lw::protocols::timing::Ws2812x}))
                   .build();

  strip->begin();

  // Animation loop
  uint16_t frame = 0;
  while (true)
  {
    auto& pixels = strip->pixels();
    for (size_t i = 0; i < kLedCount; ++i)
    {
      pixels[i] = lw::pixelFromRGB(static_cast<uint8_t>((i + frame) & 0x3F), static_cast<uint8_t>((2U * i + frame) & 0x3F), static_cast<uint8_t>((3U * i + frame) & 0x3F));
    }
    strip->show();
    ++frame;
    delay(20);
  }
}

void loop()
{
  // setup() never returns
}
