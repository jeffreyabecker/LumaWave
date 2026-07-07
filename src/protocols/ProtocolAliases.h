#pragma once

#include <type_traits>
#include <utility>

#include "colors/Color.h"
#include "protocols/DebugProtocol.h"
#include "protocols/DotStarProtocol.h"
#include "protocols/NilProtocol.h"
#include "protocols/Tm1814Protocol.h"
#include "protocols/Tm1914Protocol.h"
#include "protocols/Ws2812xProtocol.h"
#include "transports/OneWireEncoding.h"

namespace lw::protocols
{

namespace detail
{

  template <typename TProtocolCandidate, typename = void> struct ResolveProtocolType
  {
    using Type = TProtocolCandidate;
  };

  template <typename TProtocolCandidate> struct ResolveProtocolType<TProtocolCandidate, std::void_t<typename TProtocolCandidate::ProtocolType>>
  {
    using Type = typename TProtocolCandidate::ProtocolType;
  };

  template <typename TWrappedSpec, typename TWrappedSettings, typename = void> struct WrappedSpecHasNormalizeSettings : std::false_type
  {
  };

  template <typename TWrappedSpec, typename TWrappedSettings>
  struct WrappedSpecHasNormalizeSettings<TWrappedSpec, TWrappedSettings, std::void_t<decltype(TWrappedSpec::normalizeSettings(std::declval<TWrappedSettings>()))>> : std::true_type
  {
  };

} // namespace detail

template <typename TInterfaceColor = lw::Rgbw8Color, typename TDefaultChannelOrder = lw::ChannelOrder::BGR> struct DotStar
{
  using InterfaceColorType = TInterfaceColor;
  using DefaultChannelOrder = TDefaultChannelOrder;

  using ProtocolType = lw::protocols::Apa102Protocol<TInterfaceColor>;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    settings.channelOrder = TDefaultChannelOrder::value;
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::template normalizeForColor<ColorType>(std::move(settings), TDefaultChannelOrder::value); }
};

template <typename TInterfaceColor = lw::Rgbw16Color, typename TDefaultChannelOrder = lw::ChannelOrder::BGR> struct Hd108
{
  using InterfaceColorType = TInterfaceColor;
  using DefaultChannelOrder = TDefaultChannelOrder;

  using ProtocolType = lw::protocols::Hd108Protocol<TInterfaceColor>;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    settings.channelOrder = TDefaultChannelOrder::value;
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::template normalizeForColor<ColorType>(std::move(settings), TDefaultChannelOrder::value); }
};

template <typename TColor = lw::Rgbw8Color> struct None
{
  using ColorType = TColor;
  using ProtocolType = lw::protocols::NilProtocol<TColor>;
  using SettingsType = typename ProtocolType::SettingsType;

  static SettingsType defaultSettings() { return SettingsType{}; }

  static SettingsType normalizeSettings(SettingsType settings) { return settings; }
};

template <typename TWrappedProtocolSpec = None<lw::Rgbw8Color>> struct Debug
{
  using WrappedProtocolType = typename detail::ResolveProtocolType<TWrappedProtocolSpec>::Type;
  using ProtocolType = lw::protocols::DebugProtocol<WrappedProtocolType>;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    settings.wrapped = normalizeWrappedSettings(settings.wrapped);
    return settings;
  }

  static SettingsType normalizeSettings(SettingsType settings)
  {
    settings.wrapped = normalizeWrappedSettings(std::move(settings.wrapped));
    return settings;
  }

private:
  using WrappedSettingsType = typename WrappedProtocolType::SettingsType;

  static WrappedSettingsType normalizeWrappedSettings(WrappedSettingsType settings)
  {
    if constexpr (detail::WrappedSpecHasNormalizeSettings<TWrappedProtocolSpec, WrappedSettingsType>::value)
    {
      return TWrappedProtocolSpec::normalizeSettings(std::move(settings));
    }

    return settings;
  }
};

template <typename TInterfaceColor = lw::Rgbw8Color> struct Tm1814
{
  using ColorType = TInterfaceColor;
  using ProtocolType = lw::protocols::Tm1814ProtocolT<TInterfaceColor>;
  using SettingsType = typename ProtocolType::SettingsType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::template normalizeForColor<ColorType>(std::move(settings), "WRGB"); }
};

template <typename TInterfaceColor = lw::Rgbw8Color> struct Tm1914
{
  using ColorType = TInterfaceColor;
  using ProtocolType = lw::protocols::Tm1914ProtocolT<TInterfaceColor>;
  using SettingsType = typename ProtocolType::SettingsType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::template normalizeForColor<ColorType>(std::move(settings), lw::ChannelOrder::GRB::value); }
};

template <typename TInterfaceColor = lw::Color, typename TDefaultChannelOrder = lw::ChannelOrder::GRB, const transports::OneWireTiming* TDefaultTiming = &lw::transports::timing::Generic800,
          size_t NStripChannels = 3, bool TIdleHigh = false>
struct Ws2812x
{
  using InterfaceColorType = TInterfaceColor;
  using DefaultChannelOrder = TDefaultChannelOrder;

  using ProtocolType = lw::protocols::Ws2812xProtocol<InterfaceColorType, NStripChannels>;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static constexpr transports::OneWireTiming defaultTiming() { return (TDefaultTiming != nullptr) ? *TDefaultTiming : lw::transports::timing::Ws2812x; }

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    settings.channelOrder = TDefaultChannelOrder::value;
    settings.timing = defaultTiming();
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings)
  {
    settings.idleHigh = static_cast<bool>(TIdleHigh);
    return SettingsType::template normalizeForColor<ColorType>(std::move(settings), TDefaultChannelOrder::value);
  }
};

} // namespace lw::protocols
