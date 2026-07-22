#pragma once

#ifdef ARDUINO_ARCH_ESP8266

#include <cstdint>
#include <cstddef>

#include <Arduino.h>

extern "C"
{
#include "eagle_soc.h"
#include "esp8266_peri.h"
}

#include "transports/Transport.h"

namespace lw::transports::esp8266
{

struct Esp8266DmaUartTransportSettings : TransportSettingsBase
{
  uint8_t uartNumber = 1;
  uint32_t baudRate = 3200000UL;

  static Esp8266DmaUartTransportSettings normalize(Esp8266DmaUartTransportSettings settings)
  {
    if (settings.baudRate == 0)
    {
      settings.baudRate = 3200000UL;
    }

    return settings;
  }
};

class Esp8266DmaUartTransport : public Transport
{
public:
  using TransportSettingsType = Esp8266DmaUartTransportSettings;
  static constexpr size_t UartFifoSize = 128;
  static constexpr uint8_t Uart0Pin = 1;
  static constexpr uint8_t Uart1Pin = 2;

  explicit Esp8266DmaUartTransport(Esp8266DmaUartTransportSettings config) : _config{config} { _byteSendTimeUs = computeByteSendTimeUs(); }

  ~Esp8266DmaUartTransport() override
  {
    if (!_initialised)
    {
      return;
    }

    uint8_t n = _config.uartNumber;
    while ((USS(n) >> USTXC) & 0xFF)
    {
      yield();
    }
  }

  void begin() override
  {
    if (_initialised)
    {
      return;
    }

    configureUart();
    _startTime = micros();
    _initialised = true;
  }

  void transmitBytes(span<uint8_t> data) override
  {
    if (!_initialised)
    {
      begin();
    }

    _lastPayloadSize = data.size();
    _startTime = micros();

    uint8_t n = _config.uartNumber;
    for (size_t i = 0; i < data.size(); ++i)
    {
      while (((USS(n) >> USTXC) & 0xFF) > (UartFifoSize - 1))
      {
        yield();
      }

      USF(n) = data[i];
    }
  }

  bool isReadyToUpdate() const override
  {
    if (!_initialised)
    {
      return true;
    }

    uint32_t elapsed = micros() - _startTime;
    uint32_t payloadTimeUs = static_cast<uint32_t>(_lastPayloadSize) * _byteSendTimeUs;
    return elapsed >= payloadTimeUs;
  }

private:
  Esp8266DmaUartTransportSettings _config;
  uint32_t _startTime{0};
  uint32_t _byteSendTimeUs{0};
  size_t _lastPayloadSize{0};
  bool _initialised{false};

  uint32_t effectiveBaud() const { return (_config.baudRate == 0U) ? 3200000UL : _config.baudRate; }

  uint32_t computeByteSendTimeUs() const
  {
    uint32_t baud = effectiveBaud();
    if (baud == 0U)
    {
      return 10U;
    }

    return static_cast<uint32_t>((10UL * 1000000UL + (baud - 1UL)) / baud);
  }

  void configureUart()
  {
    uint8_t n = _config.uartNumber;
    uint8_t pin = (n == 0) ? Uart0Pin : Uart1Pin;

    if (n == 0)
    {
      Serial.end();
      pinMode(pin, SPECIAL);
    }
    else
    {
      Serial1.end();
      pinMode(pin, SPECIAL);
    }

    uint32_t baud = effectiveBaud();
    uint32_t uartClkDiv = (ESP8266_CLOCK / baud) & 0xFFFFF;
    USD(n) = uartClkDiv;
    USC0(n) = 0;

    USC0(n) &= ~(BIT(UCDTRI) | BIT(UCRTSI) | BIT(UCTXI) | BIT(UCDSRI) | BIT(UCCTSI) | BIT(UCRXI));

    if (!_config.invert)
    {
      USC0(n) |= BIT(UCTXI);
    }

    uint32_t tmp = USC0(n);
    tmp |= BIT(UCTXRST);
    USC0(n) = tmp;
    tmp &= ~BIT(UCTXRST);
    USC0(n) = tmp;
  }
};

} // namespace lw::transports::esp8266

#endif // ARDUINO_ARCH_ESP8266
