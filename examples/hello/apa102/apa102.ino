#include <Arduino.h>
#include <LumaWave.h>

/*
Target: Arduino platforms with SPI support.
Requires: `LW_HAS_SPI_TRANSPORT` and valid SPI pins.
API: Direct Bus construction with ProtocolTransportPipeline.
*/

constexpr pixel_count_t ledCount = 60;
constexpr int clockPin = 18;
constexpr int dataPin = 23;

lw::Color pixels[60];
lw::busses::Bus strip(lw::span<lw::Color>{pixels}, {
    {std::make_unique<lw::busses::ProtocolTransportPipeline>(
         std::make_unique<Protocols::APA102::ProtocolType>(ledCount, Protocols::APA102::defaultSettings()),
         std::make_unique<lw::transports::SpiTransport>(
             lw::transports::SpiTransportSettings{{false, 8000000UL, lw::transports::BitOrderMsbFirst,
                                                    lw::transports::SpiMode0, clockPin, dataPin}})),
     ledCount}
});
uint16_t frame = 0;

void setup()
{
  strip.begin();
}

void loop()
{
  auto& pixels = strip.pixels();
  const size_t count = pixels.size();

  while (true)
  {
    for (size_t i = 0; i < count; ++i)
    {
      pixels[i] = lw::Color(static_cast<uint8_t>((i + frame) & 0x3F), static_cast<uint8_t>((2U * i + frame) & 0x3F), static_cast<uint8_t>((3 * i + frame) & 0x3F));
    }

    strip.show();
    ++frame;
    delay(20);
  }
