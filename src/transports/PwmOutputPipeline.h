#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "colors/ChannelMap.h"
#include "colors/ColorMath.h"
#include "core/OutputPipeline.h"

namespace lw::transports
{

struct PwmOutputPipelineSettings
{
  static constexpr size_t MaxChannels = 4;
  using PinsMap = ChannelMap<int>;
  static constexpr uint16_t DefaultPwmRange = 1023;
  static constexpr uint32_t DefaultPwmFrequencyHz = 1000;

  PinsMap pins{-1};
  uint16_t pwmRange{DefaultPwmRange};
  uint32_t pwmFrequencyHz{DefaultPwmFrequencyHz};
  bool invert{false};

  static PwmOutputPipelineSettings normalize(PwmOutputPipelineSettings settings)
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

class PwmOutputPipeline : public lw::buses::OutputPipeline
{
public:
  enum class PinMode : int
  {
    Input = 0,
    Output = 1
  };

  using ColorType = lw::Color;
  using BrightnessType = lw::colors::ColorComponent;
  using LightDriverSettingsType = PwmOutputPipelineSettings;

  explicit PwmOutputPipeline(LightDriverSettingsType settings) : _settings(LightDriverSettingsType::normalize(settings)) {}

  ~PwmOutputPipeline() override
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
        platformAnalogWrite(pin, 0);
        platformPinMode(pin, PinMode::Input);
      }
    }
  }

  void begin() override
  {
    if (_begun)
    {
      return;
    }

    platformAnalogWriteFreq(_settings.pwmFrequencyHz);
    platformAnalogWriteRange(_settings.pwmRange);

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      platformPinMode(pin, PinMode::Output);
      platformAnalogWrite(pin, 0);
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
    const WideType pwmMax = static_cast<WideType>(_settings.pwmRange);

    for (size_t channel = 0; channel < 4 && channel < _settings.pins.size(); ++channel)
    {
      const int pin = _settings.pins[channel];
      if (pin < 0)
      {
        continue;
      }

      const char channelTag = "RGBW"[channel];
      const WideType component = static_cast<WideType>(lw::colors::applyBrightness(color[channelTag], brightness));
      WideType level = (component * pwmMax + (componentMax / 2U)) / componentMax;
      if (_settings.invert)
      {
        level = pwmMax - level;
      }

      platformAnalogWrite(pin, static_cast<uint16_t>(level));
    }
  }

protected:
  virtual void platformAnalogWrite(int pin, uint16_t value) = 0;
  virtual void platformPinMode(int pin, PinMode mode) = 0;
  virtual void platformAnalogWriteFreq(uint32_t freq) = 0;
  virtual void platformAnalogWriteRange(uint16_t range) = 0;

private:
  LightDriverSettingsType _settings;
  bool _begun{false};
};

} // namespace lw::transports
