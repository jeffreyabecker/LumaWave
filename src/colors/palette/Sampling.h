#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

#include "core/IndexIterator.h"
#include "colors/palette/Blends.h"
#include "colors/palette/SamplingTransition.h"
#include "colors/palette/Traits.h"

namespace lw::colors::palettes
{
namespace detail
{
template <typename TRange>
using SampleRangeStorageType =
    std::conditional_t<std::is_lvalue_reference<TRange>::value, TRange, std::remove_reference_t<TRange>>;

template <typename TRange, typename = void> struct HasSizeMethod : std::false_type
{
};

template <typename TRange>
struct HasSizeMethod<TRange, std::void_t<decltype(std::declval<const TRange&>().size())>> : std::true_type
{
};

template <typename TRange> size_t countRangeElements(TRange&& range)
{
    if constexpr (HasSizeMethod<std::remove_reference_t<TRange>>::value)
    {
        return static_cast<size_t>(range.size());
    }

    size_t count = 0;
    for (auto it = range.begin(); it != range.end(); ++it)
    {
        ++count;
    }

    return count;
}
} // namespace detail

template <typename TColor, typename TSampler, typename TIndexIt, typename = std::enable_if_t<ColorType<TColor>>>
class PaletteSampleIterator
{
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = TColor;
    using difference_type = std::ptrdiff_t;
    using reference = TColor;
    using pointer = void;

    PaletteSampleIterator(const TSampler* sampler, TIndexIt current, const void* ownerIdentity, size_t remaining)
        : _sampler(sampler), _current(current), _ownerIdentity(ownerIdentity), _remaining(remaining)
    {
    }

    reference operator*() const { return (*_sampler)(static_cast<size_t>(*_current)); }

    PaletteSampleIterator& operator++()
    {
        if (_remaining > 0)
        {
            ++_current;
            --_remaining;
        }

        return *this;
    }

    PaletteSampleIterator operator++(int)
    {
        PaletteSampleIterator copy = *this;
        ++(*this);
        return copy;
    }

    friend bool operator==(const PaletteSampleIterator& left, const PaletteSampleIterator& right)
    {
        return left._ownerIdentity == right._ownerIdentity && left._remaining == right._remaining;
    }

    friend bool operator!=(const PaletteSampleIterator& left, const PaletteSampleIterator& right)
    {
        return !(left == right);
    }

  private:
    const TSampler* _sampler;
    TIndexIt _current;
    const void* _ownerIdentity;
    size_t _remaining;
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> class SinglePaletteSampler
{
  public:
    SinglePaletteSampler(const IPalette<TColor>& palette, PaletteSampleOptions<TColor> options, size_t offset = 0)
        : _palette(palette), _options(options), _offset(offset)
    {
    }

    TColor operator()(size_t paletteIndex) const
    {
        return samplePaletteAt<TColor>(_palette.stops(), paletteIndex + _offset, _options);
    }

  private:
    const IPalette<TColor>& _palette;
    PaletteSampleOptions<TColor> _options;
    size_t _offset;
};

template <typename TValue> class ConstantSampler
{
  public:
    explicit constexpr ConstantSampler(TValue value) : _value(value) {}

    constexpr TValue operator()(size_t) const { return _value; }

  private:
    TValue _value;
};

template <typename TColor, typename TSamplerFrom, typename TSamplerTo, typename TBlendProgressSampler,
          typename TBlendDomain = uint8_t, typename = std::enable_if_t<ColorType<TColor>>>
class TransitionSampler
{
  public:
    TransitionSampler(TSamplerFrom sampleFrom, TSamplerTo sampleTo, TBlendProgressSampler sampleProgress)
        : _sampleFrom(std::move(sampleFrom)), _sampleTo(std::move(sampleTo)), _sampleProgress(std::move(sampleProgress))
    {
    }

    TColor operator()(size_t paletteIndex) const
    {
        const TColor from = _sampleFrom(paletteIndex);
        const TColor to = _sampleTo(paletteIndex);
        const TBlendDomain progress = static_cast<TBlendDomain>(_sampleProgress(paletteIndex));
        if constexpr (std::is_same_v<TBlendDomain, uint8_t>)
        {
            return lw::colors::linearBlendProgress8(from, to, progress);
        }

        const uint8_t blendProgress8 = lw::colors::scaleComponent<uint8_t>(progress);
        return lw::colors::linearBlendProgress8(from, to, blendProgress8);
    }

