#include <Arduino.h>
#include <LumaWave.h>

/*
Target: Arduino platforms with SPI support.
Requires: `LW_HAS_SPI_TRANSPORT` and valid SPI pins.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Uses `lw::transports::SpiTransport` settings with `SPI` instance.
*/

constexpr pixel_count_t ledCount = 60;
constexpr int clockPin = 18;
constexpr int dataPin = 23;
Strip<Protocols::APA102> strip(ledCount, Transport::DefaultSettings{{false, 8000000UL, lw::transports::BitOrderMsbFirst, lw::transports::SpiMode0, clockPin, dataPin}});
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
