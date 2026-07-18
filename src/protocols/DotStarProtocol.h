#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <algorithm>

#include "Protocol.h"
#include "core/Color.h"

namespace lw::protocols
{
struct Apa102ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::BGR::value;

  static Apa102ProtocolSettings normalizeForColor(Apa102ProtocolSettings settings, const char* defaultChannelOrder = ChannelOrder::BGR::value)
  {
    settings.channelOrder = lw::detail::normalizeChannelOrder(settings.channelOrder, defaultChannelOrder);
    return settings;
  }
};

struct Hd108ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::BGR::value;

  static Hd108ProtocolSettings normalizeForColor(Hd108ProtocolSettings settings, const char* defaultChannelOrder = ChannelOrder::BGR::value)
  {
    settings.channelOrder = lw::detail::normalizeChannelOrder(settings.channelOrder, defaultChannelOrder);
    return settings;
  }
};

class Apa102Protocol : public Protocol
{
public:
  using SettingsType = Apa102ProtocolSettings;
  using InterfaceColorType = lw::Color;

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&)
  {
    const size_t extraEndBytes = static_cast<size_t>((pixelCount + 15u) / 16u);
    return StartFrameSize + (static_cast<size_t>(pixelCount) * BytesPerPixel) + EndFrameFixedSize + extraEndBytes;
  }

  Apa102Protocol(PixelCount pixelCount, SettingsType settings) : Protocol(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    const size_t extraEndBytes = static_cast<size_t>((this->pixelCount() + 15u) / 16u);
    std::fill(_byteBuffer.begin(), _byteBuffer.begin() + StartFrameSize, 0x00);
    std::fill(_byteBuffer.end() - (EndFrameFixedSize + extraEndBytes), _byteBuffer.end(), 0x00);

    size_t offset = StartFrameSize;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    const char* effectiveChannelOrder = (_settings.channelOrder != nullptr) ? _settings.channelOrder : ChannelOrder::BGR::value;

    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      _byteBuffer[offset++] = encodedGainByte();
      for (size_t channel = 0; channel < StripChannelCount; ++channel)
      {
        _byteBuffer[offset++] = toStripComponent(lw::colorComponentByTag(color, effectiveChannelOrder[channel]));
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    if (type == RuntimeConfig::Brightness && value != nullptr)
    {
      _gainValue = *static_cast<uint8_t*>(value);
    }
  }

private:
  static constexpr uint8_t normalizeGainValue(uint8_t gain, uint8_t maxValue) { return static_cast<uint8_t>((static_cast<uint16_t>(gain) * static_cast<uint16_t>(maxValue) + 127u) / 255u); }

  static constexpr uint8_t MaxGain = 0x1f;
  static constexpr size_t StripChannelCount = 3; // APA102 wire format: RGB (no white)
  static constexpr size_t BytesPerPixel = 1 + StripChannelCount;
  static constexpr size_t StartFrameSize = 4;
  static constexpr size_t EndFrameFixedSize = 4;

  uint8_t encodedGainByte() const { return static_cast<uint8_t>(0xe0u | normalizeGainValue(_gainValue, MaxGain)); }

  static constexpr uint8_t toStripComponent(lw::ColorComponent value) { return static_cast<uint8_t>(value); }

  SettingsType _settings;
  size_t _requiredBufferSize{0};
  span<uint8_t> _byteBuffer{};
  uint8_t _gainValue{0xff};
};

class Hd108Protocol : public Protocol
{
public:
  using SettingsType = Hd108ProtocolSettings;
  using InterfaceColorType = lw::Color;

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return StartFrameSize + (static_cast<size_t>(pixelCount) * BytesPerPixel) + EndFrameSize; }

  Hd108Protocol(PixelCount pixelCount, SettingsType settings) : Protocol(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) {}

  void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    std::fill(_byteBuffer.begin(), _byteBuffer.begin() + StartFrameSize, 0x00);
    std::fill(_byteBuffer.end() - EndFrameSize, _byteBuffer.end(), 0xFF);

    size_t offset = StartFrameSize;
    const size_t pixelLimit = std::min(colors.size(), static_cast<size_t>(this->pixelCount()));
    const char* effectiveChannelOrder = (_settings.channelOrder != nullptr) ? _settings.channelOrder : ChannelOrder::BGR::value;

    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      _byteBuffer[offset++] = 0xFF;
      _byteBuffer[offset++] = encodedGainByte();

      for (size_t channel = 0; channel < StripChannelCount; ++channel)
      {
        const uint16_t value = toStripComponent(lw::colorComponentByTag(color, effectiveChannelOrder[channel]));
        _byteBuffer[offset++] = static_cast<uint8_t>(value >> 8);
        _byteBuffer[offset++] = static_cast<uint8_t>(value & 0xFF);
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    if (type == RuntimeConfig::Brightness && value != nullptr)
    {
      _gainValue = *static_cast<uint8_t*>(value);
    }
  }

private:
  static constexpr uint8_t normalizeGainValue(uint8_t gain, uint8_t maxValue) { return static_cast<uint8_t>((static_cast<uint16_t>(gain) * static_cast<uint16_t>(maxValue) + 127u) / 255u); }

  static constexpr uint8_t MaxGain = 0x1f;
  static constexpr size_t StripChannelCount = 3; // HD108 wire format: RGB (no white)
  static constexpr size_t BytesPerPixel = 2 + (StripChannelCount * 2);
  static constexpr size_t StartFrameSize = 16;
  static constexpr size_t EndFrameSize = 4;

  uint8_t encodedGainByte() const { return static_cast<uint8_t>(0xe0u | normalizeGainValue(_gainValue, MaxGain)); }

  static constexpr uint16_t toStripComponent(lw::ColorComponent value) { return static_cast<uint16_t>(value); }

  SettingsType _settings;
  size_t _requiredBufferSize{0};
  span<uint8_t> _byteBuffer{};
  uint8_t _gainValue{0xff};
};

} // namespace lw::protocols
