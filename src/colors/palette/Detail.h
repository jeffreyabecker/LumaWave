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
  template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> TColor applyBrightnessScale(TColor color, typename TColor::ComponentType brightnessScale)
  {
    using Component = typename TColor::ComponentType;
    constexpr uint32_t MaxComponent = static_cast<uint32_t>(std::numeric_limits<Component>::max());
    const uint32_t scale = static_cast<uint32_t>(brightnessScale);

    if (scale >= MaxComponent)
    {
      return color;
    }

    for (char channel : {'R', 'G', 'B', 'W'})
    {
      const uint32_t value = static_cast<uint32_t>(color[channel]);
      color[channel] = static_cast<Component>((value * scale) / MaxComponent);
    }

    return color;
  }

  template <typename TColor, typename TOutputIt, typename TSentinel, typename = std::enable_if_t<ColorType<TColor>>> size_t writeZeroed(TOutputIt output, TSentinel outputEnd)
  {
    size_t written = 0;
    for (; output != outputEnd; ++output)
    {
      *output = TColor{};
      ++written;
    }

    return written;
  }

  template <typename TColor, typename TOutputIt, typename TSentinel, typename = std::enable_if_t<ColorType<TColor>>>
  size_t writeScaledSolid(TColor color, typename TColor::ComponentType brightnessScale, TOutputIt output, TSentinel outputEnd)
  {
    const TColor scaled = applyBrightnessScale(color, brightnessScale);
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
