#pragma once

#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C3)

#include <cstdint>
#include <cstddef>
#include <cmath>

#include <Arduino.h>
#include "esp_heap_caps.h"
#include "esp_err.h"

#if ESP_IDF_VERSION_MAJOR >= 4
#include "esp_intr_alloc.h"
#else
#include "esp_intr.h"
#endif

#include "rom/lldesc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_struct.h"

#if defined(CONFIG_IDF_TARGET_ESP32)
#include "soc/dport_reg.h"
#endif

#if ESP_IDF_VERSION_MAJOR >= 5
#include "esp_private/periph_ctrl.h"
#else
#include "driver/periph_ctrl.h"
#endif

#include "transports/Transport.h"

namespace lw::transports::esp32
{

struct Esp32I2sTransportSettings : TransportSettingsBase
{
  uint8_t busNumber = 0;
};

class Esp32I2sTransport : public Transport
{
public:
  using TransportSettingsType = Esp32I2sTransportSettings;
  static constexpr size_t DmaBitsPerClockDataBit = 1;

  explicit Esp32I2sTransport(Esp32I2sTransportSettings config) : _config{config}, _bus{resolveBus(config.busNumber)} {}

  ~Esp32I2sTransport() override
  {
    if (_initialised)
    {
      while (!isReadyToUpdate())
      {
        yield();
      }
      deinitI2s();
      gpio_matrix_out(_config.dataPin, SIG_GPIO_OUT_IDX, false, false);
      pinMode(_config.dataPin, INPUT);
      if (_config.clockPin >= 0)
      {
        gpio_matrix_out(_config.clockPin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(_config.clockPin, INPUT);
      }
    }

    deinitDmaItems();
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
    if (_dmaItems == nullptr)
    {
      return;
    }

    bindDataDescriptors(data.data(), data.size());
    i2sWrite();
  }

  void endTransaction() override {}

  bool isReadyToUpdate() const override
  {
    if (!_initialised)
    {
      return true;
    }
    return i2sWriteDone();
  }

private:
  enum class I2sChannelMode : uint8_t
  {
    Stereo = 0
  };

  enum class I2sFifoMode : uint8_t
  {
    Fifo16BitDual = 0
  };

  static constexpr size_t I2sDmaMaxDataLen = 4092;
  static constexpr size_t I2sDmaSilenceSize = 4;
  static constexpr size_t I2sDmaSilenceBlockCountFront = 2;
  static constexpr size_t I2sDmaSilenceBlockCountBack = 1;
  static constexpr uint32_t I2sBaseClk = 160000000UL;
  static constexpr uint8_t ClockDividerBck = 4;
  static constexpr uint32_t I2sIsIdle = 0;
  static constexpr uint32_t I2sIsSending = 2;

  Esp32I2sTransportSettings _config;
  i2s_dev_t* _bus{nullptr};
  intr_handle_t _isrHandle{nullptr};
  lldesc_t* _dmaItems{nullptr};
  size_t _dmaCount{0};
  size_t _dataBlockCount{0};
  volatile uint32_t _sendState{I2sIsIdle};
  uint8_t* _silenceBuffer{nullptr};
  size_t _frameBytes{0};
  bool _initialised{false};

  static size_t roundUp4(size_t value) { return (value + 3) & ~static_cast<size_t>(3); }

  static uint16_t bitSendTimeNsFromRate(uint32_t rateHz)
  {
    if (rateHz == 0)
    {
      return 400;
    }

    const uint32_t ns = 1000000000UL / rateHz;
    return static_cast<uint16_t>((ns == 0) ? 1 : ns);
  }

  static i2s_dev_t* resolveBus(uint8_t busNumber)
  {
#if !defined(CONFIG_IDF_TARGET_ESP32S2)
    if (busNumber == 1)
    {
      return &I2S1;
    }
#endif
    if (busNumber == 0)
    {
      return &I2S0;
    }

    return nullptr;
  }

  static void toFractionClocks(uint8_t* numerator, uint8_t* denominator, double unitDecimal, double accuracy)
  {
    if (unitDecimal <= accuracy)
    {
      *numerator = 0;
      *denominator = 1;
      return;
    }
    if (unitDecimal <= (1.0 / 63.0))
    {
      *numerator = 0;
      *denominator = 2;
      return;
    }
    if (unitDecimal >= (62.0 / 63.0))
    {
      *numerator = 2;
      *denominator = 2;
      return;
    }

    uint16_t lowerN = 0;
    uint16_t lowerD = 1;

    uint16_t upperN = 1;
    uint16_t upperD = 1;
    double upperDelta = 1.0 - unitDecimal;

    uint16_t closestN = 0;
    uint16_t closestD = 1;
    double closestDelta = unitDecimal;

    for (;;)
    {
      const uint16_t middleN = lowerN + upperN;
      const uint16_t middleD = lowerD + upperD;
      const double middleUnit = static_cast<double>(middleN) / static_cast<double>(middleD);

      if (middleD > 63)
      {
        break;
      }

      if (middleD * (unitDecimal + accuracy) < middleN)
      {
        upperN = middleN;
        upperD = middleD;
        upperDelta = middleUnit - unitDecimal;
      }
      else if (middleN < (unitDecimal - accuracy) * middleD)
      {
        lowerN = middleN;
        lowerD = middleD;
      }
      else
      {
        *numerator = static_cast<uint8_t>(middleN);
        *denominator = static_cast<uint8_t>(middleD);
        return;
      }

      if (upperDelta < closestDelta)
      {
        closestN = upperN;
        closestD = upperD;
        closestDelta = upperDelta;
      }
    }

    *numerator = static_cast<uint8_t>(closestN);
    *denominator = static_cast<uint8_t>(closestD);
  }

  static void dmaItemInit(lldesc_t* item, uint8_t* position, size_t size, lldesc_t* next)
  {
    item->eof = 0;
    item->owner = 1;
    item->sosf = 0;
    item->offset = 0;
    item->buf = position;
    item->size = size;
    item->length = size;
    item->qe.stqe_next = next;
  }

  bool initDmaItems(size_t frameBytes, size_t bytesPerSample)
  {
    const size_t silenceSize = I2sDmaSilenceSize;

    if (_dmaItems == nullptr)
    {
      _dmaItems = static_cast<lldesc_t*>(heap_caps_malloc(_dmaCount * sizeof(lldesc_t), MALLOC_CAP_DMA));
      if (_dmaItems == nullptr)
      {
        return false;
      }
    }

    (void)bytesPerSample;

    if (_silenceBuffer == nullptr)
    {
      _silenceBuffer = static_cast<uint8_t*>(heap_caps_malloc(silenceSize, MALLOC_CAP_DMA));
      if (_silenceBuffer == nullptr)
      {
        return false;
      }
      _silenceBuffer[0] = 0;
      _silenceBuffer[1] = 0;
      _silenceBuffer[2] = 0;
      _silenceBuffer[3] = 0;
    }

    lldesc_t* itemFirst = &_dmaItems[0];
    lldesc_t* item = itemFirst;
    lldesc_t* itemNext = item + 1;

    size_t dataLeft = frameBytes;
    uint8_t* silencePosition = _silenceBuffer;

    dmaItemInit(item, silencePosition, silenceSize, itemNext);
    dmaItemInit(itemNext, silencePosition, silenceSize, item);
    item = itemNext;
    ++itemNext;

    while (dataLeft)
    {
      item = itemNext;
      ++itemNext;

      size_t blockSize = dataLeft;
      if (blockSize > I2sDmaMaxDataLen)
      {
        blockSize = I2sDmaMaxDataLen;
      }
      dataLeft -= blockSize;

      dmaItemInit(item, silencePosition, blockSize, itemNext);
    }

    item->eof = 1;

    item = itemNext;
    dmaItemInit(item, silencePosition, silenceSize, itemFirst);

    return true;
  }

  void deinitDmaItems()
  {
    if (_dmaItems)
    {
      heap_caps_free(_dmaItems);
      _dmaItems = nullptr;
    }

    if (_silenceBuffer)
    {
      heap_caps_free(_silenceBuffer);
      _silenceBuffer = nullptr;
    }
  }

  void bindDataDescriptors(uint8_t* data, size_t dataSize)
  {
    if (_dmaItems == nullptr || _dataBlockCount == 0 || data == nullptr || dataSize == 0)
    {
      return;
    }

    size_t dataLeft = dataSize;
    uint8_t* position = data;

    for (size_t i = 0; i < _dataBlockCount; ++i)
    {
      lldesc_t* item = &_dmaItems[2 + i];
      size_t blockSize = dataLeft;
      if (blockSize > I2sDmaMaxDataLen)
      {
        blockSize = I2sDmaMaxDataLen;
      }

      item->buf = position;
      item->size = blockSize;
      item->length = blockSize;
      item->owner = 1;
      item->eof = (i == (_dataBlockCount - 1)) ? 1 : 0;

      position += blockSize;
      dataLeft -= blockSize;
    }
  }

  bool setClock(uint8_t dividerInteger, uint8_t dividerNumerator, uint8_t dividerDenominator, uint8_t bckDivider, uint8_t bitsPerSample)
  {
    if (_bus == nullptr || dividerDenominator > 63 || dividerNumerator > 63 || bckDivider > 63)
    {
      return false;
    }

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->clkm_conf)>> clkm{};
    clkm.val = 0;
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
    clkm.clk_sel = 2;
    clkm.clk_en = 1;
#else
    clkm.clk_en = 1;
    clkm.clka_en = 0;
#endif
    clkm.clkm_div_a = dividerDenominator;
    clkm.clkm_div_b = dividerNumerator;
    clkm.clkm_div_num = dividerInteger;
    _bus->clkm_conf.val = clkm.val;

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->sample_rate_conf)>> sampleRate{};
    sampleRate.val = 0;
    sampleRate.tx_bck_div_num = bckDivider;
    sampleRate.tx_bits_mod = bitsPerSample;
    _bus->sample_rate_conf.val = sampleRate.val;

