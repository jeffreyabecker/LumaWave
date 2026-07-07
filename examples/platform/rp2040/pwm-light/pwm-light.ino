#include <Arduino.h>
#include <LumaWave.h>

/*
Target: RP2040 only.
Requires: `ARDUINO_ARCH_RP2040` and PWM-capable GPIO pins.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Uses direct `LightBus<Color, RpPwmLightDriver>` construction.
*/

#if !defined(ARDUINO_ARCH_RP2040)
#error "This example requires an RP2040 target."
#endif

namespace
{
using ColorType = lw::Color;
using DriverType = lw::transports::rp2040::RpPwmLightDriver<ColorType>;
using LightType = lw::busses::LightBus<ColorType, DriverType>;

constexpr int RedPin = 2;
constexpr int GreenPin = 3;
constexpr int BluePin = 4;

lw::transports::rp2040::RpPwmLightDriverSettings makeDriverSettings()
{
    lw::transports::rp2040::RpPwmLightDriverSettings settings{};
    settings.pins = {RedPin, GreenPin, BluePin};
    settings.wrap = 255;
    settings.clockDiv = 4.0f;
    settings.invert = false;
    return settings;
}

LightType light(makeDriverSettings());

uint8_t intensity = 0;
bool goingUp = true;
} // namespace

void setup()
{
    light.begin();
}

void loop()
{
    while (true)
    {
        auto& pixel = light.pixel();
        pixel['R'] = intensity;
        pixel['G'] = static_cast<uint8_t>(255U - intensity);
        pixel['B'] = intensity / 2U;

        light.show();

        if (goingUp)
        {
            if (intensity == 255U)
            {
                goingUp = false;
            }
            else
            {
                ++intensity;
            }
        }
        else
        {
            if (intensity == 0U)
            {
                goingUp = true;
            }
            else
            {
                --intensity;
            }
        }

        delay(8);
    }
