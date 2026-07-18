#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "colors/palette/Types.h"

namespace lw::colors::palettes
{
namespace detail
{
  lw::Color applyBrightnessScale(lw::Color color, lw::colors::ColorComponent brightnessScale)
  {
    using Component = lw::colors::ColorComponent;
    constexpr uint32_t MaxComponent = static_cast<uint32_t>(std::numeric_limits<Component>::max());
    const uint32_t scale = static_cast<uint32_t>(brightnessScale);

    if (scale >= MaxComponent)
    {
      return color;
    }

    for (char channel : {'R', 'G', 'B', 'W'})
    {
      const uint32_t value = static_cast<uint32_t>(lw::colorComponentByTag(color, channel));
      lw::setColorComponentByTag(color, channel, static_cast<Component>((value * scale) / MaxComponent));
    }

    return color;
  }

  template <typename TOutputIt, typename TSentinel, typename = void> size_t writeZeroed(TOutputIt output, TSentinel outputEnd)
  {
    size_t written = 0;
    for (; output != outputEnd; ++output)
    {
      *output = lw::Color{};
      ++written;
    }

    return written;
  }

  template <typename TOutputIt, typename TSentinel, typename = void> size_t writeScaledSolid(lw::Color color, lw::colors::ColorComponent brightnessScale, TOutputIt output, TSentinel outputEnd)
  {
    const lw::Color scaled = applyBrightnessScale(color, brightnessScale);
    size_t written = 0;
    for (; output != outputEnd; ++output)
    {
      *output = scaled;
      ++written;
    }

    return written;
  }
} // namespace detail

} // namespace lw::colors::palettes
