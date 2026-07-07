#include <Arduino.h>
#include <LumaWave.h>

/*
Target: Arduino platforms with platform-default light driver support.
Requires: Platform default light driver pin assignment.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Uses `lw::busses::LightBus<lw::Rgbw16Color, Driver>` with default-constructed driver settings.
*/

constexpr int redPin = 2;
constexpr int greenPin = 3;
constexpr int bluePin = 4;
constexpr int warmPin = 5;
constexpr int coolPin = 6;

Light<Rgbw16Color, Driver::PlatformDefault<Rgbw16Color>> light({.pins = {redPin, greenPin, bluePin, warmPin, coolPin},
                                                                  .invert = false});
uint16_t phase = 0;
Rgbw16Color rampColor()
{
    const uint16_t ramp = static_cast<uint16_t>((phase & 0xFF) * 257U);
    return Rgbw16Color(ramp, static_cast<uint16_t>(65535U - ramp), static_cast<uint16_t>(ramp / 2U),
                        static_cast<uint16_t>((phase * 113U) & 0xFFFF), static_cast<uint16_t>((phase * 197U) & 0xFFFF));
}
void setup()
{
    light.begin();
}

void loop()
{
    while (true)
    {
        auto& pixel = light.pixel();

        pixel = rampColor();

        light.show();
        ++phase;
        delay(12);
    }
