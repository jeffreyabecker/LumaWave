#pragma once

#ifdef ARDUINO_ARCH_ESP32

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <Arduino.h>

#include "colors/ChannelMap.h"
#include "colors/ColorMath.h"
#include "core/IOutputPipeline.h"

#if __has_include("driver/sdm.h")
#include "driver/sdm.h"
#define LW_ESP32_USE_SDM_DRIVER 1
#elif __has_include("driver/sigmadelta.h")
#include "driver/sigmadelta.h"
#define LW_ESP32_USE_SDM_DRIVER 0
#else
#error "ESP32 SigmaDelta light driver requires driver/sdm.h or driver/sigmadelta.h"
#endif

#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#endif

namespace lw::transports::esp32
{

struct Esp32SigmaDeltaLightDriverSettings
{
  static constexpr size_t MaxChannels = 4;
  using PinsMap = ChannelMap<int>;

  PinsMap pins{-1};
  uint32_t sampleRateHz{1000000UL};
  uint8_t prescale{80};
  bool invert{false};

  static Esp32SigmaDeltaLightDriverSettings normalize(Esp32SigmaDeltaLightDriverSettings settings)
  {
    if (settings.sampleRateHz == 0)
    {
      settings.sampleRateHz = 1000000UL;
    }

    return settings;
  }
};

class Esp32SigmaDeltaLightDriver : public IOutputPipeline
{
public:
  using ColorType = lw::Color;
  using BrightnessType = lw::colors::ColorComponent;
  using LightDriverSettingsType = Esp32SigmaDeltaLightDriverSettings;

  explicit Esp32SigmaDeltaLightDriver(LightDriverSettingsType settings) : _settings(LightDriverSettingsType::normalize(settings)) {}

  ~Esp32SigmaDeltaLightDriver() override
  {
    if (!_begun)
    {
      return;
    }

    for (size_t channel = 0; channel < activeChannelCount(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

#if LW_ESP32_USE_SDM_DRIVER
      if (_channels[channel] != nullptr)
      {
        sdm_channel_set_pulse_density(_channels[channel], mapComponentToDensity(0));
        sdm_channel_disable(_channels[channel]);
        sdm_del_channel(_channels[channel]);
        _channels[channel] = nullptr;
      }
#else
      sigmadelta_set_duty(static_cast<sigmadelta_channel_t>(channel), mapComponentToDensity(0));
#endif
      pinMode(pin, INPUT);
    }
  }

  void begin() override
  {
    if (_begun)
    {
      return;
    }

    for (size_t channel = 0; channel < activeChannelCount(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

#if LW_ESP32_USE_SDM_DRIVER
      sdm_config_t config = {
          .clk_src = SDM_CLK_SRC_DEFAULT,
          .gpio_num = pin,
          .sample_rate_hz = _settings.sampleRateHz,
      };

      sdm_channel_handle_t handle = nullptr;
      if (sdm_new_channel(&config, &handle) == ESP_OK)
      {
        _channels[channel] = handle;
        sdm_channel_enable(handle);
        sdm_channel_set_pulse_density(handle, mapComponentToDensity(0));
      }
#else
      sigmadelta_config_t config{};
      config.channel = static_cast<sigmadelta_channel_t>(channel);
      config.sigmadelta_duty = mapComponentToDensity(0);
      config.sigmadelta_prescale = _settings.prescale;
      sigmadelta_config(&config);
      sigmadelta_set_pin(static_cast<sigmadelta_channel_t>(channel), static_cast<gpio_num_t>(pin));
#endif
    }

    _begun = true;
  }

  bool isReadyToUpdate() const override { return true; }

  bool alwaysUpdate() const override { return false; }

  void write(span<const ColorType> colors, BrightnessType brightness) override
  {
    if (colors.empty())
    {
      return;
    }

    const auto& color = colors[0];
    if (!_begun)
    {
      begin();
    }

    for (size_t channel = 0; channel < activeChannelCount(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      const char channelTag = ColorType::ChannelIndexIterator::channelAt(channel);
      const auto scaledComponent = lw::colors::applyBrightness(color[channelTag], brightness);
      const int8_t duty = mapComponentToDensity(scaledComponent);

#if LW_ESP32_USE_SDM_DRIVER
      if (_channels[channel] != nullptr)
      {
        sdm_channel_set_pulse_density(_channels[channel], duty);
      }
#else
      sigmadelta_set_duty(static_cast<sigmadelta_channel_t>(channel), duty);
#endif
    }
  }

private:
#if LW_ESP32_USE_SDM_DRIVER
#if defined(SOC_SDM_CHANNELS_PER_GROUP)
  static constexpr size_t HardwareChannelCount = static_cast<size_t>(SOC_SDM_CHANNELS_PER_GROUP);
#else
  static constexpr size_t HardwareChannelCount = 4;
#endif
#else
#if defined(SIGMADELTA_CHANNEL_MAX)
  static constexpr size_t HardwareChannelCount = static_cast<size_t>(SIGMADELTA_CHANNEL_MAX);
#else
  static constexpr size_t HardwareChannelCount = 8;
#endif
#endif

  size_t activeChannelCount() const
  {
    const size_t colorChannels = 4;
    const size_t configuredPins = _settings.pins.size();

    size_t count = colorChannels;
    if (count > configuredPins)
    {
      count = configuredPins;
    }
    if (count > HardwareChannelCount)
    {
      count = HardwareChannelCount;
    }

    return count;
  }

  template <typename TComponent> int8_t mapComponentToDensity(TComponent component) const
  {
    using ComponentType = typename ColorType::ComponentType;
    using WideType = std::conditional_t<(sizeof(ComponentType) <= 2), uint32_t, uint64_t>;

    const WideType maxValue = static_cast<WideType>(std::numeric_limits<ComponentType>::max());
    const WideType value = static_cast<WideType>(component);
    const WideType scaled255 = (value * 255U + (maxValue / 2U)) / maxValue;

    int16_t density = static_cast<int16_t>(scaled255) - 128;
    if (_settings.invert)
    {
      density = static_cast<int16_t>(-1 - density);
    }

    return static_cast<int8_t>(density);
  }

  LightDriverSettingsType _settings;
#if LW_ESP32_USE_SDM_DRIVER
  std::array<sdm_channel_handle_t, LightDriverSettingsType::MaxChannels> _channels{nullptr, nullptr, nullptr, nullptr, nullptr};
#endif
  bool _begun{false};
};

} // namespace lw::transports::esp32

#endif // ARDUINO_ARCH_ESP32
