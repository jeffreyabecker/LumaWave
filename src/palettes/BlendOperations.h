#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "palettes/ColorMath.h"
#include "palettes/ModeEnums.h"
#include "palettes/PaletteDomain.h"

namespace lw::palettes
{
lw::Pixel applyQuantizedBlend(const lw::Pixel& left, const lw::Pixel& right, uint8_t progress, uint8_t levels)
{
  using Component = lw::PixelComponent;
  const uint32_t clampedLevels = (levels < 2u) ? 2u : static_cast<uint32_t>(levels);
  constexpr uint32_t maxValue = static_cast<uint32_t>(std::numeric_limits<Component>::max());
  const uint32_t step = maxValue / (clampedLevels - 1u);

  lw::Pixel blended = lw::linearBlendProgress(left, right, progress);
  return lw::mapChannels(blended,
                         [&](auto v, char)
                         {
                           uint32_t quantized = ((static_cast<uint32_t>(v) + (step / 2u)) / step) * step;
                           if (quantized > maxValue)
                           {
                             quantized = maxValue;
                           }
                           return static_cast<Component>(quantized);
                         });
}
lw::Pixel applyBlendMode(BlendMode blendMode, const lw::Pixel& left, const lw::Pixel& right, uint8_t progress, size_t sampleIndex, uint8_t quantizedLevels = 8)
{
  switch (blendMode)
  {
    case BlendMode::Step:
      return left;
    case BlendMode::HoldMidpoint:
      return (progress < 128) ? left : right;
    case BlendMode::Smoothstep:
      return lw::linearBlendProgress(left, right, lw::smoothstep(progress));
    case BlendMode::Cubic:
      return lw::linearBlendProgress(left, right, lw::cubicEaseInOut(progress));
    case BlendMode::Cosine:
      return lw::linearBlendProgress(left, right, lw::cosineLike(progress));
    case BlendMode::GammaLinear:
    {
      using Component = lw::PixelComponent;
      return lw::mapChannels(left, right,
                             [&](auto lv, auto rv, char)
                             {
                               const uint32_t leftValue = static_cast<uint32_t>(lv);
                               const uint32_t rightValue = static_cast<uint32_t>(rv);
                               const uint32_t leftLinear = leftValue * leftValue;
                               const uint32_t rightLinear = rightValue * rightValue;
                               const uint32_t linear = leftLinear + ((rightLinear - leftLinear) * progress) / lw::palettes::detail::PaletteCanonicalFractionScale;
                               return static_cast<Component>(lw::integerSqrt(linear));
                             });
    }
    case BlendMode::Quantized:
      return applyQuantizedBlend(left, right, progress, quantizedLevels);
    case BlendMode::DitheredLinear:
    {
      using Component = lw::PixelComponent;
      constexpr uint32_t maxValue = static_cast<uint32_t>(std::numeric_limits<Component>::max());

      lw::Pixel out = lw::linearBlendProgress(left, right, progress);
      return lw::mapChannels(out,
                             [&](auto v, char tag)
                             {
                               uint32_t value = static_cast<uint32_t>(v);
                               const uint8_t channelOrdinal = (tag == 'R') ? 0 : (tag == 'G') ? 1 : (tag == 'B') ? 2 : 3;
                               const uint8_t noise = static_cast<uint8_t>((sampleIndex * 37u) + (channelOrdinal * 97u));
                               if (value < maxValue && noise < (progress & 0x3Fu))
                               {
                                 ++value;
                               }
                               return static_cast<Component>(value);
                             });
    }
    case BlendMode::Nearest:
    case BlendMode::Linear:
    default:
      return lw::linearBlendProgress(left, right, progress);
  }
}

} // namespace lw::palettes
