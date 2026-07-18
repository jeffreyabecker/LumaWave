#pragma once

#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)

#include <cstdint>
#include <cstddef>

#include <Arduino.h>
#include "driver/rmt.h"
#include "soc/rmt_struct.h"

#include "transports/Transport.h"
#include "protocols/OneWireTiming.h"

namespace lw::transports::esp32
{

struct Esp32RmtTransportSettings : TransportSettingsBase
{
  Esp32RmtTransportSettings()
  {
    clockRateHz = 0;
    dataPin = 0;
  }

  rmt_channel_t channel = RMT_CHANNEL_0;

  static Esp32RmtTransportSettings normalize(Esp32RmtTransportSettings settings, const OneWireTiming* timing = nullptr)
  {
    if (timing != nullptr)
    {
      normalizeOneWireTransportClockDataBitRate(*timing, settings);
    }
    else if (settings.clockRateHz == 0)
    {
      normalizeOneWireTransportClockDataBitRate(lw::protocols::timing::Ws2812x, settings);
    }

    return settings;
  }
};

class Esp32RmtTransport : public Transport
{
public:
  using TransportSettingsType = Esp32RmtTransportSettings;
  explicit Esp32RmtTransport(Esp32RmtTransportSettings config) : _config{config} { _computeRmtItems(); }

  ~Esp32RmtTransport()
  {
    if (!_initialised)
    {
      return;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_wait_tx_done(_config.channel, 10000 / portTICK_PERIOD_MS));
    ESP_ERROR_CHECK(rmt_driver_uninstall(_config.channel));

    gpio_matrix_out(_config.dataPin, SIG_GPIO_OUT_IDX, false, false);
    pinMode(_config.dataPin, INPUT);
  }

  void begin() override
  {
    if (_initialised)
    {
      return;
    }

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(static_cast<gpio_num_t>(_config.dataPin), _config.channel);
    config.clk_div = RmtClockDivider;
    config.tx_config.idle_level = _config.invert ? RMT_IDLE_LEVEL_HIGH : RMT_IDLE_LEVEL_LOW;
    config.tx_config.idle_output_en = true;

    ESP_ERROR_CHECK(rmt_config(&config));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 3, 0)
    constexpr uint32_t intFlags = ESP_INTR_FLAG_LOWMED;
#else
    constexpr uint32_t intFlags = ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1;
#endif
    ESP_ERROR_CHECK(rmt_driver_install(_config.channel, 0, intFlags));
    ESP_ERROR_CHECK(rmt_translator_init(_config.channel, translateCb));

    _initialised = true;
  }

  void transmitBytes(span<uint8_t> data) override
  {
    if (!_initialised)
    {
      begin();
    }

    s_activeItems = &_rmtItems;
    ESP_ERROR_CHECK(rmt_write_sample(_config.channel, const_cast<uint8_t*>(data.data()), data.size(), false));
  }

  bool isReadyToUpdate() const override
  {
    if (!_initialised)
    {
      return true;
    }

    return rmt_wait_tx_done(_config.channel, 0) == ESP_OK;
  }

private:
  static constexpr uint8_t RmtClockDivider = 2;
  static constexpr float NsPerTick = 25.0f;

  struct RmtItems
  {
    uint32_t bit0;
    uint32_t bit1;
  };

  Esp32RmtTransportSettings _config;
  RmtItems _rmtItems{};
  bool _initialised{false};

  static inline const RmtItems* s_activeItems = nullptr;

  static uint32_t fromNs(uint32_t ns) { return static_cast<uint32_t>(static_cast<float>(ns) / NsPerTick + 0.5f); }

  uint32_t makeLevelItem32(bool highLevel, uint32_t durationNs) const
  {
    uint32_t ticks = fromNs(durationNs);
    if (ticks < 2)
    {
      ticks = 2;
    }

    uint32_t dur0 = ticks / 2;
    uint32_t dur1 = ticks - dur0;
    uint32_t level = (highLevel ^ _config.invert) ? 1u : 0u;

    return (dur1 << 16) | (level << 31) | (level << 15) | dur0;
  }

  void _computeRmtItems()
  {
    uint32_t effectiveclockRateHz = _config.clockRateHz;
    if (effectiveclockRateHz == 0)
    {
      effectiveclockRateHz = timing::Ws2812x.encodedDataRateHz();
    }

    uint32_t bitDurationNs = (effectiveclockRateHz == 0) ? 0 : static_cast<uint32_t>(1000000000UL / effectiveclockRateHz);

    _rmtItems.bit0 = makeLevelItem32(false, bitDurationNs);
    _rmtItems.bit1 = makeLevelItem32(true, bitDurationNs);
  }

  static void IRAM_ATTR translateCb(const void* src, rmt_item32_t* dest, size_t src_size, size_t wanted_num, size_t* translated_size, size_t* item_num)
  {
    const RmtItems* items = s_activeItems;
    if (items == nullptr || src_size == 0 || wanted_num == 0)
    {
      *translated_size = 0;
      *item_num = 0;
      return;
    }

    const uint8_t* pSrc = static_cast<const uint8_t*>(src);
    size_t srcDone = 0;
    size_t itemsDone = 0;

    while (srcDone < src_size && itemsDone + 8 <= wanted_num)
    {
      uint8_t byte = pSrc[srcDone];

      for (int bit = 7; bit >= 0; --bit)
      {
        dest[itemsDone].val = (byte & (1 << bit)) ? items->bit1 : items->bit0;
        ++itemsDone;
      }

      ++srcDone;
    }

    *translated_size = srcDone;
    *item_num = itemsDone;
  }
};

} // namespace lw::transports::esp32

#endif // ARDUINO_ARCH_ESP32
