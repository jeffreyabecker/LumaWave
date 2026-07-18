#pragma once

#ifdef ARDUINO_ARCH_ESP8266

#include <Arduino.h>

#include "transports/AnalogPwmLightDriver.h"

namespace lw::transports::esp8266
{

using Esp8266LedcLightDriverSettings = lw::transports::AnalogPwmLightDriverSettings;

class Esp8266LedcLightDriver : public lw::transports::AnalogPwmLightDriver
{
public:
  using AnalogPwmLightDriver::AnalogPwmLightDriver;

protected:
  void platformAnalogWrite(int pin, uint16_t value) override { analogWrite(pin, value); }

  void platformPinMode(int pin, PinMode mode) override { pinMode(pin, (mode == PinMode::Output) ? OUTPUT : INPUT); }

  void platformAnalogWriteFreq(uint32_t freq) override { analogWriteFreq(freq); }

  void platformAnalogWriteRange(uint16_t range) override { analogWriteRange(range); }
};

} // namespace lw::transports::esp8266

#endif // ARDUINO_ARCH_ESP8266
