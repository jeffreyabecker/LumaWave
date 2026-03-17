#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "colors/ColorMath.h"
#include "colors/palette/ModeEnums.h"

namespace lw::colors::palettes
{
template <typename TColor> TColor applyQuantizedBlend(const TColor& left, const TColor& right, uint8_t progress, uint8_t levels)
{
  using Component = typename TColor::ComponentType;
  const uint32_t clampedLevels = (levels < 2u) ? 2u : static_cast<uint32_t>(levels);
  constexpr uint32_t maxValue = static_cast<uint32_t>(std::numeric_limits<Component>::max());
  const uint32_t step = maxValue / (clampedLevels - 1u);

  TColor out = lw::colors::linearBlendProgress8(left, right, progress);
  for (char channel : TColor::channelIndexes())
  {
    const uint32_t value = static_cast<uint32_t>(out[channel]);
    uint32_t quantized = ((value + (step / 2u)) / step) * step;
    if (quantized > maxValue)
    {
      quantized = maxValue;
    }

    out[channel] = static_cast<Component>(quantized);
  }

  return out;
}
template <typename TColor> TColor applyBlendMode(BlendMode blendMode, const TColor& left, const TColor& right, uint8_t progress, size_t sampleIndex, uint8_t quantizedLevels = 8)
{
  switch (blendMode)
  {
    case BlendMode::Step:
      return left;
    case BlendMode::HoldMidpoint:
      return (progress < 128) ? left : right;
    case BlendMode::Smoothstep:
      return lw::colors::linearBlendProgress8(left, right, lw::colors::smoothstep8<TColor>(progress));
    case BlendMode::Cubic:
      return lw::colors::linearBlendProgress8(left, right, lw::colors::cubicEaseInOut8<TColor>(progress));
    case BlendMode::Cosine:
      return lw::colors::linearBlendProgress8(left, right, lw::colors::cosineLike8<TColor>(progress));
    case BlendMode::GammaLinear:
    {
      using Component = typename TColor::ComponentType;
      TColor out{};

      for (char channel : TColor::channelIndexes())
      {
        const uint32_t leftValue = static_cast<uint32_t>(left[channel]);
        const uint32_t rightValue = static_cast<uint32_t>(right[channel]);
        const uint32_t leftLinear = leftValue * leftValue;
        const uint32_t rightLinear = rightValue * rightValue;

        const uint32_t linear = leftLinear + ((rightLinear - leftLinear) * progress) / lw::colors::palettes::detail::PaletteCanonicalFractionScale;
        const uint32_t gamma = lw::colors::integerSqrt<TColor>(linear);
        out[channel] = static_cast<Component>(gamma);
      }

      return out;
    }
    case BlendMode::Quantized:
      return applyQuantizedBlend<TColor>(left, right, progress, quantizedLevels);
    case BlendMode::DitheredLinear:
    {
      using Component = typename TColor::ComponentType;
      constexpr uint32_t maxValue = static_cast<uint32_t>(std::numeric_limits<Component>::max());

      TColor out = lw::colors::linearBlendProgress8(left, right, progress);
      uint8_t channelOrdinal = 0;
      for (char channel : TColor::channelIndexes())
      {
        uint32_t value = static_cast<uint32_t>(out[channel]);
        const uint8_t noise = static_cast<uint8_t>((sampleIndex * 37u) + (channelOrdinal * 97u));
        if (value < maxValue && noise < (progress & 0x3Fu))
        {
          ++value;
        }

        out[channel] = static_cast<Component>(value);
        ++channelOrdinal;
      }

      return out;
    }
    case BlendMode::Nearest:
    case BlendMode::Linear:
    default:
      return lw::colors::linearBlendProgress8(left, right, progress);
  }
}

} // namespace lw::colors::palettes
