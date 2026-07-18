#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <algorithm>
#include <type_traits>

#include "IProtocol.h"
#include "transports/OneWireEncoding.h"
#include "transports/OneWireTiming.h"

namespace lw::protocols
{

enum class Tm1914Mode : uint8_t
{
  DinFdinAutoSwitch,
  DinOnly,
  FdinOnly
};

struct Tm1914ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::GRB::value;
  transports::OneWireTiming timing = transports::timing::Tm1914;
  uint8_t prefixResetMultiplier = 1;
  uint8_t suffixResetMultiplier = 1;
  Tm1914Mode mode = Tm1914Mode::DinOnly;

  static Tm1914ProtocolSettings normalizeForColor(Tm1914ProtocolSettings settings, const char* defaultChannelOrder = ChannelOrder::GRB::value)
  {
    settings.channelOrder = lw::detail::normalizeChannelOrder(settings.channelOrder, defaultChannelOrder);
    return settings;
  }

  template <typename TTransportSettings> static void applyTransportDefaults(const Tm1914ProtocolSettings& settings, TTransportSettings& transportSettings)
  {
    transports::normalizeOneWireTransportClockDataBitRate(settings.timing, transportSettings);
  }
};

class Tm1914ProtocolT : public IProtocol
{
public:
  using SettingsType = Tm1914ProtocolSettings;

  static_assert((std::is_same_v<lw::colors::ColorComponent, uint8_t> || std::is_same_v<lw::colors::ColorComponent, uint16_t>), "Tm1914Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType& settings)
  {
    const size_t rawBytes = SettingsSize + (static_cast<size_t>(pixelCount) * ChannelCount);
    const size_t payloadBytes = transports::OneWireEncoding::expandedPayloadSizeBytes(rawBytes, settings.timing.bitPattern());
    const size_t prefixResetBytes = transports::OneWireEncoding::computeResetBytes(settings.timing, 0, effectiveResetMultiplier(settings.prefixResetMultiplier));
    const size_t suffixResetBytes = transports::OneWireEncoding::computeResetBytes(settings.timing, 0, effectiveResetMultiplier(settings.suffixResetMultiplier));
    return prefixResetBytes + payloadBytes + suffixResetBytes;
  }

  Tm1914ProtocolT(PixelCount pixelCount, SettingsType settings)
      : IProtocol(pixelCount), _settings{std::move(settings)}, _rawDataSize(SettingsSize + (static_cast<size_t>(pixelCount) * ChannelCount)), _requiredBufferSize(requiredBufferSize(pixelCount, _settings))
  {
  }

  void begin() override {}

  void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _frameBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    encodeSettings();
    serializePixels(colors);

    const size_t encodedSize = transports::OneWireEncoding::encodeWithResets(_frameBuffer.data(), _rawDataSize, _frameBuffer.data(), _frameBuffer.size(), _settings.timing, 0,
                                                                             effectiveResetMultiplier(_settings.prefixResetMultiplier), effectiveResetMultiplier(_settings.suffixResetMultiplier), ProtocolIdleHigh);
    if (encodedSize == 0)
    {
      return;
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

private:
  static constexpr bool ProtocolIdleHigh = true;
  static constexpr size_t ChannelCount = 3;
  static constexpr size_t SettingsSize = 6;

  uint8_t encodedMode() const
  {
    switch (_settings.mode)
    {
      case Tm1914Mode::DinFdinAutoSwitch:
        return 0xff;

      case Tm1914Mode::FdinOnly:
        return 0xfa;

      case Tm1914Mode::DinOnly:
      default:
        return 0xf5;
    }
  }

  void encodeSettings()
  {
    _frameBuffer[0] = 0xff;
    _frameBuffer[1] = 0xff;
    _frameBuffer[2] = encodedMode();

    _frameBuffer[3] = static_cast<uint8_t>(~_frameBuffer[0]);
    _frameBuffer[4] = static_cast<uint8_t>(~_frameBuffer[1]);
    _frameBuffer[5] = static_cast<uint8_t>(~_frameBuffer[2]);
  }

  void serializePixels(span<const lw::colors::Color> colors)
  {
    size_t offset = SettingsSize;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      for (size_t channel = 0; channel < ChannelCount; ++channel)
      {
        _frameBuffer[offset++] = toWireComponent8(color[_settings.channelOrder[channel]]);
      }
    }
  }

  static constexpr uint8_t toWireComponent8(lw::colors::ColorComponent value)
  {
    if constexpr (std::is_same_v<lw::colors::ColorComponent, uint8_t>)
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
