#pragma once

#include <cstdint>
#include <limits>

#include "IShader.h"
#include "core/Color.h"
#include "palettes/ColorMath.h"

namespace lw::protocols
{

class BrightnessShader : public IShader
{
public:
  using BrightnessType = lw::ColorComponent;

  void apply(span<const lw::Color> source, span<lw::Color> dest) override
  {
    if (_brightnessValue == std::numeric_limits<BrightnessType>::max())
    {
      return;
    }

    for (size_t i = 0; i < source.size(); ++i)
    {
      dest[i] = source[i];

      for (char channel : {'R', 'G', 'B', 'W'})
      {
        lw::setColorComponentByTag(dest[i], channel, static_cast<lw::ColorComponent>(lw::applyBrightness(lw::colorComponentByTag(source[i], channel), _brightnessValue)));
      }
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
  BrightnessType _brightnessValue{std::numeric_limits<BrightnessType>::max()};
};

} // namespace lw::protocols
