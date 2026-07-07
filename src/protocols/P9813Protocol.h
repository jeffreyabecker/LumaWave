#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace lw::protocols
{

struct P9813ProtocolSettings : public ProtocolSettings
{
};

// P9813 protocol (Total Control Lighting).
//
// Wire format: 4 bytes per pixel.
//   Byte 0: 0xC0 | (~B >> 6 & 3) << 4 | (~G >> 6 & 3) << 2 | (~R >> 6 & 3)
//   Byte 1: Blue
//   Byte 2: Green
//   Byte 3: Red
//
// The header byte contains inverted top-2-bits of each channel as a checksum.
// Fixed channel order: BGR in data bytes.
//
// Framing:
//   Start: 4 ? 0x00
//   End:   4 ? 0x00
//
template <typename TInterfaceColor = Rgbw8Color> class P9813ProtocolT : public IProtocol<TInterfaceColor>
{
public:
  using InterfaceColorType = TInterfaceColor;
  using StripColorType = void; //Rgb8Color;
  using SettingsType = P9813ProtocolSettings;

  static_assert((std::is_same_v<typename InterfaceColorType::ComponentType, uint8_t> || std::is_same_v<typename InterfaceColorType::ComponentType, uint16_t>),
                "P9813Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return (FrameSize * 2u) + (static_cast<size_t>(pixelCount) * BytesPerPixel); }

  P9813ProtocolT(PixelCount pixelCount, SettingsType settings) : IProtocol<InterfaceColorType>(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void begin() override {}

  void update(span<const InterfaceColorType> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    std::fill(_byteBuffer.begin(), _byteBuffer.begin() + FrameSize, 0x00);
    std::fill(_byteBuffer.end() - FrameSize, _byteBuffer.end(), 0x00);

    // Serialize: checksum prefix + BGR
    size_t offset = FrameSize;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      uint8_t r = toWireComponent8(color['R']);
      uint8_t g = toWireComponent8(color['G']);
      uint8_t b = toWireComponent8(color['B']);

      // Header: 0xC0 | inverted top-2-bits of each channel
      uint8_t header = 0xC0 | ((~b >> 6) & 0x03) << 4 | ((~g >> 6) & 0x03) << 2 | ((~r >> 6) & 0x03);

      _byteBuffer[offset++] = header;
      _byteBuffer[offset++] = b;
      _byteBuffer[offset++] = g;
      _byteBuffer[offset++] = r;
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

private:
  static constexpr size_t BytesPerPixel = 4;
  static constexpr size_t FrameSize = 4;

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
