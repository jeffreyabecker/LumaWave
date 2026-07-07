#pragma once

#ifdef ARDUINO_ARCH_ESP8266

#include "transports/AnalogPwmLightDriver.h"

namespace lw::transports::esp8266
{

using Esp8266LedcLightDriverSettings = lw::transports::AnalogPwmLightDriverSettings;

 using Esp8266LedcLightDriver = lw::transports::AnalogPwmLightDriver<lw::Color>;

} // namespace lw::transports::esp8266

#endif // ARDUINO_ARCH_ESP8266
