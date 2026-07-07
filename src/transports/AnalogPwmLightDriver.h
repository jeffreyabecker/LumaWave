#pragma once

#ifdef ARDUINO
#include <Arduino.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "colors/ChannelMap.h"
#include "colors/ColorMath.h"
#include "transports/ILightDriver.h"

namespace lw::transports
{

struct AnalogPwmLightDriverSettings : LightDriverSettingsBase
{
  static constexpr size_t MaxChannels = 4;
  using PinsMap = ChannelMap<int>;
  static constexpr uint16_t DefaultPwmRange = 1023;
  static constexpr uint32_t DefaultPwmFrequencyHz = 1000;

  PinsMap pins{-1};
  uint16_t pwmRange{DefaultPwmRange};
  uint32_t pwmFrequencyHz{DefaultPwmFrequencyHz};
  bool invert{false};

  static AnalogPwmLightDriverSettings normalize(AnalogPwmLightDriverSettings settings)
  {
    if (settings.pwmRange == 0)
    {
      settings.pwmRange = 1;
    }

    if (settings.pwmFrequencyHz == 0)
    {
      settings.pwmFrequencyHz = DefaultPwmFrequencyHz;
    }

    return settings;
  }
};

template <typename TColor> class AnalogPwmLightDriver : public ILightDriver<TColor>
{
public:
  using ColorType = TColor;
  using BrightnessType = typename ILightDriver<TColor>::BrightnessType;
  using LightDriverSettingsType = AnalogPwmLightDriverSettings;

  explicit AnalogPwmLightDriver(LightDriverSettingsType settings) : _settings(LightDriverSettingsType::normalize(settings)) {}

  ~AnalogPwmLightDriver() override
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
        analogWrite(pin, 0);
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

    analogWriteFreq(_settings.pwmFrequencyHz);
    analogWriteRange(_settings.pwmRange);

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      pinMode(pin, OUTPUT);
      analogWrite(pin, 0);
    }

    _begun = true;
  }

  bool isReadyToUpdate() const override { return true; }

  void write(const ColorType& color) override { write(color, std::numeric_limits<BrightnessType>::max()); }

  void write(const ColorType& color, BrightnessType brightness) override
  {
    if (!_begun)
    {
      begin();
    }

    using ComponentType = typename ColorType::ComponentType;
    using WideType = std::conditional_t<(sizeof(ComponentType) <= 2), uint32_t, uint64_t>;

    const WideType componentMax = static_cast<WideType>(std::numeric_limits<ComponentType>::max());
    const WideType pwmMax = static_cast<WideType>(_settings.pwmRange);

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      const char channelTag = ColorType::ChannelIndexIterator::channelAt(channel);
      const WideType component = static_cast<WideType>(lw::colors::applyBrightness(color[channelTag], brightness));
      WideType level = (component * pwmMax + (componentMax / 2U)) / componentMax;
      if (_settings.invert)
      {
        level = pwmMax - level;
      }

      analogWrite(pin, static_cast<uint16_t>(level));
    }
  }

private:
  LightDriverSettingsType _settings;
  bool _begun{false};
};

} // namespace lw::transports

#endif