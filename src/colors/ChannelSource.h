#pragma once

#include <cstdint>
#include <type_traits>

namespace lw::colors
{

template <typename TValue = uint8_t> struct ChannelSource
{
  TValue R{};
  TValue G{};
  TValue B{};
  TValue W{};
};

} // namespace lw::colors

namespace lw
{

template <typename TValue = uint8_t> using ChannelSource = colors::ChannelSource<TValue>;

} // namespace lw
