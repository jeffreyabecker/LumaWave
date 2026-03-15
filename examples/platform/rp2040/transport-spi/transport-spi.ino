#include <Arduino.h>
#include <LumaWave.h>

/*
Target: RP2040 only.
Requires: `ARDUINO_ARCH_RP2040` and SPI-capable clock/data pins.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Demonstrates `lw::transports::rp2040::RpSpiTransport` with DotStar protocol.
*/

#if !defined(ARDUINO_ARCH_RP2040)
#error "This example requires ARDUINO_ARCH_RP2040."
#endif

namespace
{
using Protocol = lw::protocols::DotStar<>;
using Transport = lw::transports::rp2040::RpSpiTransport;
using StripType = lw::busses::PixelBus<Protocol, Transport>;

constexpr lw::PixelCount LedCount = 30;
constexpr int ClockPin = 2;
constexpr int DataPin = 3;

StripType::TransportSettingsType makeTransportSettings()
{
    StripType::TransportSettingsType settings{};
    settings.spiIndex = 0;
    settings.clockPin = ClockPin;
    settings.dataPin = DataPin;
    settings.clockRateHz = 8000000UL;
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
            pixels[i] = Protocol::ColorType(static_cast<uint8_t>((i + frame) & 0x3F),
                                            static_cast<uint8_t>((2U * i + frame) & 0x3F),
                                            static_cast<uint8_t>((3U * i + frame) & 0x3F));
        }

        strip.show();
        ++frame;
        delay(20);
    }
