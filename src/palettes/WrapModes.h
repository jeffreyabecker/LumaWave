#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "palettes/ModeEnums.h"
#include "palettes/Types.h"
#include "palettes/PaletteDomain.h"
namespace lw::palettes
{
namespace detail
{

  struct PaletteLogicalDomain
  {
    palette_logical_domain_count_t sampleCount{PaletteDomainSpan};

    constexpr palette_logical_index_t maxIndex() const { return (sampleCount == 0u) ? 0u : (sampleCount - 1u); }
  };

  struct PaletteCanonicalCoordinate
  {
    palette_canonical_fixed_t fixed{0};
  };

  struct NormalizedSampleIndex
  {
    palette_logical_index_t value{0};
    bool outOfRange{false};
    bool usesBoundarySampling{false};
    WrapMode wrapMode{WrapMode::Clamp};
    palette_logical_index_t domainMaxIndex{PaletteDomainMaxIndex};
    PaletteLogicalDomain logicalDomain{};
    PaletteCanonicalCoordinate canonical{};

    constexpr size_t wrapDistance(palette_stop_index_t stopIndex) const;
  };

  inline constexpr palette_canonical_fixed_t canonicalStopFixed(palette_stop_index_t stopIndex)
  {
    return static_cast<palette_canonical_fixed_t>(stopIndex * PaletteCanonicalFractionScale);
  }

  constexpr palette_logical_domain_count_t normalizedDomainSampleCount(PaletteSampleOptions options)
  {
    return (options.scaledSampleCount == 0u) ? PaletteDomainSpan : options.scaledSampleCount;
  }

  inline constexpr size_t NormalizedSampleIndex::wrapDistance(palette_stop_index_t stopIndex) const
  {
    const size_t stopFixed = static_cast<size_t>(canonicalStopFixed(stopIndex));
    const size_t canonicalFixed = static_cast<size_t>(canonical.fixed);
    const size_t direct = (stopFixed >= canonicalFixed) ? (stopFixed - canonicalFixed) : (canonicalFixed - stopFixed);

    switch (wrapMode)
    {
      case WrapMode::Circular:
        return std::min(direct, static_cast<size_t>(PaletteCanonicalWrapSpan) - direct);
      case WrapMode::Clamp:
      case WrapMode::Mirror:
      case WrapMode::HoldFirst:
      case WrapMode::HoldLast:
      case WrapMode::Blackout:
      default:
        return direct;
    }
  }

  inline constexpr PaletteCanonicalCoordinate mapLogicalIndexToCanonicalCoordinate(palette_logical_index_t logicalIndex, PaletteLogicalDomain logicalDomain)
  {
    if (logicalDomain.maxIndex() == 0u)
    {
      return PaletteCanonicalCoordinate{};
    }

    const uint64_t numerator = static_cast<uint64_t>(logicalIndex) * static_cast<uint64_t>(PaletteCanonicalMaxFixed);
    return PaletteCanonicalCoordinate{static_cast<palette_canonical_fixed_t>(numerator / logicalDomain.maxIndex())};
  }

  inline constexpr PaletteCanonicalCoordinate mapCircularLogicalIndexToCanonicalCoordinate(palette_logical_index_t logicalIndex, PaletteLogicalDomain logicalDomain)
  {
    if (logicalDomain.sampleCount == 0u)
    {
      return PaletteCanonicalCoordinate{};
    }

    const uint64_t numerator = static_cast<uint64_t>(logicalIndex) * static_cast<uint64_t>(PaletteDomainMaxIndex);
    return PaletteCanonicalCoordinate{canonicalStopFixed(static_cast<palette_stop_index_t>(numerator / logicalDomain.sampleCount))};
  }

  inline constexpr palette_logical_index_t mirrorSampleIndex(palette_logical_index_t sampleIndex, palette_logical_domain_count_t domainSampleCount, palette_logical_index_t domainMaxIndex)
  {
    if (domainSampleCount <= 1u)
    {
      return 0u;
    }

    const palette_logical_index_t period = (domainSampleCount - 1u) * 2u;
    const palette_logical_index_t position = (period == 0u) ? 0u : (sampleIndex % period);
    return (position <= domainMaxIndex) ? position : (period - position);
  }

  constexpr NormalizedSampleIndex normalizeForDomain(PaletteSampleOptions options, palette_logical_index_t sampleIndex)
  {
    const PaletteLogicalDomain logicalDomain{normalizedDomainSampleCount(options)};
    const palette_logical_domain_count_t domainSampleCount = logicalDomain.sampleCount;
    const palette_logical_index_t domainMaxIndex = logicalDomain.maxIndex();

    palette_logical_index_t normalizedValue = sampleIndex;
    bool outOfRange = false;
    bool usesBoundarySampling = false;

    switch (options.wrapMode)
    {
      case WrapMode::Blackout:
        normalizedValue = sampleIndex;
        outOfRange = sampleIndex > domainMaxIndex;
        usesBoundarySampling = true;
        break;
      case WrapMode::Clamp:
        normalizedValue = (sampleIndex > domainMaxIndex) ? domainMaxIndex : sampleIndex;
        usesBoundarySampling = true;
        break;
      case WrapMode::Circular:
        normalizedValue = sampleIndex % domainSampleCount;
        break;
      case WrapMode::Mirror:
        normalizedValue = mirrorSampleIndex(sampleIndex, domainSampleCount, domainMaxIndex);
        break;
      case WrapMode::HoldFirst:
        normalizedValue = (sampleIndex > domainMaxIndex) ? 0u : sampleIndex;
        usesBoundarySampling = true;
        break;
      case WrapMode::HoldLast:
        normalizedValue = (sampleIndex > domainMaxIndex) ? domainMaxIndex : sampleIndex;
        usesBoundarySampling = true;
        break;
      default:
        break;
    }

    const palette_logical_index_t coordinateIndex = (outOfRange && sampleIndex > domainMaxIndex) ? domainMaxIndex : normalizedValue;

    const PaletteCanonicalCoordinate canonical = (options.wrapMode == WrapMode::Circular && logicalDomain.sampleCount > PaletteDomainSpan) ? mapCircularLogicalIndexToCanonicalCoordinate(coordinateIndex, logicalDomain)
                                                                                                                                           : mapLogicalIndexToCanonicalCoordinate(coordinateIndex, logicalDomain);

    return NormalizedSampleIndex{normalizedValue, outOfRange, usesBoundarySampling, options.wrapMode, domainMaxIndex, logicalDomain, canonical};
  }

  lw::Pixel upperBoundarySample(WrapMode wrapMode, span<const PaletteStop> stops)
  {
    switch (wrapMode)
    {
      case WrapMode::HoldFirst:
        return stops.front().color;
      case WrapMode::Blackout:
        return lw::Pixel{};
      case WrapMode::Clamp:
      case WrapMode::HoldLast:
      case WrapMode::Circular:
      case WrapMode::Mirror:
      default:
        return stops.back().color;
    }
  }
} // namespace detail

} // namespace lw::palettes
