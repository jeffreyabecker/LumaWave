#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "palettes/BlendOperations.h"
#include "palettes/Detail.h"
#include "palettes/ModeEnums.h"
#include "palettes/Traits.h"
#include "palettes/WrapModes.h"

namespace lw::palettes
{
namespace detail
{
  inline constexpr uint8_t canonicalProgress8(palette_canonical_fixed_t offset, palette_canonical_fixed_t spanWidth)
  {
    if (spanWidth == 0u)
    {
      return 255u;
    }

    if (offset >= spanWidth)
    {
      return 255u;
    }

    return static_cast<uint8_t>((static_cast<uint64_t>(offset) * lw::palettes::detail::PaletteCanonicalFractionScale) / spanWidth);
  }

  size_t firstStopAtOrAfter(span<const PaletteStop> stops, palette_stop_index_t sampleIndex)
  {
    size_t left = 1;
    size_t right = static_cast<size_t>(stops.size());

    while (left < right)
    {
      const size_t mid = left + ((right - left) / 2u);
      if (stops[mid].index < sampleIndex)
      {
        left = mid + 1u;
      }
      else
      {
        right = mid;
      }
    }

    return left;
  }

  size_t firstStopAtOrAfterFixed(span<const PaletteStop> stops, palette_canonical_fixed_t sampleFixed)
  {
    size_t left = 1;
    size_t right = static_cast<size_t>(stops.size());

    while (left < right)
    {
      const size_t mid = left + ((right - left) / 2u);
      if (detail::canonicalStopFixed(stops[mid].index) < sampleFixed)
      {
        left = mid + 1u;
      }
      else
      {
        right = mid;
      }
    }

    return left;
  }

  inline constexpr bool pickNearestTieCandidate(TieBreakPolicy tieBreakPolicy, size_t candidateIndex, size_t currentIndex)
  {
    switch (tieBreakPolicy)
    {
      case TieBreakPolicy::Left:
        return candidateIndex < currentIndex;
      case TieBreakPolicy::Right:
        return candidateIndex > currentIndex;
      case TieBreakPolicy::Stable:
      default:
        return false;
    }
  }

