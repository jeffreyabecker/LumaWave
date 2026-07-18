#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <memory>
#include <type_traits>
#include <utility>
#include <algorithm>

#include "Protocol.h"
#include "core/Color.h"
#include "protocols/OneWireEncoding.h"
#include "protocols/OneWireTiming.h"

namespace lw::protocols
{

struct Ws2812xProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::GRB::value;
  protocols::OneWireTiming timing = protocols::timing::Ws2812x;
  uint8_t prefixResetMultiplier = 0;
  uint8_t suffixResetMultiplier = 1;
  bool idleHigh = false;

  Ws2812xProtocolSettings() = default;

  Ws2812xProtocolSettings(const char* co, protocols::OneWireTiming t = protocols::timing::Ws2812x, uint8_t prefixMult = 0, uint8_t suffixMult = 1, bool idle = false)
      : channelOrder(co), timing(t), prefixResetMultiplier(prefixMult), suffixResetMultiplier(suffixMult), idleHigh(idle)
  {
  }

  static Ws2812xProtocolSettings normalizeForColor(Ws2812xProtocolSettings settings, const char* defaultChannelOrder = ChannelOrder::GRB::value)
  {
    settings.channelOrder = lw::detail::normalizeChannelOrder(settings.channelOrder, defaultChannelOrder);
    return settings;
  }

  template <typename TTransportSettings, typename = void> struct TransportHasInvert : std::false_type
  {
  };

  template <typename TTransportSettings> struct TransportHasInvert<TTransportSettings, std::void_t<decltype(std::declval<TTransportSettings&>().invert)>> : std::true_type
  {
  };

  template <typename TTransportSettings> static void applyTransportDefaults(const Ws2812xProtocolSettings& settings, TTransportSettings& transportSettings)
  {
    protocols::normalizeOneWireTransportClockDataBitRate(settings.timing, transportSettings);
    if constexpr (TransportHasInvert<TTransportSettings>::value)
    {
      transportSettings.invert = settings.idleHigh;
    }
  }
};

class Ws2812xProtocol : public Protocol
{
public:
  using SettingsType = Ws2812xProtocolSettings;

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType& settings)
  {
    const char* channelOrder = resolveChannelOrder(settings.channelOrder);
    const size_t channelCount = resolveChannelCount(channelOrder);
    const size_t rawBytes = pixelCount * channelCount;
    const size_t payloadBytes = protocols::OneWireEncoding::expandedPayloadSizeBytes(rawBytes, settings.timing.bitPattern());
    const size_t prefixResetBytes = protocols::OneWireEncoding::computeResetBytes(settings.timing, 0, settings.prefixResetMultiplier);
    const size_t suffixResetBytes = protocols::OneWireEncoding::computeResetBytes(settings.timing, 0, settings.suffixResetMultiplier);
    return prefixResetBytes + payloadBytes + suffixResetBytes;
  }

  template <typename TTransportSettings> static void normalizeTransportSettings(uint16_t, const SettingsType& settings, TTransportSettings& transportSettings)
  {
    protocols::normalizeOneWireTransportClockDataBitRate(settings.timing, transportSettings);
  }

  static_assert((std::is_same_v<lw::ColorComponent, uint8_t> || std::is_same_v<lw::ColorComponent, uint16_t>), "Ws2812xProtocol interface color supports uint8_t or uint16_t components.");

  Ws2812xProtocol(PixelCount pixelCount, SettingsType settings)
      : Protocol(pixelCount), _settings{std::move(settings)}, _channelOrder{resolveChannelOrder(_settings.channelOrder)}, _channelCount{resolveChannelCount(_channelOrder)}, _rawSizeData{pixelCount * _channelCount},
        _sizeData{requiredBufferSize(pixelCount, _settings)}
  {
  }

  Ws2812xProtocol(PixelCount pixelCount, const char* channelOrder) : Ws2812xProtocol{pixelCount, Ws2812xProtocolSettings(channelOrder)} {}

  ~Ws2812xProtocol() override = default;

  Ws2812xProtocol(const Ws2812xProtocol&) = delete;
  Ws2812xProtocol& operator=(const Ws2812xProtocol&) = delete;
  Ws2812xProtocol(Ws2812xProtocol&& other) noexcept
      : Protocol(other._pixelCount), _settings{std::move(other._settings)}, _channelOrder{other._channelOrder}, _channelCount{other._channelCount}, _rawSizeData{other._rawSizeData}, _sizeData{other._sizeData},
        _frameData{other._frameData}
  {
    other._pixelCount = 0;
    other._channelOrder = ChannelOrder::GRB::value;
    other._channelCount = 0;
    other._rawSizeData = 0;
    other._sizeData = 0;
    other._frameData = span<uint8_t>{};
  }

  Ws2812xProtocol& operator=(Ws2812xProtocol&& other) noexcept
  {
    if (this == &other)
    {
      return *this;
    }

    this->_pixelCount = other._pixelCount;
    _settings = std::move(other._settings);
    _channelOrder = other._channelOrder;
    _channelCount = other._channelCount;
    _rawSizeData = other._rawSizeData;
    _sizeData = other._sizeData;
    _frameData = other._frameData;

    other._pixelCount = 0;
    other._channelOrder = ChannelOrder::GRB::value;
    other._channelCount = 0;
    other._rawSizeData = 0;
    other._sizeData = 0;
    other._frameData = span<uint8_t>{};

    return *this;
  }

  void begin() override {}

  void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _sizeData)
    {
      return;
    }

    _frameData = span<uint8_t>{buffer.data(), _sizeData};

    serialize(_frameData, colors);

    const size_t encodedSize = protocols::OneWireEncoding::encodeWithResets(_frameData.data(), _rawSizeData, _frameData.data(), _frameData.size(), _settings.timing, 0, _settings.prefixResetMultiplier,
                                                                            _settings.suffixResetMultiplier, ProtocolIdleHigh);
    if (encodedSize == 0)
    {
      return;
    }
  }

  ProtocolSettings& settings() override { return _settings; }

protected:
  size_t frameSize() const { return _sizeData; }

private:
  static constexpr bool ProtocolIdleHigh = false;
  SettingsType _settings;
  static constexpr const char* resolveChannelOrder(const char* channelOrder) { return (nullptr != channelOrder) ? channelOrder : ChannelOrder::GRB::value; }

  static constexpr size_t resolveChannelCount(const char* channelOrder)
  {
    const size_t requestedCount = std::char_traits<char>::length(channelOrder);
    if (requestedCount == 0)
    {
      return ChannelOrder::GRB::length;
    }

    return std::min(requestedCount, size_t{4});
  }

  static constexpr void appendWireComponent(span<uint8_t> pixels, size_t& offset, lw::ColorComponent value)
  {
    if constexpr (std::is_same_v<lw::ColorComponent, uint8_t>)
    {
      pixels[offset++] = value;
    }
    else
    {
      pixels[offset++] = static_cast<uint8_t>(value >> 8);
    }
  }

  void serialize(span<uint8_t> pixels, span<const lw::Color> colors)
  {
    size_t offset = 0;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));

    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      for (size_t channel = 0; channel < _channelCount; ++channel)
      {
        appendWireComponent(pixels, offset, lw::colorComponentByTag(color, _channelOrder[channel]));
      }
    }
  }

  const char* _channelOrder;
  size_t _channelCount;
  size_t _rawSizeData;
  size_t _sizeData;
  span<uint8_t> _frameData{};
};

} // namespace lw::protocols
