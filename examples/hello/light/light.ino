#include <Arduino.h>
#include <LumaWave.h>

/*
Target: Arduino platforms with platform-default light driver support.
Requires: Platform default light driver pin assignment.
API: Direct Bus construction with single PipelineRun.
*/

constexpr int redPin = 2;
constexpr int greenPin = 3;
constexpr int bluePin = 4;
constexpr int warmPin = 5;
constexpr int coolPin = 6;

lw::Color pixel;
lw::busses::Bus light(lw::span<lw::Color>{&pixel, 1}, {
    {std::make_unique<lw::transports::PlatformDefaultLightDriver>(
         lw::transports::PlatformDefaultLightDriver::LightDriverSettingsType{
             .pins = {redPin, greenPin, bluePin, warmPin, coolPin},
             .invert = false}), 1}
});
uint16_t phase = 0;
lw::Color rampColor()
{
    const uint16_t ramp = static_cast<uint16_t>((phase & 0xFF) * 257U);
    return lw::Color(ramp, static_cast<uint16_t>(65535U - ramp), static_cast<uint16_t>(ramp / 2U),
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
        light.pixels()[0] = rampColor();
        light.show();
        ++phase;
        delay(12);
    }
