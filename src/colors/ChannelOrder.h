#pragma once

#include <cstddef>
#include <string>

namespace lw::colors
{

namespace ChannelOrder
{

#define DECLARE_CHANNEL_ORDER(name)                                                                                                                                                                                             \
  struct name                                                                                                                                                                                                                   \
  {                                                                                                                                                                                                                             \
    static constexpr const char* value = #name;                                                                                                                                                                                 \
    static constexpr size_t length = std::char_traits<char>::length(value);                                                                                                                                                     \
    constexpr operator const char*() const { return value; }                                                                                                                                                                    \
  }

  DECLARE_CHANNEL_ORDER(RGB);
  DECLARE_CHANNEL_ORDER(GRB);
  DECLARE_CHANNEL_ORDER(BGR);
  DECLARE_CHANNEL_ORDER(RGBW);
  DECLARE_CHANNEL_ORDER(GRBW);
  DECLARE_CHANNEL_ORDER(BGRW);
  DECLARE_CHANNEL_ORDER(WRGB);
  DECLARE_CHANNEL_ORDER(W);
  DECLARE_CHANNEL_ORDER(CW);

#undef DECLARE_CHANNEL_ORDER

} // namespace ChannelOrder

namespace detail
{

  inline const char* normalizeChannelOrder(const char* providedChannelOrder, const char* defaultChannelOrder)
  {
    if (providedChannelOrder == nullptr || providedChannelOrder[0] == '\0')
    {
      return defaultChannelOrder;
    }

    size_t len = 0;
    while (providedChannelOrder[len] != '\0')
    {
      const char c = providedChannelOrder[len];
      if (c != 'R' && c != 'r' && c != 'G' && c != 'g' && c != 'B' && c != 'b' && c != 'W' && c != 'w')
      {
        return defaultChannelOrder;
      }

      ++len;
    }

    if (len < 3 || len > 4)
    {
      return defaultChannelOrder;
    }

    return providedChannelOrder;
  }

} // namespace detail

} // namespace lw::colors

namespace lw
{

namespace ChannelOrder = colors::ChannelOrder;

namespace detail
{
  using colors::detail::normalizeChannelOrder;
}

} // namespace lw
