#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "colors/palette/ModeEnums.h"
#include "colors/palette/Types.h"

namespace lw::colors::palettes
{
constexpr size_t absoluteDistance(size_t left, size_t right)
{
    return (left >= right) ? (left - right) : (right - left);
}

namespace detail
{
inline constexpr size_t PaletteDomainMinIndex = 0u;
inline constexpr size_t PaletteDomainMaxIndex = 255u;
inline constexpr size_t PaletteDomainSpan = PaletteDomainMaxIndex + 1u;

struct NormalizedSampleIndex
{
    size_t value{0};
    bool outOfRange{false};
    bool usesBoundarySampling{false};
    WrapMode wrapMode{WrapMode::Clamp};
    size_t domainMaxIndex{PaletteDomainMaxIndex};

    constexpr size_t wrapDistance(size_t stopIndex) const;
};

template <typename TColor> constexpr size_t normalizedDomainSampleCount(PaletteSampleOptions<TColor> options)
{
    return (options.scaledSampleCount == 0u) ? PaletteDomainSpan : options.scaledSampleCount;
}

template <typename TColor> constexpr size_t normalizedDomainMaxIndex(PaletteSampleOptions<TColor> options)
{
    return normalizedDomainSampleCount(options) - 1u;
}

inline constexpr size_t circularDistance(size_t left, size_t right, size_t maxIndex)
{
    const size_t direct = absoluteDistance(left, right);
    const size_t span = maxIndex + 1u;

    if (span == 0u)
    {
        return direct;
    }

    if (direct >= span)
    {
        return direct % span;
    }

    return std::min(direct, span - direct);
}

inline constexpr size_t NormalizedSampleIndex::wrapDistance(size_t stopIndex) const
{
    switch (wrapMode)
    {
        case WrapMode::Circular:
            return circularDistance(stopIndex, value, domainMaxIndex);
        case WrapMode::Clamp:
        case WrapMode::Mirror:
        case WrapMode::HoldFirst:
        case WrapMode::HoldLast:
        case WrapMode::Blackout:
        default:
            return absoluteDistance(stopIndex, value);
    }
}

inline constexpr size_t mirrorSampleIndex(size_t sampleIndex, size_t domainSampleCount, size_t domainMaxIndex)
{
    if (domainSampleCount <= 1u)
    {
        return 0u;
    }

    const size_t period = (domainSampleCount - 1u) * 2u;
    const size_t position = (period == 0u) ? 0u : (sampleIndex % period);
    return (position <= domainMaxIndex) ? position : (period - position);
}

template <typename TColor>
constexpr NormalizedSampleIndex normalizeForDomain(PaletteSampleOptions<TColor> options, size_t sampleIndex)
{
    const size_t domainSampleCount = normalizedDomainSampleCount(options);
    const size_t domainMaxIndex = normalizedDomainMaxIndex(options);

    switch (options.wrapMode)
    {
        case WrapMode::Blackout:
            return NormalizedSampleIndex{sampleIndex, sampleIndex > domainMaxIndex, true, options.wrapMode,
                                         domainMaxIndex};
        case WrapMode::Clamp:
            return NormalizedSampleIndex{(sampleIndex > domainMaxIndex) ? domainMaxIndex : sampleIndex, false, true,
                                         options.wrapMode, domainMaxIndex};
        case WrapMode::Circular:
            return NormalizedSampleIndex{sampleIndex % domainSampleCount, false, false, options.wrapMode,
                                         domainMaxIndex};
        case WrapMode::Mirror:
            return NormalizedSampleIndex{mirrorSampleIndex(sampleIndex, domainSampleCount, domainMaxIndex), false,
                                         false, options.wrapMode, domainMaxIndex};
        case WrapMode::HoldFirst:
            return NormalizedSampleIndex{(sampleIndex > domainMaxIndex) ? 0u : sampleIndex, false, true,
                                         options.wrapMode, domainMaxIndex};
        case WrapMode::HoldLast:
            return NormalizedSampleIndex{(sampleIndex > domainMaxIndex) ? domainMaxIndex : sampleIndex, false, true,
                                         options.wrapMode, domainMaxIndex};
        default:
            return NormalizedSampleIndex{sampleIndex, false, false, options.wrapMode, domainMaxIndex};
    }
}

template <typename TColor> TColor lowerBoundarySample(WrapMode wrapMode, span<const PaletteStop<TColor>> stops)
{
    return (wrapMode == WrapMode::HoldLast) ? stops.back().color : stops.front().color;
}

template <typename TColor> TColor upperBoundarySample(WrapMode wrapMode, span<const PaletteStop<TColor>> stops)
{
    switch (wrapMode)
    {
        case WrapMode::HoldFirst:
            return stops.front().color;
        case WrapMode::Blackout:
            return TColor{};
        case WrapMode::Clamp:
        case WrapMode::HoldLast:
        case WrapMode::Circular:
        case WrapMode::Mirror:
        default:
            return stops.back().color;
    }
}
} // namespace detail

} // namespace lw::colors::palettes