    return true;
  }

  bool setSampleRate(uint16_t dmaBitPerDataBit, uint16_t bitSendTimeNs, size_t bytesPerSample)
  {
    if (_bus == nullptr)
    {
      return false;
    }

    const double i2sClkMhz = static_cast<double>(I2sBaseClk) / 1000000.0;
    const size_t bitsPerSample = bytesPerSample * 8;

    double sampleAdjust = 2.0;
    double clockDivider = static_cast<double>(bitSendTimeNs) / static_cast<double>(bytesPerSample) / static_cast<double>(dmaBitPerDataBit) / static_cast<double>(ClockDividerBck) / 1000.0 * i2sClkMhz * sampleAdjust;

    if (clockDivider > 256.0 || clockDivider < 2.0)
    {
      return false;
    }

    uint8_t dividerInteger = static_cast<uint8_t>(clockDivider);
    uint8_t dividerNumerator = 0;
    uint8_t dividerDenominator = 0;
    const double fractional = clockDivider - static_cast<double>(dividerInteger);
    toFractionClocks(&dividerNumerator, &dividerDenominator, fractional, 0.000001);

    return setClock(dividerInteger, dividerNumerator, dividerDenominator, ClockDividerBck, static_cast<uint8_t>(bitsPerSample));
  }

  static int interruptSourceForBus(uint8_t busNumber)
  {
#if !defined(CONFIG_IDF_TARGET_ESP32S2)
    if (busNumber == 1)
    {
      return ETS_I2S1_INTR_SOURCE;
    }
#endif
    return ETS_I2S0_INTR_SOURCE;
  }

