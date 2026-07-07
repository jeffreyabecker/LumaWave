#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <type_traits>

#include "IProtocol.h"

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
template <typename TInterfaceColor = Rgbw8Color, typename TStripColor = TInterfaceColor> class Ws2801ProtocolT : public IProtocol<TInterfaceColor>
{
public:
  using InterfaceColorType = TInterfaceColor;
  using StripColorType = TStripColor;
  using SettingsType = Ws2801ProtocolSettings;

  static_assert((std::is_same_v<typename InterfaceColorType::ComponentType, uint8_t> || std::is_same_v<typename InterfaceColorType::ComponentType, uint16_t>),
                "Ws2801Protocol requires uint8_t or uint16_t interface components.");
  static_assert((std::is_same_v<typename StripColorType::ComponentType, uint8_t> || std::is_same_v<typename StripColorType::ComponentType, uint16_t>), "Ws2801Protocol requires uint8_t or uint16_t strip components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return static_cast<size_t>(pixelCount) * BytesPerPixel; }

  Ws2801ProtocolT(PixelCount pixelCount, SettingsType settings) : IProtocol<InterfaceColorType>(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void begin() override {}

  void update(span<const InterfaceColorType> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    // Serialize: raw 3-byte channel data in configured order
    size_t offset = 0;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      for (size_t channel = 0; channel < BytesPerPixel; ++channel)
      {
        const auto component = color[_settings.channelOrder[channel]];
        _byteBuffer[offset++] = toWireComponent8(toStripComponent(component));
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

private:
  static constexpr size_t BytesPerPixel = ChannelOrder::RGB::length;

  static constexpr typename StripColorType::ComponentType toStripComponent(typename InterfaceColorType::ComponentType value)
  {
    if constexpr (std::is_same_v<typename StripColorType::ComponentType, typename InterfaceColorType::ComponentType>)
    {
      return value;
    }

    if constexpr (std::is_same_v<typename StripColorType::ComponentType, uint8_t>)
    {
      return static_cast<uint8_t>(value >> 8);
    }

    return static_cast<uint16_t>((static_cast<uint16_t>(value) << 8) | static_cast<uint16_t>(value));
  }

  static constexpr uint8_t toWireComponent8(typename StripColorType::ComponentType value)
  {
    if constexpr (std::is_same_v<typename StripColorType::ComponentType, uint8_t>)
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
