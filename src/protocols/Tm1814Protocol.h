#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <type_traits>

#include "Protocol.h"
#include "protocols/OneWireEncoding.h"
#include "protocols/OneWireTiming.h"

namespace lw::protocols
{

struct Tm1814CurrentSettings
{
  uint16_t redMilliAmps = 190;
  uint16_t greenMilliAmps = 190;
  uint16_t blueMilliAmps = 190;
  uint16_t whiteMilliAmps = 190;
};

struct Tm1814ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = "WRGB";
  protocols::OneWireTiming timing = protocols::timing::Tm1814;
  uint8_t prefixResetMultiplier = 1;
  uint8_t suffixResetMultiplier = 1;
  Tm1814CurrentSettings current{};

  static Tm1814ProtocolSettings normalizeForColor(Tm1814ProtocolSettings settings, const char* defaultChannelOrder = "WRGB")
  {
    settings.channelOrder = lw::detail::normalizeChannelOrder(settings.channelOrder, defaultChannelOrder);
    return settings;
  }

  template <typename TTransportSettings> static void applyTransportDefaults(const Tm1814ProtocolSettings& settings, TTransportSettings& transportSettings)
  {
    protocols::normalizeOneWireTransportClockDataBitRate(settings.timing, transportSettings);
  }
};

class Tm1814ProtocolT : public Protocol
{
public:
  using SettingsType = Tm1814ProtocolSettings;

  static_assert((std::is_same_v<lw::ColorComponent, uint8_t> || std::is_same_v<lw::ColorComponent, uint16_t>), "Tm1814Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType& settings)
  {
    const size_t rawBytes = SettingsSize + (static_cast<size_t>(pixelCount) * ChannelCount);
    const size_t payloadBytes = protocols::OneWireEncoding::expandedPayloadSizeBytes(rawBytes, settings.timing.bitPattern());
    const size_t prefixResetBytes = protocols::OneWireEncoding::computeResetBytes(settings.timing, 0, effectiveResetMultiplier(settings.prefixResetMultiplier));
    const size_t suffixResetBytes = protocols::OneWireEncoding::computeResetBytes(settings.timing, 0, effectiveResetMultiplier(settings.suffixResetMultiplier));
    return prefixResetBytes + payloadBytes + suffixResetBytes;
  }

  Tm1814ProtocolT(PixelCount pixelCount, SettingsType settings)
      : Protocol(pixelCount), _settings{std::move(settings)}, _rawDataSize(SettingsSize + (static_cast<size_t>(pixelCount) * ChannelCount)), _requiredBufferSize(requiredBufferSize(pixelCount, _settings))
  {
  }

  void begin() override {}

  void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _frameBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    encodeSettings();
    serializePixels(colors);

    const size_t encodedSize = protocols::OneWireEncoding::encodeWithResets(_frameBuffer.data(), _rawDataSize, _frameBuffer.data(), _frameBuffer.size(), _settings.timing, 0,
                                                                            effectiveResetMultiplier(_settings.prefixResetMultiplier), effectiveResetMultiplier(_settings.suffixResetMultiplier), ProtocolIdleHigh);
    if (encodedSize == 0)
    {
      return;
    }
  }

  ProtocolSettings& settings() override { return _settings; }

private:
  static constexpr bool ProtocolIdleHigh = true;
  static constexpr size_t ChannelCount = 4;
  static constexpr size_t SettingsSize = 8;
  static constexpr uint16_t MinCurrent = 65;
  static constexpr uint16_t MaxCurrent = 380;
  static constexpr uint16_t EncodeDivisor = 5;

  static uint8_t encodeCurrent(uint16_t milliAmps)
  {
    const uint16_t limited = std::clamp(milliAmps, MinCurrent, MaxCurrent);
    return static_cast<uint8_t>((limited - MinCurrent) / EncodeDivisor);
  }

  uint8_t currentForChannel(char channel) const
  {
    switch (channel)
    {
      case 'R':
      case 'r':
        return encodeCurrent(_settings.current.redMilliAmps);

      case 'G':
      case 'g':
        return encodeCurrent(_settings.current.greenMilliAmps);

      case 'B':
      case 'b':
        return encodeCurrent(_settings.current.blueMilliAmps);

      case 'W':
      case 'w':
      default:
        return encodeCurrent(_settings.current.whiteMilliAmps);
    }
  }

  void encodeSettings()
  {
    for (size_t i = 0; i < ChannelCount; ++i)
    {
      _frameBuffer[i] = currentForChannel(_settings.channelOrder[i]);
    }

    for (size_t i = 0; i < ChannelCount; ++i)
    {
      _frameBuffer[ChannelCount + i] = static_cast<uint8_t>(~_frameBuffer[i]);
    }
  }

  void serializePixels(span<const lw::Color> colors)
  {
    size_t offset = SettingsSize;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      for (size_t channel = 0; channel < ChannelCount; ++channel)
      {
        _frameBuffer[offset++] = toWireComponent8(lw::colorComponentByTag(color, _settings.channelOrder[channel]));
      }
    }
  }

  static constexpr uint8_t toWireComponent8(lw::ColorComponent value)
  {
    if constexpr (std::is_same_v<lw::ColorComponent, uint8_t>)
    {
      return value;
    }

    return static_cast<uint8_t>(value >> 8);
  }

  static constexpr uint8_t effectiveResetMultiplier(uint8_t value) { return (value == 0) ? 1 : value; }

  SettingsType _settings;
  size_t _rawDataSize{0};
  size_t _requiredBufferSize{0};
  span<uint8_t> _frameBuffer{};
};

} // namespace lw::protocols
