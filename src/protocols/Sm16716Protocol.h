#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <algorithm>
#include <type_traits>

#include "IProtocol.h"

namespace lw::protocols
{

struct Sm16716ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::RGB::value;
};

// SM16716 protocol.
//
// Bit-level protocol ? NOT byte-aligned ? pre-packed into a byte buffer.
//
// Bit stream layout:
//   Start frame: 50 zero-bits
//   Per pixel:   1 HIGH bit (separator) + 3 ? 8-bit channel data = 25 bits
//
// Total bits = 50 + pixelCount ? 25
// Pre-packed into ceil(totalBits / 8) bytes, MSB-first.
//
// No end frame. Entire stream transmitted as bytes via transmitBytes().
//
class Sm16716ProtocolT : public IProtocol
{
public:
    using SettingsType = Sm16716ProtocolSettings;

  static_assert((std::is_same_v<lw::colors::ColorComponent, uint8_t> || std::is_same_v<lw::colors::ColorComponent, uint16_t>),
                "Sm16716Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return (StartFrameBits + (static_cast<size_t>(pixelCount) * BitsPerPixel) + 7u) / 8u; }

  Sm16716ProtocolT(PixelCount pixelCount, SettingsType settings) : IProtocol(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void begin() override {}

  void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    // Pack entire bit stream into byte buffer
    serialize(colors);
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

private:
  static constexpr size_t StartFrameBits = 50;
  static constexpr size_t ChannelCount = ChannelOrder::RGB::length;
  static constexpr size_t BitsPerPixel = 1 + (ChannelCount * 8);

  SettingsType _settings;
  size_t _requiredBufferSize{0};
  span<uint8_t> _byteBuffer{};

  // Set a single bit in the buffer (MSB-first ordering)
  void setBit(size_t bitPos) { _byteBuffer[bitPos / 8] |= (0x80 >> (bitPos % 8)); }

  // Pack an 8-bit value at an arbitrary bit position (MSB-first)
  void packByte(uint8_t val, size_t& bitPos)
  {
    size_t byteIdx = bitPos / 8;
    uint8_t shift = bitPos % 8;

    // Value may span two output bytes
    _byteBuffer[byteIdx] |= (val >> shift);
    if (shift > 0 && byteIdx + 1 < _byteBuffer.size())
    {
      _byteBuffer[byteIdx + 1] |= (val << (8 - shift));
    }
    bitPos += 8;
  }

  void serialize(span<const lw::colors::Color> colors)
  {
    // Clear buffer ? start frame is 50 zero-bits, so zeros are default
    std::fill(_byteBuffer.begin(), _byteBuffer.end(), 0);

    size_t bitPos = StartFrameBits; // skip 50 zero-bits

    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      // 1-bit HIGH separator
      setBit(bitPos++);

      // Channel bytes
      for (size_t channel = 0; channel < ChannelCount; ++channel)
      {
        packByte(toWireComponent8(color[_settings.channelOrder[channel]]), bitPos);
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
};

} // namespace lw::protocols
