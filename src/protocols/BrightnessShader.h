#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

#include "IShader.h"
#include "core/Pixel.h"

namespace lw::protocols
{

class BrightnessShader : public IShader
{
public:
  using BrightnessType = lw::PixelComponent;

  void apply(span<const lw::Pixel> source, span<lw::Pixel> dest) override
  {
    if (_brightnessValue == std::numeric_limits<BrightnessType>::max())
    {
      return;
    }

    for (size_t i = 0; i < source.size(); ++i)
    {
      dest[i] = lw::mapChannels(source[i], [&](auto v, char) { return static_cast<lw::PixelComponent>(applyBrightness(v, _brightnessValue)); });
    }
  }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    if (type == RuntimeConfig::Brightness && value != nullptr)
    {
      _brightnessValue = *static_cast<BrightnessType*>(value);
    }
  }

private:
  template <typename TValue, typename TBrightness, typename = std::enable_if_t<std::is_integral_v<TValue> && std::is_unsigned_v<TValue> && std::is_integral_v<TBrightness> && std::is_unsigned_v<TBrightness>>>
  static constexpr TValue applyBrightness(TValue value, TBrightness brightness)
  {
    using ScaleWide = uint64_t;
    constexpr ScaleWide brightnessMax = static_cast<ScaleWide>(std::numeric_limits<TBrightness>::max());
    if constexpr (brightnessMax == 0)
      return static_cast<TValue>(0);
    return static_cast<TValue>(((static_cast<ScaleWide>(value) * static_cast<ScaleWide>(brightness)) + (brightnessMax / 2u)) / brightnessMax);
  }

  BrightnessType _brightnessValue{std::numeric_limits<BrightnessType>::max()};
};

} // namespace lw::protocols
