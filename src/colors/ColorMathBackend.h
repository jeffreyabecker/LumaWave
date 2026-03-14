#pragma once

#include <cstdint>
#include <type_traits>

#include "colors/Color.h"

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

    static constexpr TColor linearBlendUnitFloat(const TColor& left, const TColor& right, float progress)
    {
        using SignedWide = std::conditional_t<(sizeof(ComponentType) <= 2), int32_t, int64_t>;

        TColor blended;
        for (auto channel : TColor::channelIndexes())
        {
            const SignedWide delta = static_cast<SignedWide>(right[channel]) - static_cast<SignedWide>(left[channel]);
            const float value = static_cast<float>(left[channel]) + (static_cast<float>(delta) * progress);
            blended[channel] = static_cast<ComponentType>(value);
        }

        return blended;
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
            // Preserve the historical 8-bit blending contract used by shader tests:
            // blend = floor((left * (256 - p) + right * p + 1) / 256)
            const UnsignedWide numerator =
                (leftValue * inverseProgress) + (rightValue * progressWide) + static_cast<UnsignedWide>(1u);
            blended[channel] = static_cast<ComponentType>(numerator / static_cast<UnsignedWide>(256u));
        }

        return blended;
    }

    static constexpr TColor linearBlendProgress16(const TColor& left, const TColor& right, uint16_t progress)
    {
        using UnsignedWide = uint64_t;

        TColor blended;
        for (auto channel : TColor::channelIndexes())
        {
            const UnsignedWide leftValue = static_cast<UnsignedWide>(left[channel]);
            const UnsignedWide rightValue = static_cast<UnsignedWide>(right[channel]);
            const UnsignedWide progressWide = static_cast<UnsignedWide>(progress);
            const UnsignedWide inverseProgress = static_cast<UnsignedWide>(65536u) - progressWide;
            const UnsignedWide numerator =
                (leftValue * inverseProgress) + (rightValue * progressWide) + static_cast<UnsignedWide>(1u);
            blended[channel] = static_cast<ComponentType>(numerator / static_cast<UnsignedWide>(65536u));
        }

        return blended;
    }

    static constexpr TColor bilinearBlend(const TColor& c00, const TColor& c01, const TColor& c10, const TColor& c11,
                                          float x, float y)
    {
        const float v00 = (1.0f - x) * (1.0f - y);
        const float v10 = x * (1.0f - y);
        const float v01 = (1.0f - x) * y;
        const float v11 = x * y;

        TColor blended;
        for (auto channel : TColor::channelIndexes())
        {
            const float value = static_cast<float>(c00[channel]) * v00 + static_cast<float>(c10[channel]) * v10 +
                                static_cast<float>(c01[channel]) * v01 + static_cast<float>(c11[channel]) * v11;
            blended[channel] = static_cast<ComponentType>(value);
        }

        return blended;
    }
};
} // namespace lw::colors::detail

namespace lw::detail
{

template <typename TColor> using ScalarColorMathBackend = colors::detail::ScalarColorMathBackend<TColor>;

} // namespace lw::detail
