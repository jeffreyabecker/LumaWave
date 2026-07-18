#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

#include "palettes/Blends.h"
#include "palettes/SamplingTransition.h"
#include "palettes/Traits.h"

namespace lw::colors::palettes
{
namespace detail
{
  template <typename TRange> using SampleRangeStorageType = std::conditional_t<std::is_lvalue_reference<TRange>::value, TRange, std::remove_reference_t<TRange>>;

  template <typename TRange, typename = void> struct HasSizeMethod : std::false_type
  {
  };

  template <typename TRange> struct HasSizeMethod<TRange, std::void_t<decltype(std::declval<const TRange&>().size())>> : std::true_type
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

template <typename TSampler, typename TIndexIt, typename = void> class PaletteSampleIterator
{
public:
  using iterator_category = std::input_iterator_tag;
  using value_type = lw::Color;
  using difference_type = std::ptrdiff_t;
  using reference = lw::Color;
  using pointer = void;

  PaletteSampleIterator(const TSampler* sampler, TIndexIt current, const void* ownerIdentity, size_t remaining) : _sampler(sampler), _current(current), _ownerIdentity(ownerIdentity), _remaining(remaining) {}

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

  friend bool operator==(const PaletteSampleIterator& left, const PaletteSampleIterator& right) { return left._ownerIdentity == right._ownerIdentity && left._remaining == right._remaining; }

  friend bool operator!=(const PaletteSampleIterator& left, const PaletteSampleIterator& right) { return !(left == right); }

private:
  const TSampler* _sampler;
  TIndexIt _current;
  const void* _ownerIdentity;
  size_t _remaining;
};

class SinglePaletteSampler
{
public:
  SinglePaletteSampler(const IPalette& palette, PaletteSampleOptions options, size_t offset = 0) : _palette(palette), _options(options), _offset(offset) {}

  lw::Color operator()(size_t paletteIndex) const { return samplePaletteAt(_palette.stops(), paletteIndex + _offset, _options); }

private:
  const IPalette& _palette;
  PaletteSampleOptions _options;
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

template <typename TSamplerFrom, typename TSamplerTo, typename TBlendProgressSampler, typename TBlendDomain = uint8_t, typename = void> class TransitionSampler
{
public:
  TransitionSampler(TSamplerFrom sampleFrom, TSamplerTo sampleTo, TBlendProgressSampler sampleProgress) : _sampleFrom(std::move(sampleFrom)), _sampleTo(std::move(sampleTo)), _sampleProgress(std::move(sampleProgress)) {}

  lw::Color operator()(size_t paletteIndex) const
  {
    const lw::Color from = _sampleFrom(paletteIndex);
    const lw::Color to = _sampleTo(paletteIndex);
    const TBlendDomain progress = static_cast<TBlendDomain>(_sampleProgress(paletteIndex));
    if constexpr (std::is_same_v<TBlendDomain, uint8_t>)
    {
      return lw::colors::linearBlendProgress(from, to, progress);
    }

    const uint8_t blendProgress8 = lw::colors::scaleComponent<uint8_t>(progress);
    return lw::colors::linearBlendProgress(from, to, blendProgress8);
  }

private:
  TSamplerFrom _sampleFrom;
  TSamplerTo _sampleTo;
  TBlendProgressSampler _sampleProgress;
};

class TransitionPaletteSampler
{
public:
  using SamplerType = TransitionSampler<SinglePaletteSampler, SinglePaletteSampler, ConstantSampler<uint8_t>, uint8_t>;

  TransitionPaletteSampler(const IPalette& paletteFrom, const IPalette& paletteTo, uint8_t blendProgress8, PaletteSampleOptions options)
      : _sampler(SinglePaletteSampler(paletteFrom, options), SinglePaletteSampler(paletteTo, options), ConstantSampler<uint8_t>(blendProgress8))
  {
  }