  void setPinsDataOnly()
  {
    if (_config.dataPin < 0)
    {
      return;
    }

#if defined(CONFIG_IDF_TARGET_ESP32S2)
    uint32_t signalData = I2S0O_DATA_OUT23_IDX;
#else
    uint32_t signalData = I2S0O_DATA_OUT23_IDX;
    if (_config.busNumber == 1)
    {
      signalData = I2S1O_DATA_OUT23_IDX;
    }
#endif

    pinMode(_config.dataPin, OUTPUT);
    gpio_matrix_out(_config.dataPin, signalData, _config.invert, false);
  }

  void setClockAndDataPins()
  {
#if defined(CONFIG_IDF_TARGET_ESP32S2)
    uint32_t signalData = I2S0O_DATA_OUT23_IDX;
    uint32_t signalClock = I2S0O_BCK_OUT_IDX;
#else
    uint32_t signalData = I2S0O_DATA_OUT23_IDX;
    uint32_t signalClock = I2S0O_BCK_OUT_IDX;

    if (_config.busNumber == 1)
    {
      signalData = I2S1O_DATA_OUT23_IDX;
      signalClock = I2S1O_BCK_OUT_IDX;
    }
#endif

    pinMode(_config.dataPin, OUTPUT);
    gpio_matrix_out(_config.dataPin, signalData, _config.invert, false);

    pinMode(_config.clockPin, OUTPUT);
    gpio_matrix_out(_config.clockPin, signalClock, false, false);
  }