  private:
    TSamplerFrom _sampleFrom;
    TSamplerTo _sampleTo;
    TBlendProgressSampler _sampleProgress;
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> class TransitionPaletteSampler
{
  public:
    using SamplerType = TransitionSampler<TColor, SinglePaletteSampler<TColor>, SinglePaletteSampler<TColor>,
                                          ConstantSampler<uint8_t>, uint8_t>;

    TransitionPaletteSampler(const IPalette<TColor>& paletteFrom, const IPalette<TColor>& paletteTo,
                             uint8_t blendProgress8, PaletteSampleOptions<TColor> options)
        : _sampler(SinglePaletteSampler<TColor>(paletteFrom, options), SinglePaletteSampler<TColor>(paletteTo, options),
                   ConstantSampler<uint8_t>(blendProgress8))
    {
    }

    TColor operator()(size_t paletteIndex) const { return _sampler(paletteIndex); }

  private:
    SamplerType _sampler;
};

template <typename TColor, typename TSampler, typename TStoredIndexRange,
          typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<TStoredIndexRange>::value>>
class PaletteSampleRangeT
{
  public:
    using Iterator = PaletteSampleIterator<TColor, TSampler, decltype(std::declval<TStoredIndexRange&>().begin())>;

    PaletteSampleRangeT(TSampler sampler, TStoredIndexRange paletteIndexes)
        : _sampler(std::move(sampler)), _paletteIndexes(paletteIndexes),
          _count(detail::countRangeElements(_paletteIndexes))
    {
    }

    Iterator begin() const { return Iterator(&_sampler, _paletteIndexes.begin(), this, _count); }

    Iterator end() const { return Iterator(&_sampler, _paletteIndexes.begin(), this, 0); }

    size_t size() const { return _count; }

  private:
    TSampler _sampler;
    TStoredIndexRange _paletteIndexes;
    size_t _count;
};

template <typename TSampleRange, typename TOutputRange,
          std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TSampleRange>>::value &&
                               IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value,
                           int> = 0>
size_t writeSampleRange(TSampleRange&& sampleRange, TOutputRange&& outputColors)
{
    auto sample = sampleRange.begin();
    const auto sampleEnd = sampleRange.end();
    auto output = outputColors.begin();
    const auto outputEnd = outputColors.end();

    size_t written = 0;
    for (; sample != sampleEnd && output != outputEnd; ++sample, ++output)
    {
        *output = *sample;
        ++written;
    }

    return written;
}

template <typename TSampler, typename TOutputRange,
          std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value &&
                               !IsBeginEndRange<std::remove_reference_t<TSampler>>::value &&
                               std::is_invocable_v<TSampler&, size_t>,
                           int> = 0>
size_t writeSampleRange(TSampler&& sampler, TOutputRange&& outputColors)
{
    auto output = outputColors.begin();
    const auto outputEnd = outputColors.end();

    size_t paletteIndex = 0;
    size_t written = 0;
    for (; output != outputEnd; ++output, ++paletteIndex)
    {
        *output = sampler(paletteIndex);
        ++written;
    }

    return written;
}

template <
    typename TColor, typename TIndexRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteSamples(const IPalette<TColor>& palette, TIndexRange&& paletteIndexes,
                    PaletteSampleOptions<TColor> options = {})
{
    using StoredIndexRange = detail::SampleRangeStorageType<TIndexRange>;
    using Sampler = SinglePaletteSampler<TColor>;
    using Range = PaletteSampleRangeT<TColor, Sampler, StoredIndexRange>;

    return Range(Sampler(palette, options), std::forward<TIndexRange>(paletteIndexes));
}

template <
    typename TColor, typename TIndexRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette<TColor>& paletteFrom, const IPalette<TColor>& paletteTo,
                              TIndexRange&& paletteIndexes, uint8_t blendProgress8,
                              PaletteSampleOptions<TColor> options = {})
{
    using StoredIndexRange = detail::SampleRangeStorageType<TIndexRange>;
    using Sampler = TransitionPaletteSampler<TColor>;
    using Range = PaletteSampleRangeT<TColor, Sampler, StoredIndexRange>;

    return Range(Sampler(paletteFrom, paletteTo, blendProgress8, options), std::forward<TIndexRange>(paletteIndexes));
}

template <
    typename TColor, typename TIndexRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette<TColor>& paletteFrom, const IPalette<TColor>& paletteTo,
                              TIndexRange&& paletteIndexes, uint8_t transitionProgress, uint8_t transitionDuration,
                              PaletteSampleOptions<TColor> options = {})
{
    return paletteTransitionSamples(paletteFrom, paletteTo, std::forward<TIndexRange>(paletteIndexes),
                                    mapTransitionProgressToBlend8(transitionProgress, transitionDuration), options);
}

template <
    typename TColor, typename TIndexRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteSamples(const IPalette<TColor>&&, TIndexRange&&, PaletteSampleOptions<TColor> = {}) = delete;

template <
    typename TColor, typename TIndexRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette<TColor>&&, const IPalette<TColor>&, TIndexRange&&, uint8_t,
                              PaletteSampleOptions<TColor> = {}) = delete;