  lw::Pixel sampleWrappedSpan(span<const PaletteStop> stops, detail::PaletteCanonicalCoordinate sampleCoordinate, size_t blendSampleIndex, BlendMode blendMode, uint8_t quantizedLevels)
  {
    const auto& left = stops.back();
    const auto& right = stops.front();
    const palette_canonical_fixed_t wrapPeriod = detail::PaletteCanonicalWrapSpan;
    const palette_canonical_fixed_t leftIndex = detail::canonicalStopFixed(left.index);
    const palette_canonical_fixed_t rightIndex = detail::canonicalStopFixed(right.index) + wrapPeriod;
    const palette_canonical_fixed_t wrappedSampleIndex = (sampleCoordinate.fixed >= leftIndex) ? sampleCoordinate.fixed : (sampleCoordinate.fixed + wrapPeriod);
    const palette_canonical_fixed_t spanWidth = rightIndex - leftIndex;

    if (spanWidth == 0)
    {
      return left.color;
    }

    const palette_canonical_fixed_t offset = wrappedSampleIndex - leftIndex;
    const uint8_t progress = detail::canonicalProgress8(offset, spanWidth);
    return applyBlendMode(blendMode, left.color, right.color, progress, blendSampleIndex, quantizedLevels);
  }
} // namespace detail

template <typename TIndexRange, typename TOutputRange, typename = std::enable_if_t<true && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t sampleNearest(const IPalette& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions options);

template <typename TIndexRange, typename TOutputRange, typename = std::enable_if_t<true && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t sampleInterpolated(const IPalette& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions options);

template <typename TOutputIt> void writeOutOfRangeSample(TOutputIt& output, PaletteSampleOptions options)
{
  *output = detail::applyBrightnessScale(options.outOfRangePixel, options.brightnessScale);
}

lw::Pixel sampleNearestAt(span<const PaletteStop> stops, size_t rawSampleIndex, PaletteSampleOptions options)
{
  if (stops.empty())
  {
    return lw::Pixel{};
  }

  const detail::NormalizedSampleIndex normalized = detail::normalizeForDomain(options, rawSampleIndex);
  if (normalized.outOfRange)
  {
    return detail::applyBrightnessScale(options.outOfRangePixel, options.brightnessScale);
  }

  size_t nearestStopIndex = 0;
  size_t nearestDistance = std::numeric_limits<size_t>::max();

  for (size_t stopIndex = 0; stopIndex < stops.size(); ++stopIndex)
  {
    const size_t distance = normalized.wrapDistance(stops[stopIndex].index);

    if (distance < nearestDistance)
    {
      nearestDistance = distance;
      nearestStopIndex = stopIndex;
    }
    else if (distance == nearestDistance && detail::pickNearestTieCandidate(options.tieBreakPolicy, stopIndex, nearestStopIndex))
    {
      nearestStopIndex = stopIndex;
    }
  }

  return detail::applyBrightnessScale(stops[nearestStopIndex].color, options.brightnessScale);
}

lw::Pixel sampleInterpolatedAt(span<const PaletteStop> stops, size_t rawSampleIndex, PaletteSampleOptions options)
{
  if (stops.empty())
  {
    return lw::Pixel{};
  }

  const detail::NormalizedSampleIndex normalized = detail::normalizeForDomain(options, rawSampleIndex);
  if (normalized.outOfRange)
  {
    return detail::applyBrightnessScale(options.outOfRangePixel, options.brightnessScale);
  }

  const palette_logical_index_t sampleIndex = normalized.value;
  const palette_canonical_fixed_t sampleFixed = normalized.canonical.fixed;
  lw::Pixel sampled{};

  const size_t stopIndex = detail::firstStopAtOrAfterFixed(stops, sampleFixed);
  if (stopIndex < static_cast<size_t>(stops.size()))
  {
    const auto& left = stops[stopIndex - 1];
    const auto& right = stops[stopIndex];
    const palette_canonical_fixed_t leftFixed = detail::canonicalStopFixed(left.index);
    const palette_canonical_fixed_t rightFixed = detail::canonicalStopFixed(right.index);
    const palette_canonical_fixed_t spanWidth = rightFixed - leftFixed;

    if (rawSampleIndex != sampleIndex && sampleFixed == rightFixed)
    {
      sampled = right.color;
    }
    else if (spanWidth == 0)
    {
      sampled = right.color;
    }
    else
    {
      const palette_canonical_fixed_t offset = sampleFixed - leftFixed;
      const uint8_t progress = detail::canonicalProgress8(offset, spanWidth);
      sampled = applyBlendMode(options.blendMode, left.color, right.color, progress, sampleIndex, options.quantizedLevels);
    }
  }
  else if (normalized.usesBoundarySampling)
  {
    sampled = detail::upperBoundarySample(options.wrapMode, stops);
  }
  else
  {
    sampled = detail::sampleWrappedSpan(stops, normalized.canonical, sampleIndex, options.blendMode, options.quantizedLevels);
  }

  return detail::applyBrightnessScale(sampled, options.brightnessScale);
}

lw::Pixel samplePaletteAt(span<const PaletteStop> stops, size_t rawSampleIndex, PaletteSampleOptions options)
{
  if (options.blendMode == BlendMode::Nearest)
  {
    return sampleNearestAt(stops, rawSampleIndex, options);
  }

  return sampleInterpolatedAt(stops, rawSampleIndex, options);
}

template <typename TIndexRange, typename TOutputRange, typename> size_t sampleNearest(const IPalette& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions options)
{
  const auto stops = palette.stops();
  auto index = paletteIndexes.begin();
  const auto indexEnd = paletteIndexes.end();
  auto output = outputColors.begin();
  const auto outputEnd = outputColors.end();

  size_t written = 0;
  for (; output != outputEnd && index != indexEnd; ++output, ++index)
  {
    *output = sampleNearestAt(stops, static_cast<size_t>(*index), options);
    ++written;
  }

  return written;
}

template <typename TIndexRange, typename TOutputRange, typename> size_t sampleInterpolated(const IPalette& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions options)
{
  const auto stops = palette.stops();
  auto index = paletteIndexes.begin();
  const auto indexEnd = paletteIndexes.end();
  auto output = outputColors.begin();
  const auto outputEnd = outputColors.end();

  size_t written = 0;
  for (; output != outputEnd && index != indexEnd; ++output, ++index)
  {
    *output = sampleInterpolatedAt(stops, static_cast<size_t>(*index), options);
    ++written;
  }

  return written;
}

namespace blend
{
  inline constexpr BlendMode Linear = BlendMode::Linear;
  inline constexpr BlendMode Nearest = BlendMode::Nearest;
  inline constexpr BlendMode Step = BlendMode::Step;
  inline constexpr BlendMode HoldMidpoint = BlendMode::HoldMidpoint;
  inline constexpr BlendMode Midpoint = BlendMode::HoldMidpoint;
  inline constexpr BlendMode MidPoint = BlendMode::HoldMidpoint;
  inline constexpr BlendMode Smoothstep = BlendMode::Smoothstep;
  inline constexpr BlendMode Cubic = BlendMode::Cubic;
  inline constexpr BlendMode Cosine = BlendMode::Cosine;
  inline constexpr BlendMode GammaLinear = BlendMode::GammaLinear;
  inline constexpr BlendMode Quantized = BlendMode::Quantized;
  inline constexpr BlendMode DitheredLinear = BlendMode::DitheredLinear;
} // namespace blend

} // namespace lw::palettes