  static void IRAM_ATTR dmaIsr(void* context)
  {
    auto* self = static_cast<Esp32I2sTransport*>(context);
    if (self != nullptr)
    {
      self->onDmaIsr();
    }
  }

  void IRAM_ATTR onDmaIsr()
  {
    if (_bus == nullptr)
    {
      return;
    }

    if (_bus->int_st.out_eof && _sendState != I2sIsIdle)
    {
      lldesc_t* loop = &_dmaItems[0];
      lldesc_t* loopBreaker = loop + 1;
      loopBreaker->qe.stqe_next = loop;
      _sendState = I2sIsIdle;
    }

    _bus->int_clr.val = _bus->int_st.val;
  }

  bool initI2s(size_t dmaBlockCount, uint16_t bitSendTimeNs)
  {
    if (_bus == nullptr)
    {
      return false;
    }

    _dmaCount = dmaBlockCount + I2sDmaSilenceBlockCountFront + I2sDmaSilenceBlockCountBack;

    if (!initDmaItems(_frameBytes, 2))
    {
      return false;
    }

#if !defined(CONFIG_IDF_TARGET_ESP32S2)
    if (_config.busNumber == 1)
    {
      periph_module_enable(PERIPH_I2S1_MODULE);
    }
    else
#endif
    {
      periph_module_enable(PERIPH_I2S0_MODULE);
    }

    if (_isrHandle != nullptr)
    {
      esp_intr_disable(_isrHandle);
      esp_intr_free(_isrHandle);
      _isrHandle = nullptr;
    }

    _bus->out_link.stop = 1;
    _bus->conf.tx_start = 0;
    _bus->int_ena.val = 0;
    _bus->int_clr.val = 0xFFFFFFFF;
    _bus->fifo_conf.dscr_en = 0;

    _bus->conf.tx_reset = 1;
    _bus->conf.tx_reset = 0;
    _bus->conf.rx_reset = 1;
    _bus->conf.rx_reset = 0;

    _bus->lc_conf.in_rst = 1;
    _bus->lc_conf.in_rst = 0;
    _bus->lc_conf.out_rst = 1;
    _bus->lc_conf.out_rst = 0;

    _bus->conf.rx_fifo_reset = 1;
    _bus->conf.rx_fifo_reset = 0;
    _bus->conf.tx_fifo_reset = 1;
    _bus->conf.tx_fifo_reset = 0;

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->conf2)>> conf2{};
    conf2.val = 0;
    conf2.lcd_en = 0;
    _bus->conf2.val = conf2.val;

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->lc_conf)>> lcConf{};
    lcConf.val = 0;
    lcConf.out_eof_mode = 1;
    _bus->lc_conf.val = lcConf.val;

#if !defined(CONFIG_IDF_TARGET_ESP32S2)
    _bus->pdm_conf.pcm2pdm_conv_en = 0;
    _bus->pdm_conf.pdm2pcm_conv_en = 0;
