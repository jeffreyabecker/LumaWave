#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <cmath>
#include <algorithm>
#include <limits>

#include "Color.h"
#include "IShader.h"
#include "KelvinToRgbStrategies.h"

namespace lw::shaders
{

template <typename TColor, typename = std::enable_if_t<ColorChannelsAtLeast<TColor, 4>>>
struct AutoWhiteBalanceShaderSettings
{
    bool dualWhite = false;
    uint16_t whiteKelvin = 6500;
    uint16_t warmWhiteKelvin = 2700;
    uint16_t coolWhiteKelvin = 6500;
};

// White-balance and Kelvin-to-RGB correction logic adapted from WLED / WLED-MM.
// Source: https://github.com/MoonModules/WLED-MM
template <typename TColor, template <typename> class TKelvinToRgbStrategy = KelvinToRgbLut64Strategy,
          typename = std::enable_if_t<ColorChannelsAtLeast<TColor, 4>>>
class AutoWhiteBalanceShader : public IShader<TColor>
{
  public:
    using ColorType = TColor;
    using SettingsType = AutoWhiteBalanceShaderSettings<TColor>;
    using ComponentType = typename TColor::ComponentType;

    explicit AutoWhiteBalanceShader(SettingsType settings)
        : _dualWhite{settings.dualWhite},
          _warmCorrection{kelvinToRgbCorrection(settings.dualWhite ? settings.warmWhiteKelvin : settings.whiteKelvin)},
          _coolCorrection{settings.dualWhite ? kelvinToRgbCorrection(settings.coolWhiteKelvin) : _warmCorrection}
    {
    }

    void apply(span<TColor> colors) override
    {
        for (auto& color : colors)
        {
            uint16_t warmWeight = 255;
            uint16_t coolWeight = 0;

            if (_dualWhite)
            {
                uint16_t warm = color['W'];
                uint16_t cool = color['C'];
                uint16_t total = warm + cool;

                if (total > 0)
                {
                    warmWeight = static_cast<uint16_t>((warm * 255u) / total);
                    coolWeight = static_cast<uint16_t>(255u - warmWeight);
                }
                else
                {
                    warmWeight = 128;
                    coolWeight = 127;
                }
            }

            for (size_t channel = 0; channel < 3; ++channel)
            {
                const char channelTag = ChannelOrder::RGB::value[channel];
                uint16_t correction = _warmCorrection[channel];

                if (_dualWhite)
                {
                    correction = static_cast<uint16_t>(
                        (_warmCorrection[channel] * warmWeight + _coolCorrection[channel] * coolWeight + 127u) / 255u);
                }

                color[channelTag] = static_cast<ComponentType>(
                    (static_cast<uint64_t>(color[channelTag]) * correction + (MaxCorrection / 2u)) / MaxCorrection);
            }
        }
    }

  private:
    static constexpr uint16_t MinKelvin = 1200;
    static constexpr uint16_t MaxKelvin = 65000;
    static constexpr uint16_t MaxCorrection = static_cast<uint16_t>(std::numeric_limits<ComponentType>::max());
    using KelvinToRgbStrategy = TKelvinToRgbStrategy<ComponentType>;

    static std::array<ComponentType, 3> kelvinToRgbCorrection(uint16_t kelvin)
    {
        if (kelvin < MinKelvin || kelvin > MaxKelvin)
        {
            return {MaxCorrection, MaxCorrection, MaxCorrection};
        }

        return KelvinToRgbStrategy::convert(kelvin);
    }

    bool _dualWhite;
    std::array<ComponentType, 3> _warmCorrection;
    std::array<ComponentType, 3> _coolCorrection;
};

} // namespace lw::shaders

namespace lw
{

template <typename TColor, typename Enable = std::enable_if_t<ColorChannelsAtLeast<TColor, 4>>>
using AutoWhiteBalanceShaderSettings = shaders::AutoWhiteBalanceShaderSettings<TColor, Enable>;

template <typename TColor, template <typename> class TKelvinToRgbStrategy = KelvinToRgbLut64Strategy,
          typename Enable = std::enable_if_t<ColorChannelsAtLeast<TColor, 4>>>
using AutoWhiteBalanceShader = shaders::AutoWhiteBalanceShader<TColor, TKelvinToRgbStrategy, Enable>;

} // namespace lw
