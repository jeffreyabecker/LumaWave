#pragma once

#ifdef ARDUINO_ARCH_RP2040

#include <cstdint>
#include <cstddef>

#include <Arduino.h>
#if __has_include(<SPI.h>)
#include <SPI.h>
#endif

#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "transports/Transport.h"
#include "RpDmaManager.h"

namespace lw::transports::rp2040
{

static constexpr uint32_t SpiClockDefaultHz = LW_SPI_CLOCK_DEFAULT_HZ;

struct RpSpiTransportSettings : TransportSettingsBase
{
  uint8_t spiIndex = 0;

  static RpSpiTransportSettings normalize(RpSpiTransportSettings settings)
  {
    if (settings.clockRateHz == 0)
    {
      settings.clockRateHz = SpiClockDefaultHz;
    }

    return settings;
  }
};

class RpSpiTransport : public Transport
{
public:
  using TransportSettingsType = RpSpiTransportSettings;

  explicit RpSpiTransport(RpSpiTransportSettings config) : _config{config}, _holdoffUs{RpDmaManager::computeFifoCacheEmptyDeltaUs(computeBitPeriodNs(config.clockRateHz))} {}

  ~RpSpiTransport()
  {
    if (!_initialised)
    {
      return;
    }

    while (!isReadyToUpdate())
    {
      yield();
    }

    _dmaLease.release();

    if (_config.dataPin >= 0)
    {
      pinMode(_config.dataPin, INPUT);
    }
    if (_config.clockPin >= 0)
    {
      pinMode(_config.clockPin, INPUT);
    }
  }

  void begin() override
  {
    if (_initialised)
    {
      return;
    }

    if (_config.dataPin < 0 || _config.clockRateHz == 0)
    {
      return;
    }

    _spi = resolveSpi();
    if (_spi == nullptr)
    {
      return;
    }

    spi_init(_spi, _config.clockRateHz);
    spi_set_format(_spi, 8, (_config.dataMode == SpiMode2 || _config.dataMode == SpiMode3) ? SPI_CPOL_1 : SPI_CPOL_0, (_config.dataMode == SpiMode1 || _config.dataMode == SpiMode3) ? SPI_CPHA_1 : SPI_CPHA_0,
                   (_config.bitOrder == BitOrderLsbFirst) ? SPI_LSB_FIRST : SPI_MSB_FIRST);

    if (_config.dataPin >= 0)
    {
      gpio_set_function(static_cast<uint>(_config.dataPin), GPIO_FUNC_SPI);
    }
    if (_config.clockPin >= 0)
    {
      gpio_set_function(static_cast<uint>(_config.clockPin), GPIO_FUNC_SPI);
    }

    if (_config.invert)
    {
      if (_config.dataPin >= 0)
      {
        gpio_set_outover(static_cast<uint>(_config.dataPin), GPIO_OVERRIDE_INVERT);
      }
      if (_config.clockPin >= 0)
      {
        gpio_set_outover(static_cast<uint>(_config.clockPin), GPIO_OVERRIDE_INVERT);
      }
    }

    _dmaLease = _dmaManager.requestChannel();
    if (!_dmaLease.isValid())
    {
      if (_config.dataPin >= 0)
      {
        pinMode(_config.dataPin, INPUT);
      }
      if (_config.clockPin >= 0)
      {
        pinMode(_config.clockPin, INPUT);
      }
      return;
    }

    _initialised = true;
  }

  void transmitBytes(span<uint8_t> data) override
  {
    if (!_initialised)
    {
      begin();
    }

    if (!_initialised || data.empty() || _spi == nullptr)
    {
      return;
    }

    _dmaLease.startTransfer(data, static_cast<volatile void*>(&(spi_get_hw(_spi)->dr)), spi_get_dreq(_spi, true), false, true, false);
  }

  bool isReadyToUpdate() const override
  {
    if (!_initialised)
    {
      return true;
    }

    if (_dmaManager.isSending())
    {
      return false;
    }

    if (_dmaManager.hasDmaCompleted())
    {
      if (_spi != nullptr && spi_is_busy(_spi))
      {
        return false;
      }

      if (_dmaManager.elapsedSinceDmaCompleteUs() < _holdoffUs)
      {
        return false;
      }

      const_cast<RpDmaManager&>(_dmaManager).setIdle();
      return true;
    }

    return true;
  }

private:
  RpSpiTransportSettings _config;
  RpDmaManager _dmaManager;
  RpDmaManager::ChannelLease _dmaLease;
  spi_inst_t* _spi{nullptr};
  const uint32_t _holdoffUs;
  bool _initialised{false};

  static uint32_t computeBitPeriodNs(uint32_t bitRateHz)
  {
    if (bitRateHz == 0)
    {
      return 0;
    }

    return static_cast<uint32_t>((1000000000ULL + static_cast<uint64_t>(bitRateHz) - 1ULL) / static_cast<uint64_t>(bitRateHz));
  }

  spi_inst_t* resolveSpi() const
  {
    if (_config.spiIndex > 1)
    {
      return nullptr;
    }

    return (_config.spiIndex == 0) ? spi0 : spi1;
  }
};

} // namespace lw::transports::rp2040

#endif // ARDUINO_ARCH_RP2040
