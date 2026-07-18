#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "core/Compat.h"

#include "Transport.h"

#if defined(LW_HAS_SPI_TRANSPORT)
#if LW_HAS_ARDUINO
#include <Arduino.h>
#endif
#include <SPI.h>
#endif

namespace lw::transports
{

#if defined(LW_HAS_SPI_TRANSPORT)

static constexpr uint32_t SpiClockDefaultHz = LW_SPI_CLOCK_DEFAULT_HZ;

struct SpiTransportSettings : TransportSettingsBase
{
  SPIClass* spi = nullptr;

  static SpiTransportSettings normalize(SpiTransportSettings settings)
  {
    if (settings.clockRateHz == 0)
    {
      settings.clockRateHz = SpiClockDefaultHz;
    }

    return settings;
  }
};

class SpiTransport : public Transport
{
public:
  using TransportSettingsType = SpiTransportSettings;

  explicit SpiTransport(SpiTransportSettings config) : _config{config} {}

  void begin() override
  {
    if (_config.spi == nullptr)
    {
      return;
    }

    _config.spi->begin();
#if LW_HAS_ARDUINO
    if (_config.clockPin >= 0 && _config.dataPin >= 0)
    {
      pinMode(_config.clockPin, OUTPUT);
      pinMode(_config.dataPin, OUTPUT);
    }
#endif
  }

  void beginTransaction() override
  {
    if (_config.spi == nullptr)
    {
      return;
    }

    _config.spi->beginTransaction(SPISettings(_config.clockRateHz, resolveBitOrder(_config.bitOrder), resolveDataMode(_config.dataMode)));
  }

  void transmitBytes(span<uint8_t> data) override
  {
    if (_config.spi == nullptr || data.empty())
    {
      return;
    }

    if (!_config.invert)
    {
      for (size_t index = 0; index < data.size(); ++index)
      {
        _config.spi->transfer(data[index]);
      }
      return;
    }

    for (size_t index = 0; index < data.size(); ++index)
    {
      _config.spi->transfer(static_cast<uint8_t>(~data[index]));
    }
  }

  void endTransaction() override
  {
    if (_config.spi == nullptr)
    {
      return;
    }

    _config.spi->endTransaction();
  }

private:
  SpiTransportSettings _config;

  static auto resolveBitOrder(uint8_t bitOrder) -> decltype(LSBFIRST) { return (bitOrder == BitOrderLsbFirst) ? LSBFIRST : MSBFIRST; }

  static auto resolveDataMode(uint8_t dataMode) -> decltype(SPI_MODE0)
  {
    switch (dataMode)
    {
      case SpiMode1:
        return SPI_MODE1;
      case SpiMode2:
        return SPI_MODE2;
      case SpiMode3:
        return SPI_MODE3;
      case SpiMode0:
      default:
        return SPI_MODE0;
    }
  }
};

#endif

} // namespace lw::transports
