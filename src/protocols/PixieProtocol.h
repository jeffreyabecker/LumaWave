#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>

#include "IProtocol.h"

namespace lw::protocols
{

struct PixieProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::RGB::value;
};

template <typename TInterfaceColor = Rgbw8Color> class PixieProtocolT : public IProtocol<TInterfaceColor>
{
public:
  using InterfaceColorType = TInterfaceColor;
  using StripColorType = void; //Rgb8Color;
  using SettingsType = PixieProtocolSettings;

  static_assert((std::is_same_v<typename InterfaceColorType::ComponentType, uint8_t> || std::is_same_v<typename InterfaceColorType::ComponentType, uint16_t>),
                "PixieProtocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return static_cast<size_t>(pixelCount) * BytesPerPixel; }

  PixieProtocolT(PixelCount pixelCount, SettingsType settings) : IProtocol<InterfaceColorType>(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void begin() override {}

  void update(span<const InterfaceColorType> colors, span<uint8_t> buffer = span<uint8_t>{}) override
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
        _byteBuffer[offset++] = toWireComponent8(color[_settings.channelOrder[channel]]);
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return true; }

  size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

private:
  static constexpr size_t BytesPerPixel = ChannelOrder::RGB::length;

  static constexpr uint8_t toWireComponent8(typename InterfaceColorType::ComponentType value)
  {
    if constexpr (std::is_same_v<typename InterfaceColorType::ComponentType, uint8_t>)
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
