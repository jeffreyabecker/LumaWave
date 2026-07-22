#include <Arduino.h>
#include <LumaWave.h>

/*
Target: ESP8266 only.
Requires: `ARDUINO_ARCH_ESP8266`.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Demonstrates `lw::transports::esp8266::Esp8266DmaI2sTransport`.
*/

#if !defined(ARDUINO_ARCH_ESP8266)
#error "This example requires ARDUINO_ARCH_ESP8266."
#endif

namespace
{
using Protocol = lw::protocols::Ws2812x<>;
using Transport = lw::transports::esp8266::Esp8266DmaI2sTransport;
using StripType = lw::busses::PixelBus<Protocol, Transport>;

constexpr lw::PixelCount LedCount = 24;

StripType strip(LedCount, StripType::TransportSettingsType{});
uint16_t frame = 0;
} // namespace

void setup()
{
    strip.begin();
}

void loop()
{
    while (true)
    {
        auto& pixels = strip.pixels();
        for (size_t i = 0; i < pixels.size(); ++i)
        {
            pixels[i] = Protocol::PixelType(static_cast<uint8_t>((i + frame) & 0x3F),
                                            static_cast<uint8_t>((2U * i + frame) & 0x3F),
                                            static_cast<uint8_t>((3U * i + frame) & 0x3F));
        }

        strip.show();
        ++frame;
        delay(20);
    }
