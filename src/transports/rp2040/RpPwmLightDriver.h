#pragma once

#ifdef ARDUINO_ARCH_RP2040

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <Arduino.h>

#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "colors/ChannelMap.h"
#include "colors/ColorMath.h"
#include "core/OutputPipeline.h"

namespace lw::transports::rp2040
{

struct RpPwmLightDriverSettings
{
  static constexpr size_t MaxChannels = 4;
  using PinsMap = ChannelMap<int>;

  PinsMap pins{-1};
  uint16_t wrap{255};
  float clockDiv{4.0f};
  bool invert{false};

  static RpPwmLightDriverSettings normalize(RpPwmLightDriverSettings settings)
  {
    if (settings.wrap == 0)
    {
      settings.wrap = 1;
    }

    if (settings.clockDiv < 1.0f)
    {
      settings.clockDiv = 1.0f;
    }

    return settings;
  }
};

class RpPwmLightDriver : public lw::buses::OutputPipeline
{
public:
  using ColorType = lw::Color;
  using BrightnessType = lw::colors::ColorComponent;
  using LightDriverSettingsType = RpPwmLightDriverSettings;

  explicit RpPwmLightDriver(LightDriverSettingsType settings) : _settings(LightDriverSettingsType::normalize(settings)) {}

  ~RpPwmLightDriver() override
  {
    if (!_begun)
    {
      return;
    }

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin >= 0)
      {
        pinMode(pin, INPUT);
      }
    }
  }

  void begin() override
  {
    if (_begun)
    {
      return;
    }

    std::array<bool, NUM_PWM_SLICES> initializedSlices{};

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      const uint gpioPin = static_cast<uint>(pin);
      gpio_set_function(gpioPin, GPIO_FUNC_PWM);

      const uint slice = pwm_gpio_to_slice_num(gpioPin);
      if (!initializedSlices[slice])
      {
        pwm_config config = pwm_get_default_config();
        pwm_config_set_clkdiv(&config, _settings.clockDiv);
        pwm_config_set_wrap(&config, _settings.wrap);
        pwm_init(slice, &config, true);
        initializedSlices[slice] = true;
      }

      if (_settings.invert)
      {
        gpio_set_outover(gpioPin, GPIO_OVERRIDE_INVERT);
      }

      pwm_set_gpio_level(gpioPin, 0);
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

    using ComponentType = typename ColorType::ComponentType;
    using WideType = std::conditional_t<(sizeof(ComponentType) <= 2), uint32_t, uint64_t>;

    const WideType componentMax = static_cast<WideType>(std::numeric_limits<ComponentType>::max());
    const WideType wrap = static_cast<WideType>(_settings.wrap);

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      const char channelTag = "RGBW"[channel];
      const WideType component = static_cast<WideType>(lw::colors::applyBrightness(color[channelTag], brightness));
      const WideType scaled = (component * wrap + (componentMax / 2U)) / componentMax;
      const uint16_t level = static_cast<uint16_t>(scaled);
      pwm_set_gpio_level(static_cast<uint>(pin), level);
    }
  }

private:
  LightDriverSettingsType _settings;
  bool _begun{false};
};

} // namespace lw::transports::rp2040

#endif // ARDUINO_ARCH_RP2040
