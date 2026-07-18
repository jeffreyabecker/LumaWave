#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include "palettes/Types.h"

namespace lw::palettes
{
template <typename TRange, typename = void> struct IsBeginEndRange : std::false_type
{
};

template <typename TRange> struct IsBeginEndRange<TRange, std::void_t<decltype(std::declval<TRange&>().begin()), decltype(std::declval<TRange&>().end())>> : std::true_type
{
};

template <typename TPaletteLike, typename = void> struct IsPaletteLike : std::false_type
{
};

template <typename TPaletteLike>
struct IsPaletteLike<TPaletteLike, std::void_t<typename TPaletteLike::ColorType, decltype(std::declval<const TPaletteLike&>().stops()), decltype(std::declval<TPaletteLike&>().update())>>
    : std::integral_constant<bool, std::is_base_of_v<IPalette, TPaletteLike> &&
                                       std::is_convertible_v<decltype(std::declval<const TPaletteLike&>().stops()), lw::span<const PaletteStop>>>
{
};

template <typename TPaletteLike> using EnableIfPaletteLike = std::enable_if_t<IsPaletteLike<TPaletteLike>::value>;

} // namespace lw::palettes
