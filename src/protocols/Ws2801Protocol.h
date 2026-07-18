#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <type_traits>

#include "Protocol.h"

namespace lw::protocols
{

struct Ws2801ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::RGB::value;
};

// WS2801 protocol.
//
// Wire format: raw 3 bytes per pixel, full 8-bit per channel.
// No start or end frame.
// Latch: 500 ?s clock-low after last byte.
//
class Ws2801ProtocolT : public Protocol
{
public:
  using SettingsType = Ws2801ProtocolSettings;

  static_assert((std::is_same_v<lw::ColorComponent, uint8_t> || std::is_same_v<lw::ColorComponent, uint16_t>), "Ws2801Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return static_cast<size_t>(pixelCount) * BytesPerPixel; }

  Ws2801ProtocolT(PixelCount pixelCount, SettingsType settings) : Protocol(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    size_t offset = 0;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      for (size_t channel = 0; channel < BytesPerPixel; ++channel)
      {
        _byteBuffer[offset++] = toWireByte(lw::colorComponentByTag(color, _settings.channelOrder[channel]));
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

private:
  static constexpr size_t BytesPerPixel = ChannelOrder::RGB::length;

  static constexpr uint8_t toWireByte(lw::ColorComponent value)
  {
    if constexpr (std::is_same_v<lw::ColorComponent, uint8_t>)
    {
      return value;
    }

    return static_cast<uint8_t>(value >> 8);
  }

  SettingsType _settings;
  size_t _requiredBufferSize{0};
  span<uint8_t> _byteBuffer{};
};

} // namespace lw::protocols
