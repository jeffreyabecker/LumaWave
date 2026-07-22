#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "core/Pixel.h"

namespace lw
{

// ---------------------------------------------------------------------------
// Easing / progress curves (depth-independent, templated on progress type)
// ---------------------------------------------------------------------------
template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>> constexpr T smoothstep(T progress)
{
  using Wide = std::conditional_t<(sizeof(T) <= 2), uint32_t, uint64_t>;
  constexpr Wide maxVal = static_cast<Wide>(std::numeric_limits<T>::max());

  const Wide p = static_cast<Wide>(progress);
  const Wide numerator = p * p * (static_cast<Wide>(3) * maxVal - static_cast<Wide>(2) * p);
  return static_cast<T>(numerator / (maxVal * maxVal));
}

template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>> constexpr T cubicEaseInOut(T progress)
{
  using Wide = std::conditional_t<(sizeof(T) <= 2), uint32_t, uint64_t>;
  constexpr Wide maxVal = static_cast<Wide>(std::numeric_limits<T>::max());
  constexpr Wide halfMax = maxVal / static_cast<Wide>(2);

  const Wide p = static_cast<Wide>(progress);
  if (p < halfMax)
  {
    return static_cast<T>((static_cast<Wide>(4) * p * p * p) / (maxVal * maxVal));
  }
  const Wide q = maxVal - p;
  return static_cast<T>(maxVal - ((static_cast<Wide>(4) * q * q * q) / (maxVal * maxVal)));
}

template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>> constexpr T cosineLike(T progress)
{
  using Wide = std::conditional_t<(sizeof(T) <= 2), uint32_t, uint64_t>;
  constexpr Wide maxVal = static_cast<Wide>(std::numeric_limits<T>::max());
  constexpr Wide halfMax = maxVal / static_cast<Wide>(2);

  const Wide p = static_cast<Wide>(progress);
  if (p < halfMax)
  {
    return static_cast<T>((static_cast<Wide>(2) * p * p) / maxVal);
  }
  const Wide q = maxVal - p;
  return static_cast<T>(maxVal - ((static_cast<Wide>(2) * q * q) / maxVal));
}

// ---------------------------------------------------------------------------
// Integer square root
// ---------------------------------------------------------------------------
constexpr uint32_t integerSqrt(uint32_t value)
{
  uint32_t root = 0;
  uint32_t bit = 1u << 30;

  while (bit > value)
  {
    bit >>= 2;
  }

  while (bit != 0)
  {
    if (value >= root + bit)
    {
      value -= root + bit;
      root = (root >> 1) + bit;
    }
    else
    {
      root >>= 1;
    }
    bit >>= 2;
  }
  return root;
}

// ---------------------------------------------------------------------------
// Color blending (depth-independent, templated on progress type)
// ---------------------------------------------------------------------------
template <typename TProgress, typename = std::enable_if_t<std::is_unsigned_v<TProgress>>> constexpr lw::Color linearBlendProgress(const lw::Color& left, const lw::Color& right, TProgress progress)
{
  using Wide = std::conditional_t<(sizeof(ColorComponent) <= 2), uint32_t, uint64_t>;
  constexpr Wide maxProgress = static_cast<Wide>(std::numeric_limits<TProgress>::max());

  if (progress == 0)
    return left;
  if (progress == maxProgress)
    return right;

  return lw::mapChannels(left, right,
                         [&](auto lv, auto rv, char)
                         {
                           const Wide leftValue = static_cast<Wide>(lv);
                           const Wide rightValue = static_cast<Wide>(rv);
                           const Wide progressWide = static_cast<Wide>(progress);
                           const Wide inverseProgress = maxProgress - progressWide;
                           const Wide numerator = (leftValue * inverseProgress) + (rightValue * progressWide) + (maxProgress / static_cast<Wide>(2));
                           return static_cast<ColorComponent>(numerator / maxProgress);
                         });
}

// ---------------------------------------------------------------------------
// HSB to RGB
// ---------------------------------------------------------------------------
namespace detail
{
  constexpr void hsbToRgb8(uint8_t hue, uint8_t saturation, uint8_t brightness, uint8_t& r, uint8_t& g, uint8_t& b)
  {
    if (saturation == 0u)
    {
      r = brightness;
      g = brightness;
      b = brightness;
      return;
    }

    const uint8_t region = static_cast<uint8_t>(hue / 43u);
    const uint8_t remainder = static_cast<uint8_t>((hue - static_cast<uint8_t>(region * 43u)) * 6u);

    const uint8_t p = static_cast<uint8_t>((static_cast<uint16_t>(brightness) * (255u - saturation) + 127u) / 255u);
    const uint8_t q = static_cast<uint8_t>((static_cast<uint16_t>(brightness) * (255u - ((static_cast<uint16_t>(saturation) * static_cast<uint16_t>(remainder) + 127u) / 255u)) + 127u) / 255u);
    const uint8_t t = static_cast<uint8_t>((static_cast<uint16_t>(brightness) * (255u - ((static_cast<uint16_t>(saturation) * static_cast<uint16_t>(255u - remainder) + 127u) / 255u)) + 127u) / 255u);

    switch (region)
    {
      case 0:
        r = brightness;
        g = t;
        b = p;
        break;
      case 1:
        r = q;
        g = brightness;
        b = p;
        break;
      case 2:
        r = p;
        g = brightness;
        b = t;
        break;
      case 3:
        r = p;
        g = q;
        b = brightness;
        break;
      case 4:
        r = t;
        g = p;
        b = brightness;
        break;
      default:
        r = brightness;
        g = p;
        b = q;
        break;
    }
  }
} // namespace detail

constexpr lw::Color hsbToRgb(lw::ColorComponent h, lw::ColorComponent s, lw::ColorComponent b)
{
  uint8_t r = 0, g = 0, blue = 0;
  detail::hsbToRgb8(static_cast<uint8_t>((static_cast<uint32_t>(h) * 255u + (std::numeric_limits<ColorComponent>::max() / 2u)) / std::numeric_limits<ColorComponent>::max()),
                    static_cast<uint8_t>((static_cast<uint32_t>(s) * 255u + (std::numeric_limits<ColorComponent>::max() / 2u)) / std::numeric_limits<ColorComponent>::max()),
                    static_cast<uint8_t>((static_cast<uint32_t>(b) * 255u + (std::numeric_limits<ColorComponent>::max() / 2u)) / std::numeric_limits<ColorComponent>::max()), r, g, blue);

  return lw::colorFromRGBW(static_cast<ColorComponent>((static_cast<uint32_t>(r) * std::numeric_limits<ColorComponent>::max() + 127u) / 255u),
                           static_cast<ColorComponent>((static_cast<uint32_t>(g) * std::numeric_limits<ColorComponent>::max() + 127u) / 255u),
                           static_cast<ColorComponent>((static_cast<uint32_t>(blue) * std::numeric_limits<ColorComponent>::max() + 127u) / 255u), static_cast<ColorComponent>(0));
}

// ---------------------------------------------------------------------------
// Component scaling
// ---------------------------------------------------------------------------
template <typename TTarget, typename TSource, typename = std::enable_if_t<std::is_integral_v<TTarget> && std::is_unsigned_v<TTarget> && std::is_integral_v<TSource> && std::is_unsigned_v<TSource>>>
constexpr TTarget scaleComponent(TSource source)
{
  using ScaleWide = uint64_t;
  constexpr ScaleWide maxSource = static_cast<ScaleWide>(std::numeric_limits<TSource>::max());
  constexpr ScaleWide maxTarget = static_cast<ScaleWide>(std::numeric_limits<TTarget>::max());
  if constexpr (maxSource == maxTarget)
    return static_cast<TTarget>(source);
  return static_cast<TTarget>(((static_cast<ScaleWide>(source) * maxTarget) + (maxSource / 2u)) / maxSource);
}

} // namespace lw
