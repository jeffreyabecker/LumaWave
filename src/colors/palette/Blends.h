#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "colors/palette/BlendOperations.h"
#include "colors/palette/Detail.h"
#include "colors/palette/ModeEnums.h"
#include "colors/palette/Traits.h"
#include "colors/palette/WrapModes.h"

namespace lw::colors::palettes
{
namespace detail
{
  template <typename TColor> size_t firstStopAtOrAfter(span<const PaletteStop<TColor>> stops, palette_stop_index_t sampleIndex)
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

  template <typename TColor> size_t firstStopAtOrAfterFixed(span<const PaletteStop<TColor>> stops, palette_canonical_fixed_t sampleFixed)
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

  template <typename TColor> TColor sampleWrappedSpan(span<const PaletteStop<TColor>> stops, detail::PaletteCanonicalCoordinate sampleCoordinate, size_t blendSampleIndex, BlendMode blendMode, uint8_t quantizedLevels)
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
    const uint8_t progress = static_cast<uint8_t>((static_cast<uint64_t>(offset) * lw::colors::palettes::detail::PaletteCanonicalFractionScale) / spanWidth);
    return applyBlendMode<TColor>(blendMode, left.color, right.color, progress, blendSampleIndex, quantizedLevels);
  }
} // namespace detail

template <typename TColor, typename TIndexRange, typename TOutputRange,
          typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t sampleNearest(const IPalette<TColor>& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions<TColor> options);

template <typename TColor, typename TIndexRange, typename TOutputRange,
          typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t sampleInterpolated(const IPalette<TColor>& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions<TColor> options);

template <typename TColor, typename TOutputIt> void writeOutOfRangeSample(TOutputIt& output, PaletteSampleOptions<TColor> options)
{
  *output = detail::applyBrightnessScale(options.outOfRangeColor, options.brightnessScale);
}

template <typename TColor> TColor sampleNearestAt(span<const PaletteStop<TColor>> stops, size_t rawSampleIndex, PaletteSampleOptions<TColor> options)
{
  if (stops.empty())
  {
    return TColor{};
  }

  const detail::NormalizedSampleIndex normalized = detail::normalizeForDomain(options, rawSampleIndex);
  if (normalized.outOfRange)
  {
    return detail::applyBrightnessScale(options.outOfRangeColor, options.brightnessScale);
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

template <typename TColor> TColor sampleInterpolatedAt(span<const PaletteStop<TColor>> stops, size_t rawSampleIndex, PaletteSampleOptions<TColor> options)
{
  if (stops.empty())
  {
    return TColor{};
  }

  const detail::NormalizedSampleIndex normalized = detail::normalizeForDomain(options, rawSampleIndex);
  if (normalized.outOfRange)
  {
    return detail::applyBrightnessScale(options.outOfRangeColor, options.brightnessScale);
  }

  const palette_logical_index_t sampleIndex = normalized.value;
  const palette_canonical_fixed_t sampleFixed = normalized.canonical.fixed;
  TColor sampled{};

  const size_t stopIndex = detail::firstStopAtOrAfterFixed<TColor>(stops, sampleFixed);
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
      const uint8_t progress = static_cast<uint8_t>((static_cast<uint64_t>(offset) * lw::colors::palettes::detail::PaletteCanonicalFractionScale) / spanWidth);
      sampled = applyBlendMode<TColor>(options.blendMode, left.color, right.color, progress, sampleIndex, options.quantizedLevels);
    }
  }
  else if (normalized.usesBoundarySampling)
  {
    sampled = detail::upperBoundarySample<TColor>(options.wrapMode, stops);
  }
  else
  {
    sampled = detail::sampleWrappedSpan<TColor>(stops, normalized.canonical, sampleIndex, options.blendMode, options.quantizedLevels);
  }

  return detail::applyBrightnessScale(sampled, options.brightnessScale);
}

template <typename TColor> TColor samplePaletteAt(span<const PaletteStop<TColor>> stops, size_t rawSampleIndex, PaletteSampleOptions<TColor> options)
{
  if (options.blendMode == BlendMode::Nearest)
  {
    return sampleNearestAt<TColor>(stops, rawSampleIndex, options);
  }

  return sampleInterpolatedAt<TColor>(stops, rawSampleIndex, options);
}

template <typename TColor, typename TIndexRange, typename TOutputRange, typename>
size_t sampleNearest(const IPalette<TColor>& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions<TColor> options)
{
  const auto stops = palette.stops();
  auto index = paletteIndexes.begin();
  const auto indexEnd = paletteIndexes.end();
  auto output = outputColors.begin();
  const auto outputEnd = outputColors.end();

  size_t written = 0;
  for (; output != outputEnd && index != indexEnd; ++output, ++index)
  {
    *output = sampleNearestAt<TColor>(stops, static_cast<size_t>(*index), options);
    ++written;
  }

  return written;
}

template <typename TColor, typename TIndexRange, typename TOutputRange, typename>
size_t sampleInterpolated(const IPalette<TColor>& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions<TColor> options)
{
  const auto stops = palette.stops();
  auto index = paletteIndexes.begin();
  const auto indexEnd = paletteIndexes.end();
  auto output = outputColors.begin();
  const auto outputEnd = outputColors.end();

  size_t written = 0;
  for (; output != outputEnd && index != indexEnd; ++output, ++index)
  {
    *output = sampleInterpolatedAt<TColor>(stops, static_cast<size_t>(*index), options);
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

} // namespace lw::colors::palettes
