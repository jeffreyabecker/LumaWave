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
#include "hardware/uart.h"

#include "transports/ITransport.h"
#include "RpDmaManager.h"

namespace lw::transports::rp2040
{

static constexpr uint32_t UartClockDefaultHz = LW_SPI_CLOCK_DEFAULT_HZ;
static constexpr uint8_t UartDataBits = 8;
static constexpr uint8_t UartStopBits = 1;
static constexpr uart_parity_t UartParity = UART_PARITY_NONE;

struct RpUartTransportSettings : TransportSettingsBase
{
  uint8_t spiIndex = 0;

  static RpUartTransportSettings normalize(RpUartTransportSettings settings)
  {
    if (settings.clockRateHz == 0)
    {
      settings.clockRateHz = UartClockDefaultHz;
    }

    return settings;
  }
};

class RpUartTransport : public ITransport
{
public:
  using TransportSettingsType = RpUartTransportSettings;

  explicit RpUartTransport(RpUartTransportSettings config) : _config{config}, _holdoffUs{RpDmaManager::computeFifoCacheEmptyDeltaUs(computeBitPeriodNs(config.clockRateHz))} {}

  ~RpUartTransport()
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

    if (_uart != nullptr)
    {
      uart_deinit(_uart);
    }

    if (_config.dataPin >= 0)
    {
      pinMode(_config.dataPin, INPUT);
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

    _uart = resolveUart();
    if (_uart == nullptr)
    {
      return;
    }

    uart_init(_uart, _config.clockRateHz);
    uart_set_format(_uart, UartDataBits, UartStopBits, UartParity);
    uart_set_hw_flow(_uart, false, false);
    uart_set_fifo_enabled(_uart, true);

    gpio_set_function(static_cast<uint>(_config.dataPin), GPIO_FUNC_UART);

    if (_config.invert)
    {
      gpio_set_outover(static_cast<uint>(_config.dataPin), GPIO_OVERRIDE_INVERT);
    }

    _dmaLease = _dmaManager.requestChannel();
    if (!_dmaLease.isValid())
    {
      uart_deinit(_uart);
      _uart = nullptr;
      pinMode(_config.dataPin, INPUT);
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

    if (!_initialised || data.empty() || _uart == nullptr)
    {
      return;
    }
    if (_config.bitOrder == BitOrderLsbFirst)
    {
      // If LSB first, we need to reverse the bits in each byte before sending
      for (size_t i = 0; i < data.size(); ++i)
      {
        uint8_t byte = data[i];
        byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
        byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
        byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
        data[i] = byte;
      }
    }

    _dmaLease.startTransfer(data, static_cast<volatile void*>(&(uart_get_hw(_uart)->dr)), uart_get_dreq(_uart, true), false, true, false);
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
  RpUartTransportSettings _config;
  RpDmaManager _dmaManager;
  RpDmaManager::ChannelLease _dmaLease;
  uart_inst_t* _uart{nullptr};
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

  uart_inst_t* resolveUart() const
  {
    if (_config.spiIndex > 1)
    {
      return nullptr;
    }

    return (_config.spiIndex == 0) ? uart0 : uart1;
  }
};

} // namespace lw::transports::rp2040

#endif // ARDUINO_ARCH_RP2040
