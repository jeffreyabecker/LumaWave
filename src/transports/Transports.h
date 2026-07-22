#pragma once

#include "transports/Transport.h"
#include "transports/NilTransport.h"
#include "transports/NilTransportPreset.h"
#include "transports/TransportPresets.h"
#include "core/OutputPipeline.h"

#if defined(ARDUINO_ARCH_NRF52840)
#include "transports/nrf52/Nrf52PwmOneWireTransport.h"
#endif

namespace lw::transports
{

#if defined(ARDUINO_ARCH_RP2040)
using PlatformDefaultLightDriver = lw::transports::rp2040::RpPwmLightDriver;
#elif defined(ARDUINO_ARCH_ESP32)
using PlatformDefaultLightDriver = lw::transports::esp32::Esp32SigmaDeltaLightDriver;
#elif defined(ARDUINO_ARCH_ESP8266)
using PlatformDefaultLightDriver = lw::transports::esp8266::Esp8266LedcLightDriver;
#else
using PlatformDefaultLightDriver = lw::OutputPipeline;
#endif

} // namespace lw::transports