  lw::Color operator()(size_t paletteIndex) const { return _sampler(paletteIndex); }

private:
  SamplerType _sampler;
};

template <typename TSampler, typename TStoredIndexRange, typename = std::enable_if_t<IsBeginEndRange<TStoredIndexRange>::value>> class PaletteSampleRangeT
{
public:
  using Iterator = PaletteSampleIterator<TSampler, decltype(std::declval<TStoredIndexRange&>().begin())>;

  PaletteSampleRangeT(TSampler sampler, TStoredIndexRange paletteIndexes) : _sampler(std::move(sampler)), _paletteIndexes(paletteIndexes), _count(detail::countRangeElements(_paletteIndexes)) {}

  Iterator begin() const { return Iterator(&_sampler, _paletteIndexes.begin(), this, _count); }

  Iterator end() const { return Iterator(&_sampler, _paletteIndexes.begin(), this, 0); }

  size_t size() const { return _count; }

private:
  TSampler _sampler;
  TStoredIndexRange _paletteIndexes;
  size_t _count;
};

template <typename TSampleRange, typename TOutputRange, std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TSampleRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value, int> = 0>
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
          std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value && !IsBeginEndRange<std::remove_reference_t<TSampler>>::value && std::is_invocable_v<TSampler&, size_t>, int> = 0>
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

template <typename TIndexRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteSamples(const IPalette& palette, TIndexRange&& paletteIndexes, PaletteSampleOptions options = {})
{
  using StoredIndexRange = detail::SampleRangeStorageType<TIndexRange>;
  using Sampler = SinglePaletteSampler;
  using Range = PaletteSampleRangeT<Sampler, StoredIndexRange>;

  return Range(Sampler(palette, options), std::forward<TIndexRange>(paletteIndexes));
}

template <typename TIndexRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette& paletteFrom, const IPalette& paletteTo, TIndexRange&& paletteIndexes, uint8_t blendProgress8, PaletteSampleOptions options = {})
{
  using StoredIndexRange = detail::SampleRangeStorageType<TIndexRange>;
  using Sampler = TransitionPaletteSampler;
  using Range = PaletteSampleRangeT<Sampler, StoredIndexRange>;

  return Range(Sampler(paletteFrom, paletteTo, blendProgress8, options), std::forward<TIndexRange>(paletteIndexes));
}

template <typename TIndexRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette& paletteFrom, const IPalette& paletteTo, TIndexRange&& paletteIndexes, uint8_t transitionProgress, uint8_t transitionDuration, PaletteSampleOptions options = {})
{
  return paletteTransitionSamples(paletteFrom, paletteTo, std::forward<TIndexRange>(paletteIndexes), mapTransitionProgressToBlend8(transitionProgress, transitionDuration), options);
}

template <typename TIndexRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>> auto paletteSamples(const IPalette&&, TIndexRange&&, PaletteSampleOptions = {}) = delete;

template <typename TIndexRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette&&, const IPalette&, TIndexRange&&, uint8_t, PaletteSampleOptions = {}) = delete;

template <typename TIndexRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value>>
auto paletteTransitionSamples(const IPalette&, const IPalette&&, TIndexRange&&, uint8_t, PaletteSampleOptions = {}) = delete;

template <typename TIndexRange, typename TOutputRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette& palette, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, PaletteSampleOptions options = {})
{
  return writeSampleRange(paletteSamples(palette, std::forward<TIndexRange>(paletteIndexes), options), std::forward<TOutputRange>(outputColors));
}

template <typename TOutputRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette& palette, size_t paletteIndex, TOutputRange&& outputColors, PaletteSampleOptions options = {})
{
  const size_t outputCount = static_cast<size_t>(std::distance(outputColors.begin(), outputColors.end()));
  IndexRange paletteIndexes(paletteIndex, static_cast<size_t>(1), outputCount);
  return samplePalette(palette, paletteIndexes, std::forward<TOutputRange>(outputColors), options);
}

template <typename TIndexRange, typename TOutputRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette& paletteFrom, const IPalette& paletteTo, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, uint8_t blendProgress8, PaletteSampleOptions options = {})
{
  return writeSampleRange(paletteTransitionSamples(paletteFrom, paletteTo, std::forward<TIndexRange>(paletteIndexes), blendProgress8, options), std::forward<TOutputRange>(outputColors));
}

template <typename TOutputRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette& paletteFrom, const IPalette& paletteTo, size_t firstPaletteIndex, TOutputRange&& outputColors, uint8_t blendProgress8, PaletteSampleOptions options = {})
{
  const size_t outputCount = static_cast<size_t>(std::distance(outputColors.begin(), outputColors.end()));
  IndexRange paletteIndexes(firstPaletteIndex, static_cast<size_t>(1), outputCount);
  return samplePalette(paletteFrom, paletteTo, paletteIndexes, std::forward<TOutputRange>(outputColors), blendProgress8, options);
}

template <typename TIndexRange, typename TOutputRange, typename = std::enable_if_t<IsBeginEndRange<std::remove_reference_t<TIndexRange>>::value && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
size_t samplePalette(const IPalette& paletteFrom, const IPalette& paletteTo, TIndexRange&& paletteIndexes, TOutputRange&& outputColors, uint8_t transitionProgress, uint8_t transitionDuration,
                     PaletteSampleOptions options = {})
{
  return samplePalette(paletteFrom, paletteTo, paletteIndexes, outputColors, mapTransitionProgressToBlend8(transitionProgress, transitionDuration), options);
}

} // namespace lw::colors::palettes