template <
    typename TColor, typename TIndexRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette<TColor>&, const IPalette<TColor>&&, TIndexRange&&, uint8_t,
                              PaletteSampleOptions<TColor> = {}) = delete;

template <
    typename TColor, typename TIndexRange, typename TOutputRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value &&
                                IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette<TColor>& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors,
                     PaletteSampleOptions<TColor> options = {})
{
    return writeSampleRange(paletteSamples(palette, std::forward<TIndexRange>(paletteIndexes), options),
                            std::forward<TOutputRange>(outputColors));
}

template <
    typename TColor, typename TOutputRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette<TColor>& palette, size_t paletteIndex, TOutputRange&& outputColors,
                     PaletteSampleOptions<TColor> options = {})
{
    const size_t outputCount = static_cast<size_t>(std::distance(outputColors.begin(), outputColors.end()));
    IndexRange paletteIndexes(paletteIndex, static_cast<size_t>(1), outputCount);
    return samplePalette(palette, paletteIndexes, std::forward<TOutputRange>(outputColors), options);
}

template <
    typename TColor, typename TIndexRange, typename TOutputRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value &&
                                IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette<TColor>& paletteFrom, const IPalette<TColor>& paletteTo,
                     TIndexRange&& paletteIndexes, TOutputRange&& outputColors, uint8_t blendProgress8,
                     PaletteSampleOptions<TColor> options = {})
{
    return writeSampleRange(paletteTransitionSamples(paletteFrom, paletteTo, std::forward<TIndexRange>(paletteIndexes),
                                                     blendProgress8, options),
                            std::forward<TOutputRange>(outputColors));
}

template <
    typename TColor, typename TOutputRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette<TColor>& paletteFrom, const IPalette<TColor>& paletteTo, size_t firstPaletteIndex,
                     TOutputRange&& outputColors, uint8_t blendProgress8, PaletteSampleOptions<TColor> options = {})
{
    const size_t outputCount = static_cast<size_t>(std::distance(outputColors.begin(), outputColors.end()));
    IndexRange paletteIndexes(firstPaletteIndex, static_cast<size_t>(1), outputCount);
    return samplePalette(paletteFrom, paletteTo, paletteIndexes, std::forward<TOutputRange>(outputColors),
                         blendProgress8, options);
}

template <
    typename TColor, typename TIndexRange, typename TOutputRange,
    typename = std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value &&
                                IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette<TColor>& paletteFrom, const IPalette<TColor>& paletteTo,
                     TIndexRange&& paletteIndexes, TOutputRange&& outputColors, uint8_t transitionProgress,
                     uint8_t transitionDuration, PaletteSampleOptions<TColor> options = {})
{
    return samplePalette(paletteFrom, paletteTo, paletteIndexes, outputColors,
                         mapTransitionProgressToBlend8(transitionProgress, transitionDuration), options);
}

} // namespace lw::colors::palettes
