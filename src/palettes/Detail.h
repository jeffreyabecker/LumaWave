#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "palettes/Types.h"

namespace lw::palettes
{
namespace detail
{
  lw::Color applyBrightnessScale(lw::Color color, lw::ColorComponent brightnessScale)
  {
    using Component = lw::ColorComponent;
    constexpr uint32_t MaxComponent = static_cast<uint32_t>(std::numeric_limits<Component>::max());
    const uint32_t scale = static_cast<uint32_t>(brightnessScale);

    if (scale >= MaxComponent)
    {
      return color;
    }

    return lw::mapChannels(color, [&](auto v, char) { return static_cast<Component>((static_cast<uint32_t>(v) * scale) / MaxComponent); });
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

  template <typename TOutputIt, typename TSentinel, typename = void> size_t writeScaledSolid(lw::Color color, lw::ColorComponent brightnessScale, TOutputIt output, TSentinel outputEnd)
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

} // namespace lw::palettes
