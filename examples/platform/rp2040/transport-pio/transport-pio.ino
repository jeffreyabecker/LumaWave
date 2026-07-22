#include <Arduino.h>
#include <LumaWave.h>

/*
Target: RP2040 only.
Requires: `ARDUINO_ARCH_RP2040` and valid one-wire data pin.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Demonstrates `lw::transports::rp2040::RpPioTransport` as explicit bus transport.
*/

#if !defined(ARDUINO_ARCH_RP2040)
#error "This example requires ARDUINO_ARCH_RP2040."
#endif

namespace
{
using Protocol = lw::protocols::Ws2812x<>;
using Transport = lw::transports::rp2040::RpPioTransport;
using StripType = lw::busses::PixelBus<Protocol, Transport>;

constexpr lw::PixelCount LedCount = 24;
constexpr int DataPin = 2;

StripType::TransportSettingsType makeTransportSettings()
{
    StripType::TransportSettingsType settings{};
    settings.dataPin = DataPin;
    settings.pioIndex = 1;
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
