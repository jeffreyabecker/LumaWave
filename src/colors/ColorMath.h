#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "Color.h"

namespace lw::colors::detail
{
template <typename TColor> struct ScalarColorMathBackend
{
    using ComponentType = typename TColor::ComponentType;

    static constexpr uint8_t smoothstep8(uint8_t progress)
    {
        const uint32_t p = progress;
        const uint32_t numerator = p * p * (765u - (2u * p));
        return static_cast<uint8_t>(numerator / 65025u);
    }

    static constexpr uint8_t cubicEaseInOut8(uint8_t progress)
    {
        const uint32_t p = progress;
        if (p < 128u)
        {
            return static_cast<uint8_t>((4u * p * p * p) / 65025u);
        }

        const uint32_t q = 255u - p;
        return static_cast<uint8_t>(255u - ((4u * q * q * q) / 65025u));
    }

    static constexpr uint8_t cosineLike8(uint8_t progress)
    {
        const uint32_t p = progress;
        if (p < 128u)
        {
            return static_cast<uint8_t>((2u * p * p) / 255u);
        }

        const uint32_t q = 255u - p;
        return static_cast<uint8_t>(255u - ((2u * q * q) / 255u));
    }

    static constexpr uint32_t integerSqrt(uint32_t value)
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

    static constexpr uint8_t scaleComponentToByte(ComponentType value)
    {
        using ScaleWide = uint64_t;

        constexpr ScaleWide maxSource = static_cast<ScaleWide>(std::numeric_limits<ComponentType>::max());
        constexpr ScaleWide maxTarget = static_cast<ScaleWide>(std::numeric_limits<uint8_t>::max());

        if constexpr (maxSource == maxTarget)
        {
            return static_cast<uint8_t>(value);
        }

        return static_cast<uint8_t>(((static_cast<ScaleWide>(value) * maxTarget) + (maxSource / 2u)) / maxSource);
    }

    static constexpr ComponentType scaleByteToComponent(uint8_t value)
    {
        using ScaleWide = uint64_t;

        constexpr ScaleWide maxSource = static_cast<ScaleWide>(std::numeric_limits<uint8_t>::max());
        constexpr ScaleWide maxTarget = static_cast<ScaleWide>(std::numeric_limits<ComponentType>::max());

        if constexpr (maxSource == maxTarget)
        {
            return static_cast<ComponentType>(value);
        }

        return static_cast<ComponentType>(((static_cast<ScaleWide>(value) * maxTarget) + (maxSource / 2u)) / maxSource);
    }

    static constexpr uint8_t linearBlendScalar8(uint8_t left, uint8_t right, uint8_t progress)
    {
        if (progress == 0u)
        {
            return left;
        }

        if (progress == 255u)
        {
            return right;
        }

        const uint32_t inverseProgress = static_cast<uint32_t>(255u) - static_cast<uint32_t>(progress);
        const uint32_t numerator = (static_cast<uint32_t>(left) * inverseProgress) +
                                   (static_cast<uint32_t>(right) * static_cast<uint32_t>(progress)) + 127u;
        return static_cast<uint8_t>(numerator / 255u);
    }

    static constexpr uint8_t bilinearBlendScalar8(uint8_t c00, uint8_t c01, uint8_t c10, uint8_t c11, uint8_t x,
                                                  uint8_t y)
    {
        const uint8_t top = linearBlendScalar8(c00, c10, x);
        const uint8_t bottom = linearBlendScalar8(c01, c11, x);
        return linearBlendScalar8(top, bottom, y);
    }

    static constexpr void hsbToRgb8(uint8_t hue, uint8_t saturation, uint8_t brightness, uint8_t& r, uint8_t& g,
                                    uint8_t& b)
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
        const uint8_t q = static_cast<uint8_t>(
            (static_cast<uint16_t>(brightness) *
                 (255u - ((static_cast<uint16_t>(saturation) * static_cast<uint16_t>(remainder) + 127u) / 255u)) +
             127u) /
            255u);
        const uint8_t t = static_cast<uint8_t>(
            (static_cast<uint16_t>(brightness) *
                 (255u -
                  ((static_cast<uint16_t>(saturation) * static_cast<uint16_t>(255u - remainder) + 127u) / 255u)) +
             127u) /
            255u);

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

    static constexpr uint8_t hslHueComponent8(int16_t p, int16_t q, int16_t t)
    {
        while (t < 0)
        {
            t += 256;
        }

        while (t > 255)
        {
            t -= 256;
        }

        if (t < 43)
        {
            const int32_t delta = static_cast<int32_t>(q) - static_cast<int32_t>(p);
            const int32_t value = static_cast<int32_t>(p) + ((delta * 6 * t + 127) / 255);
            return static_cast<uint8_t>(std::clamp<int32_t>(value, 0, 255));
        }

        if (t < 128)
        {
            return static_cast<uint8_t>(q);
        }

        if (t < 171)
        {
            const int32_t delta = static_cast<int32_t>(q) - static_cast<int32_t>(p);
            const int32_t value = static_cast<int32_t>(p) + ((delta * 6 * (171 - t) + 127) / 255);
            return static_cast<uint8_t>(std::clamp<int32_t>(value, 0, 255));
        }

        return static_cast<uint8_t>(p);
    }

    static constexpr void hslToRgb8(uint8_t hue, uint8_t saturation, uint8_t lightness, uint8_t& r, uint8_t& g,
                                    uint8_t& b)
    {
        if (saturation == 0u)
        {
            r = lightness;
            g = lightness;
            b = lightness;
            return;
        }

        const int16_t q =
            (lightness < 128u)
                ? static_cast<int16_t>((static_cast<uint16_t>(lightness) * (255u + saturation) + 127u) / 255u)
                : static_cast<int16_t>(lightness + saturation -
                                       ((static_cast<uint16_t>(lightness) * saturation + 127u) / 255u));
        const int16_t p = static_cast<int16_t>((2 * lightness) - q);

        r = hslHueComponent8(p, q, static_cast<int16_t>(hue) + 85);
        g = hslHueComponent8(p, q, static_cast<int16_t>(hue));
        b = hslHueComponent8(p, q, static_cast<int16_t>(hue) - 85);
    }

    static constexpr void darken(TColor& color, ComponentType delta)
    {
        for (auto channel : TColor::channelIndexes())
        {
            auto& component = color[channel];
            if (component > delta)
            {
                component = static_cast<ComponentType>(component - delta);
            }
            else
            {
                component = static_cast<ComponentType>(0);
            }
        }
    }

    static constexpr void lighten(TColor& color, ComponentType delta)
    {
        for (auto channel : TColor::channelIndexes())
        {
            auto& component = color[channel];
            if (component < static_cast<ComponentType>(TColor::MaxComponent - delta))
            {
                component = static_cast<ComponentType>(component + delta);
            }
            else
            {
                component = TColor::MaxComponent;
            }
        }
    }

    static constexpr TColor linearBlendProgress8(const TColor& left, const TColor& right, uint8_t progress)
    {
        using UnsignedWide = std::conditional_t<(sizeof(ComponentType) <= 2), uint32_t, uint64_t>;

        TColor blended;
        for (auto channel : TColor::channelIndexes())
        {
            const UnsignedWide leftValue = static_cast<UnsignedWide>(left[channel]);
            const UnsignedWide rightValue = static_cast<UnsignedWide>(right[channel]);
            const UnsignedWide progressWide = static_cast<UnsignedWide>(progress);
            const UnsignedWide inverseProgress = static_cast<UnsignedWide>(256u) - progressWide;
            const UnsignedWide numerator =
                (leftValue * inverseProgress) + (rightValue * progressWide) + static_cast<UnsignedWide>(1u);
            blended[channel] = static_cast<ComponentType>(numerator / static_cast<UnsignedWide>(256u));
        }

        return blended;
    }

    static constexpr TColor linearBlendProgress16(const TColor& left, const TColor& right, uint16_t progress)
    {
        using UnsignedWide = uint64_t;

        if (progress == 0u)
        {
            return left;
        }

        if (progress == 65535u)
        {
            return right;
        }

        TColor blended;
        for (auto channel : TColor::channelIndexes())
        {
            const UnsignedWide leftValue = static_cast<UnsignedWide>(left[channel]);
            const UnsignedWide rightValue = static_cast<UnsignedWide>(right[channel]);
            const UnsignedWide progressWide = static_cast<UnsignedWide>(progress);
            const UnsignedWide inverseProgress = static_cast<UnsignedWide>(65535u) - progressWide;
            const UnsignedWide numerator =
                (leftValue * inverseProgress) + (rightValue * progressWide) + static_cast<UnsignedWide>(32767u);
            blended[channel] = static_cast<ComponentType>(numerator / static_cast<UnsignedWide>(65535u));
        }

        return blended;
    }
};
} // namespace lw::colors::detail

