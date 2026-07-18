#pragma once

#include <type_traits>
#include <utility>

#include "colors/Color.h"
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

template <typename TDefaultChannelOrder = lw::ChannelOrder::BGR> struct DotStar
{
  using DefaultChannelOrder = TDefaultChannelOrder;

  using ProtocolType = lw::protocols::Apa102Protocol;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    settings.channelOrder = TDefaultChannelOrder::value;
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::normalizeForColor(std::move(settings), TDefaultChannelOrder::value); }
};

template <typename TDefaultChannelOrder = lw::ChannelOrder::BGR> struct Hd108
{
  using DefaultChannelOrder = TDefaultChannelOrder;

  using ProtocolType = lw::protocols::Hd108Protocol;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    settings.channelOrder = TDefaultChannelOrder::value;
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::normalizeForColor(std::move(settings), TDefaultChannelOrder::value); }
};

struct None
{
  using ProtocolType = lw::protocols::IProtocol;
  using SettingsType = lw::protocols::ProtocolSettings;
  using ColorType = lw::colors::Color;

  static constexpr size_t requiredBufferSize(uint16_t, const SettingsType&) { return 0; }
  static SettingsType defaultSettings() { return SettingsType{}; }
  static SettingsType normalizeSettings(SettingsType settings) { return settings; }
};

struct Tm1814
{
  using ProtocolType = lw::protocols::Tm1814ProtocolT;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::normalizeForColor(std::move(settings), "WRGB"); }
};

struct Tm1914
{
  using ProtocolType = lw::protocols::Tm1914ProtocolT;
  using SettingsType = typename ProtocolType::SettingsType;
  using ColorType = typename ProtocolType::ColorType;

  static SettingsType defaultSettings()
  {
    SettingsType settings{};
    return normalizeSettings(std::move(settings));
  }

  static SettingsType normalizeSettings(SettingsType settings) { return SettingsType::normalizeForColor(std::move(settings), lw::ChannelOrder::GRB::value); }
};

template <typename TDefaultChannelOrder = lw::ChannelOrder::GRB, const transports::OneWireTiming* TDefaultTiming = &lw::transports::timing::Generic800, bool TIdleHigh = false> struct Ws2812x
{
  using DefaultChannelOrder = TDefaultChannelOrder;

  using ProtocolType = lw::protocols::Ws2812xProtocol;
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
    return SettingsType::normalizeForColor(std::move(settings), TDefaultChannelOrder::value);
  }
};

} // namespace lw::protocols
