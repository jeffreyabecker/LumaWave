#pragma once

#include "transports/Transport.h"
#include "core/OutputPipeline.h"
#include "transports/PwmOutputPipeline.h"
#include "transports/PrintOutputPipeline.h"
#include "transports/PrintTransport.h"
#include "transports/SpiTransport.h"

#ifdef ARDUINO_ARCH_RP2040
#include "transports/rp2040/RpPwmLightDriver.h"
#include "transports/rp2040/RpPioTransport.h"
#include "transports/rp2040/RpSpiTransport.h"
#include "transports/rp2040/RpUartTransport.h"
#endif

#ifdef ARDUINO_ARCH_ESP32
#include "transports/esp32/Esp32LedcLightDriver.h"
#include "transports/esp32/Esp32DmaSpiTransport.h"
#include "transports/esp32/Esp32I2sTransport.h"
#include "transports/esp32/Esp32RmtTransport.h"
#include "transports/esp32/Esp32SigmaDeltaLightDriver.h"
#endif

#ifdef ARDUINO_ARCH_ESP8266
#include "transports/esp8266/Esp8266DmaI2sTransport.h"
#include "transports/esp8266/Esp8266DmaUartTransport.h"
#include "transports/esp8266/Esp8266LedcLightDriver.h"
#endif

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
using PlatformDefaultLightDriver = lw::buses::OutputPipeline;
#endif

} // namespace lw::transports
