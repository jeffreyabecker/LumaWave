#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "colors/ColorMath.h"
#include "core/IPixelBus.h"
#include "protocols/IProtocol.h"
#include "transports/ITransport.h"
#include "transports/Transports.h"

namespace lw::busses
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

} // namespace detail

#if defined(ARDUINO_ARCH_ESP32)
using PlatformDefaultTransport = lw::transports::esp32::Esp32I2sTransport;
#elif defined(ARDUINO_ARCH_ESP8266)
using PlatformDefaultTransport = lw::transports::esp8266::Esp8266DmaI2sTransport;
#elif defined(ARDUINO_ARCH_RP2040)
using PlatformDefaultTransport = lw::transports::rp2040::RpPioTransport;
#elif defined(ARDUINO_ARCH_NATIVE) || !defined(ARDUINO)
using PlatformDefaultTransport = lw::transports::NilTransport;
#else
#if defined(LW_HAS_SPI_TRANSPORT)
using PlatformDefaultTransport = lw::transports::SpiTransport;
#else
using PlatformDefaultTransport = lw::transports::NilTransport;
#endif
#endif

using PlatformDefaultTransportSettings = typename PlatformDefaultTransport::TransportSettingsType;

#if !LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES

template <typename TProtocol, typename TTransport = PlatformDefaultTransport> class PixelBus : public IPixelBus
{
public:
  using ProtocolSpecType = TProtocol;

  using ProtocolType = typename detail::ResolveProtocolType<ProtocolSpecType>::Type;
  using TransportType = TTransport;
  using ColorType = typename ProtocolType::ColorType;
  using BrightnessType = lw::colors::ColorComponent;
  using ProtocolSettingsType = typename ProtocolType::SettingsType;
  using TransportSettingsType = typename TransportType::TransportSettingsType;

  static_assert(!std::is_same_v<ProtocolSettingsType, void>, "Protocol settings type must not be void.");
  static_assert(std::is_convertible_v<ProtocolType*, protocols::IProtocol*>, "Protocol type must derive from IProtocol.");
  static_assert(std::is_convertible_v<TransportType*, transports::ITransport*>, "Transport type must derive from ITransport.");

  PixelBus(size_t pixelCount, ProtocolSettingsType protocolSettings, TransportSettingsType transportSettings)
      : _pixelCount(normalizePixelCount(pixelCount)), _transport(normalizeTransportSettings(std::move(transportSettings), _pixelCount, protocolSettings)),
        _protocol(makeProtocol(_pixelCount, _transport, normalizeProtocolSettings(std::move(protocolSettings)))), _rootPixels(_pixelCount), _pixels(span<lw::colors::Color>{_rootPixels.data(), _rootPixels.size()}),
        _protocolBuffer(_protocol.requiredBufferSizeBytes(), static_cast<uint8_t>(0))
  {
  }

  PixelBus(size_t pixelCount, ProtocolSettingsType protocolSettings, transports::OneWireTiming timing, TransportSettingsType transportSettings)
      : PixelBus(pixelCount, assignProtocolTimingIfPresent(std::move(protocolSettings), timing), std::move(transportSettings))
  {
  }

  template <typename = std::enable_if_t<std::is_default_constructible_v<ProtocolSettingsType>>>
  PixelBus(size_t pixelCount, TransportSettingsType transportSettings) : PixelBus(pixelCount, defaultProtocolSettings(), std::move(transportSettings))
  {
  }

  void begin() override
  {
    _transport.begin();
    _protocol.begin();
  }

  void show() override
  {
    if (!_dirty && !_protocol.alwaysUpdate())
    {
      return;
    }

    if (!_transport.isReadyToUpdate())
    {
      return;
    }

    span<const lw::colors::Color> protocolInput{};

    if (!_rootPixels.empty())
    {
      protocolInput = span<const lw::colors::Color>{_rootPixels.data(), _rootPixels.size()};
    }

    span<uint8_t> protocolBytes{};
    if (!_protocolBuffer.empty())
    {
      protocolBytes = span<uint8_t>{_protocolBuffer.data(), _protocolBuffer.size()};
    }

    // Apply bus-level global brightness scaling before protocol encoding.
    if ((_brightness != std::numeric_limits<BrightnessType>::max()) && !protocolInput.empty())
    {
      const size_t count = static_cast<size_t>(protocolInput.size());
      if (_brightnessScratch.size() != count)
      {
        _brightnessScratch.assign(count, lw::colors::Color{});
      }

      for (size_t i = 0; i < count; ++i)
      {
        const lw::colors::Color& src = protocolInput[i];
        lw::colors::Color& dst = _brightnessScratch[i];
        for (char channel : {'R', 'G', 'B', 'W'})
        {
          const auto comp = src[channel];
          dst[channel] = static_cast<typename lw::colors::ColorComponent>(lw::colors::applyBrightness(comp, _brightness));
        }
      }

      protocolInput = span<const lw::colors::Color>{_brightnessScratch.data(), _brightnessScratch.size()};
    }

    _protocol.update(protocolInput, protocolBytes);

    if (!protocolBytes.empty())
    {
      _transport.beginTransaction();
      _transport.transmitBytes(protocolBytes, transports::TransportBrightness::from(_brightness, false));
      _transport.endTransaction();
    }

    _dirty = false;
  }

  bool isReadyToUpdate() const override { return _transport.isReadyToUpdate(); }

  PixelView& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const PixelView& pixels() const override { return _pixels; }

  size_t pixelCount() const { return _pixelCount; }

  span<lw::colors::Color> rootPixels() { return span<lw::colors::Color>{_rootPixels.data(), _rootPixels.size()}; }

  span<const lw::colors::Color> rootPixels() const { return span<const lw::colors::Color>{_rootPixels.data(), _rootPixels.size()}; }

  span<uint8_t> protocolBuffer() { return span<uint8_t>{_protocolBuffer.data(), _protocolBuffer.size()}; }

  span<const uint8_t> protocolBuffer() const { return span<const uint8_t>{_protocolBuffer.data(), _protocolBuffer.size()}; }

  ProtocolType& protocol() { return _protocol; }

  const ProtocolType& protocol() const { return _protocol; }

  TransportType& transport() { return _transport; }

  const TransportType& transport() const { return _transport; }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

private:
  static size_t normalizePixelCount(size_t pixelCount)
  {
    const size_t maxPixelCount = static_cast<size_t>(std::numeric_limits<uint16_t>::max());
    return (pixelCount <= maxPixelCount) ? pixelCount : maxPixelCount;
  }

  static ProtocolType makeProtocol(size_t pixelCount, TransportType& transport, ProtocolSettingsType settings)
  {
    const uint16_t protocolPixelCount = static_cast<uint16_t>(pixelCount);
    if constexpr (std::is_constructible_v<ProtocolType, uint16_t, ProtocolSettingsType>)
    {
      return ProtocolType{protocolPixelCount, std::move(settings)};
    }
    else if constexpr (std::is_constructible_v<ProtocolType, uint16_t, ProtocolSettingsType, TransportType&>)
    {
      return ProtocolType{protocolPixelCount, std::move(settings), transport};
    }
    else
    {
      static_assert(std::is_constructible_v<ProtocolType, uint16_t, ProtocolSettingsType> || std::is_constructible_v<ProtocolType, uint16_t, ProtocolSettingsType, TransportType&>,
                    "Protocol must be constructible with settings (with or without transport reference).");
      return ProtocolType{protocolPixelCount, std::move(settings)};
    }
  }

  template <typename TSettings, typename = void> struct ProtocolSettingsHasNormalizeForColor : std::false_type
  {
  };

  template <typename TSettings> struct ProtocolSettingsHasNormalizeForColor<TSettings, std::void_t<decltype(TSettings::normalizeForColor(std::declval<TSettings>()))>> : std::true_type
  {
  };

  template <typename TSettings, typename = void> struct ProtocolSettingsHasTiming : std::false_type
  {
  };

  template <typename TSettings> struct ProtocolSettingsHasTiming<TSettings, std::void_t<decltype(std::declval<TSettings&>().timing)>> : std::true_type
  {
  };

  static ProtocolSettingsType assignProtocolTimingIfPresent(ProtocolSettingsType settings, transports::OneWireTiming timing)
  {
    if constexpr (ProtocolSettingsHasTiming<ProtocolSettingsType>::value)
    {
      settings.timing = timing;
    }

    return settings;
  }

  template <typename TProtocolSpec, typename = void> struct ProtocolSpecHasDefaultSettings : std::false_type
  {
  };

  template <typename TProtocolSpec> struct ProtocolSpecHasDefaultSettings<TProtocolSpec, std::void_t<decltype(TProtocolSpec::defaultSettings())>> : std::true_type
  {
  };

  template <typename TProtocolSpec, typename = void> struct ProtocolSpecHasNormalizeSettings : std::false_type
  {
  };

  template <typename TProtocolSpec> struct ProtocolSpecHasNormalizeSettings<TProtocolSpec, std::void_t<decltype(TProtocolSpec::normalizeSettings(std::declval<ProtocolSettingsType>()))>> : std::true_type
  {
  };

  static ProtocolSettingsType defaultProtocolSettings()
  {
    if constexpr (ProtocolSpecHasDefaultSettings<ProtocolSpecType>::value)
    {
      return ProtocolSpecType::defaultSettings();
    }

    return ProtocolSettingsType{};
  }

  static ProtocolSettingsType normalizeProtocolSettings(ProtocolSettingsType settings)
  {
    if constexpr (ProtocolSpecHasNormalizeSettings<ProtocolSpecType>::value)
    {
      return ProtocolSpecType::normalizeSettings(std::move(settings));
    }

    if constexpr (ProtocolSettingsHasNormalizeForColor<ProtocolSettingsType>::value)
    {
      return ProtocolSettingsType::normalizeForColor(std::move(settings));
    }

    return settings;
  }

  template <typename TSettings, typename = void> struct TransportSettingsHasNormalizePixelCount : std::false_type
  {
  };

  template <typename TSettings> struct TransportSettingsHasNormalizePixelCount<TSettings, std::void_t<decltype(TSettings::normalize(std::declval<TSettings>(), std::declval<PixelCount>()))>> : std::true_type
  {
  };

  template <typename TSettings, typename = void> struct TransportSettingsHasNormalize : std::false_type
  {
  };

  template <typename TSettings> struct TransportSettingsHasNormalize<TSettings, std::void_t<decltype(TSettings::normalize(std::declval<TSettings>()))>> : std::true_type
  {
  };

  template <typename TProtocolSettings, typename TTransportSettings, typename = void> struct ProtocolSettingsHasApplyTransportDefaults : std::false_type
  {
  };

  template <typename TProtocolSettings, typename TTransportSettings>
  struct ProtocolSettingsHasApplyTransportDefaults<TProtocolSettings, TTransportSettings,
                                                   std::void_t<decltype(TProtocolSettings::applyTransportDefaults(std::declval<const TProtocolSettings&>(), std::declval<TTransportSettings&>()))>> : std::true_type
  {
  };

  template <typename TProtocolCandidate, typename TProtocolSettings, typename TTransportSettings, typename = void> struct ProtocolHasNormalizeTransportSettings : std::false_type
  {
  };

  template <typename TProtocolCandidate, typename TProtocolSettings, typename TTransportSettings>
  struct ProtocolHasNormalizeTransportSettings<TProtocolCandidate, TProtocolSettings, TTransportSettings,
                                               std::void_t<decltype(TProtocolCandidate::normalizeTransportSettings(std::declval<PixelCount>(), std::declval<const TProtocolSettings&>(), std::declval<TTransportSettings&>()))>>
      : std::true_type
  {
  };

  static TransportSettingsType normalizeTransportSettings(TransportSettingsType settings, size_t pixelCount, ProtocolSettingsType protocolSettings)
  {
    const PixelCount protocolPixelCount = static_cast<PixelCount>(pixelCount);

    protocolSettings = normalizeProtocolSettings(std::move(protocolSettings));

    if constexpr (ProtocolHasNormalizeTransportSettings<ProtocolType, ProtocolSettingsType, TransportSettingsType>::value)
    {
      ProtocolType::normalizeTransportSettings(protocolPixelCount, protocolSettings, settings);
    }

    if constexpr (ProtocolSettingsHasApplyTransportDefaults<ProtocolSettingsType, TransportSettingsType>::value)
    {
      ProtocolSettingsType::applyTransportDefaults(protocolSettings, settings);
    }

    if constexpr (TransportSettingsHasNormalizePixelCount<TransportSettingsType>::value)
    {
      return TransportSettingsType::normalize(std::move(settings), protocolPixelCount);
    }

    if constexpr (TransportSettingsHasNormalize<TransportSettingsType>::value)
    {
      return TransportSettingsType::normalize(std::move(settings));
    }

    return settings;
  }

  size_t _pixelCount{0};
  TransportType _transport;
  ProtocolType _protocol;
  std::vector<lw::colors::Color> _rootPixels;
  PixelView _pixels;
  std::vector<uint8_t> _protocolBuffer;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  std::vector<lw::colors::Color> _brightnessScratch;
  bool _dirty{true};
};

#endif

} // namespace lw::busses
