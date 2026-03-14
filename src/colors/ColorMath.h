#pragma once

#include "Color.h"
#include "ColorMathBackend.h"

namespace lw::colors
{
template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> struct ColorMathBackendSelector
{
    using Type = LW_COLOR_MATH_BACKEND<TColor>;
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t smoothstep8(uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::smoothstep8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t cubicEaseInOut8(uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::cubicEaseInOut8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t cosineLike8(uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::cosineLike8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint32_t integerSqrt(uint32_t value)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::integerSqrt(value);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr void darken(TColor& color, typename TColor::ComponentType delta)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    Backend::darken(color, delta);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr void lighten(TColor& color, typename TColor::ComponentType delta)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    Backend::lighten(color, delta);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendUnitFloat(const TColor& left, const TColor& right, float progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::linearBlendUnitFloat(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress8(const TColor& left, const TColor& right, uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::linearBlendProgress8(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress16(const TColor& left, const TColor& right, uint16_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::linearBlendProgress16(left, right, progress);
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

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor bilinearBlend(const TColor& c00, const TColor& c01, const TColor& c10, const TColor& c11, float x,
                               float y)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::bilinearBlend(c00, c01, c10, c11, x, y);
}
} // namespace lw::colors

namespace lw
{

template <typename TColor, typename Enable = std::enable_if_t<ColorType<TColor>>>
struct ColorMathBackendSelector : colors::ColorMathBackendSelector<TColor, Enable>
{
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t smoothstep8(uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::smoothstep8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t cubicEaseInOut8(uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::cubicEaseInOut8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint8_t cosineLike8(uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::cosineLike8(progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr uint32_t integerSqrt(uint32_t value)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::integerSqrt(value);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr void darken(TColor& color, typename TColor::ComponentType delta)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    Backend::darken(color, delta);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr void lighten(TColor& color, typename TColor::ComponentType delta)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    Backend::lighten(color, delta);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendUnitFloat(const TColor& left, const TColor& right, float progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::linearBlendUnitFloat(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress8(const TColor& left, const TColor& right, uint8_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::linearBlendProgress8(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress16(const TColor& left, const TColor& right, uint16_t progress)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::linearBlendProgress16(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress(const TColor& left, const TColor& right, uint8_t progress)
{
    return lw::colors::linearBlendProgress8(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor linearBlendProgress(const TColor& left, const TColor& right, uint16_t progress)
{
    return lw::colors::linearBlendProgress16(left, right, progress);
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
constexpr TColor bilinearBlend(const TColor& c00, const TColor& c01, const TColor& c10, const TColor& c11, float x,
                               float y)
{
    using Backend = typename ColorMathBackendSelector<TColor>::Type;
    return Backend::bilinearBlend(c00, c01, c10, c11, x, y);
}

} // namespace lw