#endif

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->fifo_conf)>> fifoConf{};
    fifoConf.val = 0;
    fifoConf.tx_fifo_mod_force_en = 1;
    fifoConf.tx_fifo_mod = static_cast<uint8_t>(I2sFifoMode::Fifo16BitDual);
    fifoConf.tx_data_num = 32;
    _bus->fifo_conf.val = fifoConf.val;

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->conf1)>> conf1{};
    conf1.val = 0;
    conf1.tx_stop_en = 0;
    conf1.tx_pcm_bypass = 1;
    _bus->conf1.val = conf1.val;

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->conf_chan)>> confChan{};
    confChan.val = 0;
    confChan.tx_chan_mod = static_cast<uint8_t>(I2sChannelMode::Stereo);
    _bus->conf_chan.val = confChan.val;

    std::remove_cv_t<std::remove_reference_t<decltype(_bus->conf)>> conf{};
    conf.val = 0;
    conf.tx_msb_shift = 1;
    conf.tx_right_first = 1;
    conf.tx_short_sync = 0;
    _bus->conf.val = conf.val;

    _bus->timing.val = 0;

#if !defined(CONFIG_IDF_TARGET_ESP32S2)
    _bus->pdm_conf.tx_pdm_en = 0;
#endif

    if (!setSampleRate(static_cast<uint16_t>(DmaBitsPerClockDataBit), bitSendTimeNs, 2))
    {
      return false;
    }

    _bus->lc_conf.in_rst = 1;
    _bus->lc_conf.out_rst = 1;
    _bus->lc_conf.ahbm_rst = 1;
    _bus->lc_conf.ahbm_fifo_rst = 1;
    _bus->lc_conf.in_rst = 0;
    _bus->lc_conf.out_rst = 0;
    _bus->lc_conf.ahbm_rst = 0;
    _bus->lc_conf.ahbm_fifo_rst = 0;

    _bus->conf.tx_reset = 1;
    _bus->conf.tx_fifo_reset = 1;
    _bus->conf.rx_fifo_reset = 1;
    _bus->conf.tx_reset = 0;
    _bus->conf.tx_fifo_reset = 0;
    _bus->conf.rx_fifo_reset = 0;

    const int interruptSource = interruptSourceForBus(_config.busNumber);
    esp_intr_alloc(interruptSource, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1, &Esp32I2sTransport::dmaIsr, this, &_isrHandle);

    _bus->int_ena.out_eof = 1;
    _bus->int_ena.out_dscr_err = 1;

    _bus->fifo_conf.dscr_en = 1;
    _bus->out_link.start = 0;
    _bus->out_link.addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&_dmaItems[0]));
    _bus->out_link.start = 1;
    _bus->conf.tx_start = 1;

    esp_intr_enable(_isrHandle);
    _sendState = I2sIsIdle;

    return true;
  }

  void deinitI2s()
  {
    if (_isrHandle != nullptr)
    {
      esp_intr_disable(_isrHandle);
      esp_intr_free(_isrHandle);
      _isrHandle = nullptr;
    }

    deinitDmaItems();
    _sendState = I2sIsIdle;
  }

  void i2sWrite()
  {
    if (_dmaItems == nullptr)
    {
      return;
    }

    lldesc_t* loopBreaker = &_dmaItems[1];
    lldesc_t* next = loopBreaker + 1;
    loopBreaker->qe.stqe_next = next;
    _sendState = I2sIsSending;
  }

  bool i2sWriteDone() const { return _sendState == I2sIsIdle; }

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
      deinitI2s();
      _initialised = false;
    }

    _frameBytes = frameBytes;
    _dataBlockCount = (_frameBytes + I2sDmaMaxDataLen - 1) / I2sDmaMaxDataLen;
    size_t dmaBlockCount = _dataBlockCount;

    uint16_t bitSendTimeNs = bitSendTimeNsFromRate(_config.clockRateHz);

    if (!initI2s(dmaBlockCount, bitSendTimeNs))
    {
      return;
    }

    if (_config.clockPin >= 0)
    {
      setClockAndDataPins();
    }
    else
    {
      setPinsDataOnly();
    }

    _initialised = true;
  }
};

} // namespace lw::transports::esp32

#endif // ARDUINO_ARCH_ESP32 && !ESP32S3 && !ESP32C3
