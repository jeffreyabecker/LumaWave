#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <algorithm>
#include <type_traits>

#include "Protocol.h"

namespace lw::protocols
{

struct Lpd6803ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::RGB::value;
};

// LPD6803 protocol.
//
// Wire format: 5-5-5 packed RGB into 2 bytes per pixel (big-endian).
//   Bit 15: always 1
//   Bits 14..10: channel 1 (top 5 bits)
//   Bits  9.. 5: channel 2 (top 5 bits)
//   Bits  4.. 0: channel 3 (top 5 bits)
//
// Framing:
//   Start: 4 ? 0x00
//   Pixel data: 2 bytes per pixel
//   End:   ceil(N / 8) bytes of 0x00  (1 bit per pixel)
//
class Lpd6803ProtocolT : public Protocol
{
public:
  using SettingsType = Lpd6803ProtocolSettings;

  static_assert((std::is_same_v<lw::ColorComponent, uint8_t> || std::is_same_v<lw::ColorComponent, uint16_t>), "Lpd6803Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return StartFrameSize + (static_cast<size_t>(pixelCount) * BytesPerPixel) + ((static_cast<size_t>(pixelCount) + 7u) / 8u); }

  Lpd6803ProtocolT(PixelCount pixelCount, SettingsType settings) : Protocol(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)), _endFrameSize{(pixelCount + 7u) / 8u}
  {
  }

  void begin() override {}

  void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    std::fill(_byteBuffer.begin(), _byteBuffer.begin() + StartFrameSize, 0x00);
    std::fill(_byteBuffer.end() - _endFrameSize, _byteBuffer.end(), 0x00);

    // Serialize: 5-5-5 packed into 2 bytes per pixel
    size_t offset = StartFrameSize;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      uint8_t ch1 = toWireComponent8(lw::colorComponentByTag(color, _settings.channelOrder[0])) & 0xF8;
      uint8_t ch2 = toWireComponent8(lw::colorComponentByTag(color, _settings.channelOrder[1])) & 0xF8;
      uint8_t ch3 = toWireComponent8(lw::colorComponentByTag(color, _settings.channelOrder[2])) & 0xF8;

      // Pack: 1_ccccc_ccccc_ccccc (big-endian)
      uint16_t packed = 0x8000 | (static_cast<uint16_t>(ch1) << 7) | (static_cast<uint16_t>(ch2) << 2) | (static_cast<uint16_t>(ch3) >> 3);

      _byteBuffer[offset++] = static_cast<uint8_t>(packed >> 8);
      _byteBuffer[offset++] = static_cast<uint8_t>(packed & 0xFF);
    }
  }

  ProtocolSettings& settings() override { return _settings; }

private:
  static constexpr size_t BytesPerPixel = 2;
  static constexpr size_t StartFrameSize = 4;

  static constexpr uint8_t toWireComponent8(lw::ColorComponent value)
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
  size_t _endFrameSize;
};

} // namespace lw::protocols
