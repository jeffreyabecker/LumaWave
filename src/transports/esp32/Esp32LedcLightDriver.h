#pragma once

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP8266)
#if __has_include("driver/ledc.h")
#include "driver/ledc.h"
#else
#error "Esp32LedcLightDriver on ESP8266 requires driver/ledc.h from the SDK"
#endif
#endif

#include "palettes/ColorMath.h"
#include "core/OutputPipeline.h"

namespace lw::transports::esp32
{

struct Esp32LedcLightDriverSettings
{
  static constexpr size_t MaxChannels = 4;
  using PinsMap = std::array<int, 4>;
  static constexpr uint32_t DefaultFrequencyHz = 5000;
  static constexpr uint8_t DefaultResolutionBits = 8;

  PinsMap pins{-1, -1, -1, -1};
  uint32_t frequencyHz{DefaultFrequencyHz};
  uint8_t resolutionBits{DefaultResolutionBits};
  bool invert{false};

  static Esp32LedcLightDriverSettings normalize(Esp32LedcLightDriverSettings settings)
  {
    if (settings.frequencyHz == 0)
    {
      settings.frequencyHz = DefaultFrequencyHz;
    }

#if defined(ARDUINO_ARCH_ESP8266)
    if (settings.resolutionBits < 1)
    {
      settings.resolutionBits = 1;
    }
    else if (settings.resolutionBits > 16)
    {
      settings.resolutionBits = 16;
    }
#else
    if (settings.resolutionBits < 1)
    {
      settings.resolutionBits = 1;
    }
    else if (settings.resolutionBits > 14)
    {
      settings.resolutionBits = 14;
    }
#endif

    return settings;
  }
};

class Esp32LedcLightDriver : public lw::OutputPipeline
{
public:
  using ColorType = lw::Color;
  using BrightnessType = lw::ColorComponent;
  using LightDriverSettingsType = Esp32LedcLightDriverSettings;

  explicit Esp32LedcLightDriver(LightDriverSettingsType settings) : _settings(LightDriverSettingsType::normalize(settings)), _maxDuty(computeMaxDuty(_settings.resolutionBits)) {}

  ~Esp32LedcLightDriver() override
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

#if defined(ARDUINO_ARCH_ESP32)
      ledcWrite(static_cast<uint8_t>(channel), 0U);
      ledcDetachPin(static_cast<uint8_t>(pin));
#else
      ledc_stop(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel), 0U);
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

#if defined(ARDUINO_ARCH_ESP8266)
    ledc_timer_config_t timerConfig{};
    timerConfig.speed_mode = LEDC_LOW_SPEED_MODE;
    timerConfig.duty_resolution = static_cast<ledc_timer_bit_t>(_settings.resolutionBits);
    timerConfig.timer_num = LEDC_TIMER_0;
    timerConfig.freq_hz = _settings.frequencyHz;
    timerConfig.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timerConfig);
#endif

    for (size_t channel = 0; channel < activeChannelCount(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      pinMode(pin, OUTPUT);

#if defined(ARDUINO_ARCH_ESP32)
      const uint8_t ledcChannel = static_cast<uint8_t>(channel);
      ledcSetup(ledcChannel, _settings.frequencyHz, _settings.resolutionBits);
      ledcAttachPin(static_cast<uint8_t>(pin), ledcChannel);
      ledcWrite(ledcChannel, 0U);
#else
      ledc_channel_config_t channelConfig{};
      channelConfig.gpio_num = static_cast<int>(pin);
      channelConfig.speed_mode = LEDC_LOW_SPEED_MODE;
      channelConfig.channel = static_cast<ledc_channel_t>(channel);
      channelConfig.intr_type = LEDC_INTR_DISABLE;
      channelConfig.timer_sel = LEDC_TIMER_0;
      channelConfig.duty = 0;
      channelConfig.hpoint = 0;
      ledc_channel_config(&channelConfig);
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

    const auto adjusted = lw::applyBrightness(colors[0], brightness);
    if (!_begun)
    {
      begin();
    }

    using ComponentType = typename ColorType::ComponentType;
    using WideType = std::conditional_t<(sizeof(ComponentType) <= 2), uint32_t, uint64_t>;

    const WideType componentMax = static_cast<WideType>(std::numeric_limits<ComponentType>::max());
    const WideType pwmMax = static_cast<WideType>(_maxDuty);

    for (size_t channel = 0; channel < activeChannelCount(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      const char channelTag = "RGBW"[channel];
      const WideType component = static_cast<WideType>(lw::colorComponentByIndex(adjusted, channel));
      WideType duty = (component * pwmMax + (componentMax / 2U)) / componentMax;
      if (_settings.invert)
      {
        duty = pwmMax - duty;
      }

#if defined(ARDUINO_ARCH_ESP32)
      ledcWrite(static_cast<uint8_t>(channel), static_cast<uint32_t>(duty));
#else
      ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel), static_cast<uint32_t>(duty));
      ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel));
#endif
    }
  }

private:
  static uint32_t computeMaxDuty(uint8_t resolutionBits)
  {
    if (resolutionBits >= 31)
    {
      return 0x7FFFFFFFU;
    }

    return (1UL << resolutionBits) - 1UL;
  }

  size_t activeChannelCount() const
  {
    const size_t colorChannels = 4;
    const size_t configuredPins = _settings.pins.size();

    size_t count = (colorChannels < configuredPins) ? colorChannels : configuredPins;
#if defined(LEDC_CHANNEL_MAX)
    const size_t hardwareChannels = static_cast<size_t>(LEDC_CHANNEL_MAX);
    if (count > hardwareChannels)
    {
      count = hardwareChannels;
    }
#endif
    return count;
  }

  LightDriverSettingsType _settings;
  uint32_t _maxDuty{255};
  bool _begun{false};
};

} // namespace lw::transports::esp32

#endif // ARDUINO_ARCH_ESP8266 || ARDUINO_ARCH_ESP32
