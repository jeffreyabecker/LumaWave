#pragma once

#ifdef ARDUINO_ARCH_ESP8266

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <Arduino.h>

extern "C"
{
#include "eagle_soc.h"
#include "i2s_reg.h"
#include "slc_register.h"
#include "esp8266_peri.h"
}

#include "transports/ITransport.h"

namespace lw::transports::esp8266
{

struct Esp8266DmaI2sTransportSettings : TransportSettingsBase
{
};

class Esp8266DmaI2sTransport : public ITransport
{
public:
  using TransportSettingsType = Esp8266DmaI2sTransportSettings;
  static constexpr uint8_t I2sPin = 3;
  static constexpr size_t MaxDmaBlockSize = 4092;

  explicit Esp8266DmaI2sTransport(Esp8266DmaI2sTransportSettings config) : _config{config} {}

  ~Esp8266DmaI2sTransport() override
  {
    if (_initialised)
    {
      stopI2s();
    }
    freeI2sBuffers();
  }

  void begin() override {}

  void beginTransaction() override {}

  void transmitBytes(span<uint8_t> data) override
  {
    if (data.empty())
    {
      return;
    }

    ensureInitialised(data.size());
    if (_descriptors == nullptr)
    {
      return;
    }

    if (_config.invert)
    {
      for (size_t i = 0; i < data.size(); ++i)
      {
        data[i] = static_cast<uint8_t>(~data[i]);
      }
    }
    if (_config.bitOrder == BitOrderLsbFirst)
    {
      for (size_t i = 0; i < data.size(); ++i)
      {
        data[i] = reverseBits8(data[i]);
      }
    }

    bindDataDescriptors(data.data(), data.size());

    writeI2s();
  }

  void endTransaction() override {}

  bool isReadyToUpdate() const override { return _dmaState == DmaState::Idle; }

private:
  enum class DmaState : uint8_t
  {
    Idle,
    Sending
  };

  struct SlcDescriptor
  {
    uint32_t blocksize : 12;
    uint32_t datalen : 12;
    uint32_t unused : 5;
    uint32_t sub_sof : 1;
    uint32_t eof : 1;
    uint32_t owner : 1;
    uint32_t buf_ptr;
    uint32_t next_link_ptr;
  };

  Esp8266DmaI2sTransportSettings _config;
  size_t _frameBytes{0};

  uint8_t* _idleData{nullptr};
  size_t _idleDataSize{0};

  SlcDescriptor* _descriptors{nullptr};
  size_t _descriptorCount{0};
  size_t _dataBlockCount{0};

  volatile DmaState _dmaState{DmaState::Idle};
  bool _initialised{false};

  static size_t roundUp4(size_t value) { return (value + 3) & ~static_cast<size_t>(3); }

  static uint8_t reverseBits8(uint8_t value)
  {
    value = static_cast<uint8_t>(((value & 0xF0U) >> 4) | ((value & 0x0FU) << 4));
    value = static_cast<uint8_t>(((value & 0xCCU) >> 2) | ((value & 0x33U) << 2));
    value = static_cast<uint8_t>(((value & 0xAAU) >> 1) | ((value & 0x55U) << 1));
    return value;
  }

  void ensureInitialised(size_t frameBytes)
  {
    if (_initialised && _frameBytes == frameBytes)
    {
      return;
    }

    if (_initialised)
    {
      while (!isReadyToUpdate())
      {
        yield();
      }
      stopI2s();
      _initialised = false;
    }

    freeI2sBuffers();
    _frameBytes = frameBytes;

    allocateI2sBuffers();
    initI2s();
    _initialised = true;
  }

  void allocateI2sBuffers()
  {
    _idleDataSize = 256;
    _idleData = static_cast<uint8_t*>(malloc(_idleDataSize));

    uint8_t idleFill = _config.invert ? 0xFF : 0x00;
    if (_idleData)
    {
      std::memset(_idleData, idleFill, _idleDataSize);
    }

    _dataBlockCount = (_frameBytes + MaxDmaBlockSize - 1) / MaxDmaBlockSize;

    _descriptorCount = 2 + _dataBlockCount;
    _descriptors = static_cast<SlcDescriptor*>(calloc(_descriptorCount, sizeof(SlcDescriptor)));

    if (_idleData == nullptr || _descriptors == nullptr)
    {
      return;
    }

    _descriptors[0].blocksize = 4;
    _descriptors[0].datalen = 4;
    _descriptors[0].buf_ptr = reinterpret_cast<uint32_t>(_idleData);
    _descriptors[0].owner = 1;
    _descriptors[0].next_link_ptr = reinterpret_cast<uint32_t>(&_descriptors[1]);

    _descriptors[1].blocksize = 4;
    _descriptors[1].datalen = 4;
    _descriptors[1].buf_ptr = reinterpret_cast<uint32_t>(_idleData + 4);
    _descriptors[1].owner = 1;
    _descriptors[1].next_link_ptr = reinterpret_cast<uint32_t>(&_descriptors[0]);
  }

