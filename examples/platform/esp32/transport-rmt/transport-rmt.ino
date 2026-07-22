#include <Arduino.h>
#include <LumaWave.h>

/*
Target: ESP32 (except C6/H2 where this transport is not available).
Requires: `ARDUINO_ARCH_ESP32` and one-wire data pin.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Demonstrates `lw::transports::esp32::Esp32RmtTransport`.
*/

#if !defined(ARDUINO_ARCH_ESP32)
#error "This example requires ARDUINO_ARCH_ESP32."
#endif

namespace
{
using Protocol = lw::protocols::Ws2812x<>;
using Transport = lw::transports::esp32::Esp32RmtTransport;
using StripType = lw::busses::PixelBus<Protocol, Transport>;

constexpr lw::PixelCount LedCount = 32;
constexpr int DataPin = 2;

StripType::TransportSettingsType makeTransportSettings()
{
    StripType::TransportSettingsType settings{};
    settings.dataPin = DataPin;
    settings.invert = false;
    return settings;
}

StripType strip(LedCount, makeTransportSettings());
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
