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

lw::Pixel pixel;

// Caller-owned light driver
lw::transports::PlatformDefaultLightDriver lightDriver(lw::transports::PlatformDefaultLightDriver::LightDriverSettingsType{.pins = {redPin, greenPin, bluePin, warmPin, coolPin}, .invert = false});

// Caller-owned pipeline run
lw::busses::PipelineRun runs[] = {{&lightDriver, 1}};

// Bus: non-owning view over caller-managed resources
lw::busses::Bus light(lw::span<lw::Pixel>{&pixel, 1}, lw::span<const lw::busses::PipelineRun>{runs});
uint16_t phase = 0;
lw::Pixel rampColor()
{
  const uint16_t ramp = static_cast<uint16_t>((phase & 0xFF) * 257U);
  return lw::Pixel(ramp, static_cast<uint16_t>(65535U - ramp), static_cast<uint16_t>(ramp / 2U), static_cast<uint16_t>((phase * 113U) & 0xFFFF), static_cast<uint16_t>((phase * 197U) & 0xFFFF));
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