  void freeI2sBuffers()
  {
    free(_idleData);
    _idleData = nullptr;
    free(_descriptors);
    _descriptors = nullptr;
    _dataBlockCount = 0;
  }

  void bindDataDescriptors(uint8_t* data, size_t length)
  {
    if (_descriptors == nullptr || _dataBlockCount == 0 || data == nullptr || length == 0)
    {
      return;
    }

    size_t remaining = length;
    uint8_t* pBuf = data;
    for (size_t i = 0; i < _dataBlockCount; ++i)
    {
      const size_t blockLen = (remaining > MaxDmaBlockSize) ? MaxDmaBlockSize : remaining;
      auto& desc = _descriptors[2 + i];
      desc.blocksize = static_cast<uint32_t>(blockLen);
      desc.datalen = static_cast<uint32_t>(blockLen);
      desc.buf_ptr = reinterpret_cast<uint32_t>(pBuf);
      desc.owner = 1;
      desc.eof = (i == _dataBlockCount - 1) ? 1 : 0;
      desc.next_link_ptr = (i == _dataBlockCount - 1) ? reinterpret_cast<uint32_t>(&_descriptors[0]) : reinterpret_cast<uint32_t>(&_descriptors[2 + i + 1]);
      pBuf += blockLen;
      remaining -= blockLen;
    }
  }

  void initI2s()
  {
    pinMode(I2sPin, FUNCTION_1);

    SLCC0 |= SLCRXLR | SLCTXLR;
    SLCC0 &= ~(SLCRXLR | SLCTXLR);
    SLCIC = 0xFFFFFFFF;
    SLCC0 &= ~(SLCMM << SLCM);
    SLCC0 |= (1 << SLCM);

    SLCRXDC |= SLCBINR | SLCBTNR;
    SLCRXDC &= ~(SLCBRXFE | SLCBRXEM | SLCBRXFM);

    SLCTXL &= ~(SLCTXLAM << SLCTXLA);
    SLCTXL |= reinterpret_cast<uint32_t>(&_descriptors[0]) << SLCTXLA;

    SLCIE = SLCIRXEOF;

    ETS_SLC_INTR_ATTACH(slcIsr, reinterpret_cast<void*>(this));
    ETS_SLC_INTR_ENABLE();

    I2SC = 0;
    I2SC |= I2SRST;
    I2SC &= ~I2SRST;

    I2SFC &= ~(I2SDE | (I2STXFMM << I2STXFM) | (I2SRXFMM << I2SRXFM));

    uint32_t targetHz = (_config.clockRateHz == 0) ? 2500000UL : _config.clockRateHz;
    configureClock(targetHz);

    I2SC |= I2STXS;
    SLCTXL |= SLCTXLS;
  }

  void configureClock(uint32_t targetRateHz)
  {
    static constexpr uint32_t BaseFreq = 160000000UL;
    uint8_t bestBclkDiv = 1;
    uint8_t bestClkDiv = 1;
    int32_t bestError = INT32_MAX;

    for (uint8_t bclk = 1; bclk <= 63; ++bclk)
    {
      for (uint8_t clk = 1; clk <= 63; ++clk)
      {
        uint32_t rate = BaseFreq / (bclk * clk);
        int32_t error = static_cast<int32_t>(rate) - static_cast<int32_t>(targetRateHz);
        if (error < 0)
        {
          error = -error;
        }
        if (error < bestError)
        {
          bestError = error;
          bestBclkDiv = bclk;
          bestClkDiv = clk;
        }
      }
    }

    I2SC &= ~(I2SBMM << I2SBM);
    I2SC |= (bestBclkDiv << I2SBM);
    I2SC &= ~(I2SCDM << I2SCD);
    I2SC |= (bestClkDiv << I2SCD);
  }

  void writeI2s()
  {
    _dmaState = DmaState::Sending;
    _descriptors[1].next_link_ptr = reinterpret_cast<uint32_t>(&_descriptors[2]);
  }

  void stopI2s()
  {
    ETS_SLC_INTR_DISABLE();
    SLCTXL &= ~SLCTXLS;
    I2SC &= ~I2STXS;
    pinMode(I2sPin, INPUT);
  }

  static void IRAM_ATTR slcIsr(void* arg)
  {
    uint32_t status = SLCIS;
    SLCIC = 0xFFFFFFFF;

    if (status & SLCIRXEOF)
    {
      auto* self = static_cast<Esp8266DmaI2sTransport*>(arg);
      self->_descriptors[1].next_link_ptr = reinterpret_cast<uint32_t>(&self->_descriptors[0]);
      self->_dmaState = DmaState::Idle;
    }
  }
};

} // namespace lw::transports::esp8266

#endif // ARDUINO_ARCH_ESP8266