namespace lw::colors
{
template <typename TValue, typename TBrightness,
          typename = std::enable_if_t<std::is_integral_v<TValue> && std::is_unsigned_v<TValue> &&
                                      std::is_integral_v<TBrightness> && std::is_unsigned_v<TBrightness> &&
                                      !std::is_same_v<std::remove_cv_t<TValue>, bool> &&
                                      !std::is_same_v<std::remove_cv_t<TBrightness>, bool>>>
constexpr TValue applyBrightness(TValue value, TBrightness brightness)
{
    using ScaleWide = uint64_t;

    constexpr ScaleWide brightnessMax = static_cast<ScaleWide>(std::numeric_limits<TBrightness>::max());

    if constexpr (brightnessMax == 0)
    {
        return static_cast<TValue>(0);
    }

    return static_cast<TValue>(((static_cast<ScaleWide>(value) * static_cast<ScaleWide>(brightness)) +
                                (brightnessMax / 2u)) /
                               brightnessMax);
}

template <typename TTarget, typename TSource,
          typename = std::enable_if_t<std::is_integral_v<TTarget> && std::is_unsigned_v<TTarget> &&
                                      std::is_integral_v<TSource> && std::is_unsigned_v<TSource> &&
                                      !std::is_same_v<std::remove_cv_t<TTarget>, bool> &&
                                      !std::is_same_v<std::remove_cv_t<TSource>, bool>>>
constexpr TTarget scaleComponent(TSource source)
{
    using ScaleWide = uint64_t;

    constexpr ScaleWide maxSource = static_cast<ScaleWide>(std::numeric_limits<TSource>::max());
    constexpr ScaleWide maxTarget = static_cast<ScaleWide>(std::numeric_limits<TTarget>::max());

    if constexpr (maxSource == maxTarget)
    {
        return static_cast<TTarget>(source);
    }

    return static_cast<TTarget>(((static_cast<ScaleWide>(source) * maxTarget) + (maxSource / 2u)) / maxSource);
}

template <
    typename TTarget, typename TValue, typename TProgress, typename TProgressMax,
    typename = std::enable_if_t<
        std::is_integral_v<TTarget> && std::is_unsigned_v<TTarget> && std::is_integral_v<TValue> &&
        std::is_unsigned_v<TValue> && std::is_integral_v<TProgress> && std::is_unsigned_v<TProgress> &&
        std::is_integral_v<TProgressMax> && std::is_unsigned_v<TProgressMax> &&
        !std::is_same_v<std::remove_cv_t<TTarget>, bool> && !std::is_same_v<std::remove_cv_t<TValue>, bool> &&
        !std::is_same_v<std::remove_cv_t<TProgress>, bool> && !std::is_same_v<std::remove_cv_t<TProgressMax>, bool>>>
constexpr TTarget interpolateComponent(TValue left, TValue right, TProgress progress, TProgressMax maxProgress)
{
    using SignedScaleWide = int64_t;

    if (progress == 0u || maxProgress == 0u)
    {
        return scaleComponent<TTarget>(left);
    }

    if (progress >= maxProgress)
    {
        return scaleComponent<TTarget>(right);
    }

    const SignedScaleWide leftWide = static_cast<SignedScaleWide>(left);
    const SignedScaleWide rightWide = static_cast<SignedScaleWide>(right);
    const SignedScaleWide delta = rightWide - leftWide;
    const SignedScaleWide progressWide = static_cast<SignedScaleWide>(progress);
    const SignedScaleWide maxProgressWide = static_cast<SignedScaleWide>(maxProgress);
    const SignedScaleWide numerator = (leftWide * maxProgressWide) + (delta * progressWide) + (maxProgressWide / 2);

    return scaleComponent<TTarget>(static_cast<TValue>(numerator / maxProgressWide));
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t smoothstep8(uint8_t progress)
{
    return detail::ScalarColorMathBackend<TColor>::smoothstep8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t cubicEaseInOut8(uint8_t progress)
{
    return detail::ScalarColorMathBackend<TColor>::cubicEaseInOut8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t cosineLike8(uint8_t progress)
{
    return detail::ScalarColorMathBackend<TColor>::cosineLike8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint32_t integerSqrt(uint32_t value)
{
    return detail::ScalarColorMathBackend<TColor>::integerSqrt(value);
}

template <typename TColor, typename = std::enable_if_t<ColorChannelsAtLeast<TColor, 3>>>
constexpr TColor hslToRgb(typename TColor::ComponentType h, typename TColor::ComponentType s,
                          typename TColor::ComponentType l)
{
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t b = 0u;
    detail::ScalarColorMathBackend<TColor>::hslToRgb8(detail::ScalarColorMathBackend<TColor>::scaleComponentToByte(h),
                                                      detail::ScalarColorMathBackend<TColor>::scaleComponentToByte(s),
                                                      detail::ScalarColorMathBackend<TColor>::scaleComponentToByte(l),
                                                      r, g, b);

    TColor rgb{};
    rgb['R'] = detail::ScalarColorMathBackend<TColor>::scaleByteToComponent(r);
    rgb['G'] = detail::ScalarColorMathBackend<TColor>::scaleByteToComponent(g);
    rgb['B'] = detail::ScalarColorMathBackend<TColor>::scaleByteToComponent(b);

    if constexpr (ColorChannelsAtLeast<TColor, 4>)
    {
        rgb['W'] = static_cast<typename TColor::ComponentType>(0);
    }

    if constexpr (ColorChannelsAtLeast<TColor, 5>)
    {
        rgb['C'] = static_cast<typename TColor::ComponentType>(0);
    }

    return rgb;
}

template <typename TColor, typename = std::enable_if_t<ColorChannelsAtLeast<TColor, 3>>>
constexpr TColor hsbToRgb(typename TColor::ComponentType h, typename TColor::ComponentType s,
                          typename TColor::ComponentType b)
{
    uint8_t r = 0u;
    uint8_t g = 0u;
    uint8_t blue = 0u;
    detail::ScalarColorMathBackend<TColor>::hsbToRgb8(detail::ScalarColorMathBackend<TColor>::scaleComponentToByte(h),
                                                      detail::ScalarColorMathBackend<TColor>::scaleComponentToByte(s),
                                                      detail::ScalarColorMathBackend<TColor>::scaleComponentToByte(b),
                                                      r, g, blue);

    TColor rgb{};
    rgb['R'] = detail::ScalarColorMathBackend<TColor>::scaleByteToComponent(r);
    rgb['G'] = detail::ScalarColorMathBackend<TColor>::scaleByteToComponent(g);
    rgb['B'] = detail::ScalarColorMathBackend<TColor>::scaleByteToComponent(blue);

    if constexpr (ColorChannelsAtLeast<TColor, 4>)
    {
        rgb['W'] = static_cast<typename TColor::ComponentType>(0);
    }

    if constexpr (ColorChannelsAtLeast<TColor, 5>)
    {
        rgb['C'] = static_cast<typename TColor::ComponentType>(0);
    }

    return rgb;
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr void darken(TColor& color, typename TColor::ComponentType delta)
{
    detail::ScalarColorMathBackend<TColor>::darken(color, delta);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr void lighten(TColor& color, typename TColor::ComponentType delta)
{
    detail::ScalarColorMathBackend<TColor>::lighten(color, delta);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress8(const TColor& left, const TColor& right, uint8_t progress)
{
    return detail::ScalarColorMathBackend<TColor>::linearBlendProgress8(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress16(const TColor& left, const TColor& right, uint16_t progress)
{
    return detail::ScalarColorMathBackend<TColor>::linearBlendProgress16(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress(const TColor& left, const TColor& right, uint8_t progress)
{
    return colors::linearBlendProgress8(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress(const TColor& left, const TColor& right, uint16_t progress)
{
    return colors::linearBlendProgress16(left, right, progress);
}
} // namespace lw::colors
