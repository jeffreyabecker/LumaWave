#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

#include "colors/ColorMath.h"
#include "colors/palette/Traits.h"

namespace lw::colors::palettes
{
namespace samplingtransition
{
template <typename TColor, typename TOutputIt, typename = std::enable_if_t<ColorType<TColor>>> class BlendAssignProxy
{
  public:
    BlendAssignProxy(TOutputIt output, uint8_t blendProgress) : _output(output), _blendProgress(blendProgress) {}

    BlendAssignProxy& operator=(const TColor& sampled)
    {
        *_output = lw::colors::linearBlendProgress8(*_output, sampled, _blendProgress);
        return *this;
    }

  private:
    TOutputIt _output;
    uint8_t _blendProgress;
};

template <typename TColor, typename TOutputIt, typename = std::enable_if_t<ColorType<TColor>>> class BlendOutputIterator
{
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = TColor;
    using difference_type = std::ptrdiff_t;
    using reference = BlendAssignProxy<TColor, TOutputIt>;
    using pointer = void;

    BlendOutputIterator(TOutputIt output, uint8_t blendProgress) : _output(output), _blendProgress(blendProgress) {}

    reference operator*() const { return reference(_output, _blendProgress); }

    BlendOutputIterator& operator++()
    {
        ++_output;
        return *this;
    }

    BlendOutputIterator operator++(int)
    {
        BlendOutputIterator copy = *this;
        ++(*this);
        return copy;
    }

    friend bool operator==(const BlendOutputIterator& left, const BlendOutputIterator& right)
    {
        return left._output == right._output;
    }

    friend bool operator!=(const BlendOutputIterator& left, const BlendOutputIterator& right)
    {
        return !(left == right);
    }

  private:
    TOutputIt _output;
    uint8_t _blendProgress;
};

template <typename TColor, typename TOutputRange,
          typename =
              std::enable_if_t<ColorType<TColor> && IsBeginEndRange<std::remove_reference_t<TOutputRange>>::value>>
class BlendOutputRange
{
  public:
    using Iterator = BlendOutputIterator<TColor, decltype(std::declval<TOutputRange&>().begin())>;

    BlendOutputRange(TOutputRange& outputColors, uint8_t blendProgress)
        : _outputColors(outputColors), _blendProgress(blendProgress)
    {
    }

    Iterator begin() const { return Iterator(_outputColors.begin(), _blendProgress); }

    Iterator end() const { return Iterator(_outputColors.end(), _blendProgress); }

  private:
    TOutputRange& _outputColors;
    uint8_t _blendProgress;
};

} // namespace samplingtransition

inline uint8_t mapTransitionProgressToBlend8(uint8_t transitionProgress, uint8_t transitionDuration)
{
    if (transitionDuration == 0)
    {
        return 255;
    }

    const uint8_t clamped = (transitionProgress >= transitionDuration) ? transitionDuration : transitionProgress;
    return static_cast<uint8_t>((static_cast<uint16_t>(clamped) * 255u) / transitionDuration);
}

} // namespace lw::colors::palettes
