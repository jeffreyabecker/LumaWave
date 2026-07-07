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

  inline const char* normalizeChannelOrderForCount(const char* providedChannelOrder, const char* defaultChannelOrder, size_t channelCount)
  {
    const char* channelOrder = (providedChannelOrder != nullptr) ? providedChannelOrder : defaultChannelOrder;
    if (channelOrder == nullptr)
    {
      return defaultChannelOrder;
    }

    const size_t suppliedLength = std::char_traits<char>::length(channelOrder);
    if (suppliedLength == channelCount)
    {
      return channelOrder;
    }

    const char* value = channelOrder;
    if (std::char_traits<char>::length(value) < 3)
    {
      value = defaultChannelOrder;
    }

    bool grbPrefix = false;
    bool bgrPrefix = false;
    if (value != nullptr && std::char_traits<char>::length(value) >= 3)
    {
      grbPrefix = (value[0] == 'G' && value[1] == 'R' && value[2] == 'B');
      bgrPrefix = (value[0] == 'B' && value[1] == 'G' && value[2] == 'R');
    }

    if (channelCount <= 3)
    {
      if (grbPrefix)
      {
        return ChannelOrder::GRB::value;
      }

      if (bgrPrefix)
      {
        return ChannelOrder::BGR::value;
      }

      return ChannelOrder::RGB::value;
    }

    if (grbPrefix)
    {
      return ChannelOrder::GRBW::value;
    }

    if (bgrPrefix)
    {
      return ChannelOrder::BGRW::value;
    }

    return ChannelOrder::RGBW::value;
  }

} // namespace detail

} // namespace lw::colors

namespace lw
{

namespace ChannelOrder = colors::ChannelOrder;

namespace detail
{
  using colors::detail::normalizeChannelOrderForCount;
}

} // namespace lw
