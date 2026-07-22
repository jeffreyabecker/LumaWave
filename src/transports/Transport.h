#pragma once

#include <cstdint>
#include <cstddef>

#include "core/Compat.h"
#include "core/RuntimeConfig.h"

namespace lw::transports
{

static constexpr uint8_t BitOrderLsbFirst = 0;
static constexpr uint8_t BitOrderMsbFirst = 1;

static constexpr uint8_t SpiMode0 = 0x00;
static constexpr uint8_t SpiMode1 = 0x04;
static constexpr uint8_t SpiMode2 = 0x08;
static constexpr uint8_t SpiMode3 = 0x0C;

struct TransportSettingsBase
{
  bool invert = false;
  uint32_t clockRateHz = LW_SPI_CLOCK_DEFAULT_HZ;
  uint8_t bitOrder = BitOrderMsbFirst;
  uint8_t dataMode = SpiMode0;
  int clockPin = -1;
  int dataPin = -1;
};

class Transport
{
public:
  using TransportSettingsType = TransportSettingsBase;

  virtual ~Transport() = default;

  virtual void begin() {}

  virtual void beginTransaction() {}

  virtual void transmitBytes(span<uint8_t> data) {}

  virtual void endTransaction() {}

  virtual bool isReadyToUpdate() const { return true; }

  virtual void setRuntimeConfig(lw::RuntimeConfig type, void* value)
  {
    (void)type;
    (void)value;
  }
};

} // namespace lw::transports
