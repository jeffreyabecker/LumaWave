#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace lw
{

namespace detail
{

  struct NullWritable
  {
    size_t write(const uint8_t*, size_t) { return 0; }
  };

} // namespace detail

template <typename TWritable, typename = void> struct WritableImpl : std::false_type
{
};

template <typename TWritable>
struct WritableImpl<TWritable, std::void_t<decltype(std::declval<TWritable&>().write(std::declval<const uint8_t*>(), std::declval<size_t>()))>>
    : std::integral_constant<bool, std::is_convertible_v<decltype(std::declval<TWritable&>().write(std::declval<const uint8_t*>(), std::declval<size_t>())), size_t>>
{
};

template <typename TWritable> static constexpr bool Writable = WritableImpl<TWritable>::value;

} // namespace lw
