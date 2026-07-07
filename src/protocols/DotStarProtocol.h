#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <algorithm>

#include "IProtocol.h"
#include "colors/Color.h"

namespace lw::protocols
{
struct Apa102ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::BGR::value;

  template <typename TColor> static Apa102ProtocolSettings normalizeForColor(Apa102ProtocolSettings settings, const char* defaultChannelOrder = ChannelOrder::BGR::value)
  {
    settings.channelOrder = lw::detail::normalizeChannelOrderForCount(settings.channelOrder, defaultChannelOrder, static_cast<size_t>(TColor::ChannelCount));
    return settings;
  }
};

struct Hd108ProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::BGR::value;

  template <typename TColor> static Hd108ProtocolSettings normalizeForColor(Hd108ProtocolSettings settings, const char* defaultChannelOrder = ChannelOrder::BGR::value)
  {
    settings.channelOrder = lw::detail::normalizeChannelOrderForCount(settings.channelOrder, defaultChannelOrder, static_cast<size_t>(TColor::ChannelCount));
    return settings;
  }
};

template <typename TInterfaceColor = Rgbw8Color> class Apa102Protocol : public IProtocol<TInterfaceColor>, public IHaveGain
{
public:
  using SettingsType = Apa102ProtocolSettings;
  using InterfaceColorType = TInterfaceColor;

  static_assert((std::is_same_v<typename InterfaceColorType::ComponentType, uint8_t> || std::is_same_v<typename InterfaceColorType::ComponentType, uint16_t>),
                "Apa102Protocol interface color supports uint8_t or uint16_t components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&)
  {
    const size_t extraEndBytes = static_cast<size_t>((pixelCount + 15u) / 16u);
    return StartFrameSize + (static_cast<size_t>(pixelCount) * BytesPerPixel) + EndFrameFixedSize + extraEndBytes;
  }

  Apa102Protocol(PixelCount pixelCount, SettingsType settings) : IProtocol<InterfaceColorType>(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) { _gainValue = 0xff; }

  void begin() override {}

  void update(span<const InterfaceColorType> colors, span<uint8_t> buffer = span<uint8_t>{}) override
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
        _byteBuffer[offset++] = toStripComponent(color[effectiveChannelOrder[channel]]);
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

private:
  static constexpr uint8_t MaxGain = 0x1f;
  static constexpr size_t StripChannelCount = 3; // APA102 wire format: RGB (no white)
  static constexpr size_t BytesPerPixel = 1 + StripChannelCount;
  static constexpr size_t StartFrameSize = 4;
  static constexpr size_t EndFrameFixedSize = 4;

  uint8_t encodedGainByte() const { return static_cast<uint8_t>(0xe0u | normalizeGainValue(_gainValue, MaxGain)); }

  static constexpr uint8_t toStripComponent(typename InterfaceColorType::ComponentType value)
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

template <typename TInterfaceColor = Rgbw16Color> class Hd108Protocol : public IProtocol<TInterfaceColor>, public IHaveGain
{
public:
  using SettingsType = Hd108ProtocolSettings;
  using InterfaceColorType = TInterfaceColor;

  static_assert((std::is_same_v<typename InterfaceColorType::ComponentType, uint8_t> || std::is_same_v<typename InterfaceColorType::ComponentType, uint16_t>),
                "Hd108Protocol interface color supports uint8_t or uint16_t components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&) { return StartFrameSize + (static_cast<size_t>(pixelCount) * BytesPerPixel) + EndFrameSize; }

  Hd108Protocol(PixelCount pixelCount, SettingsType settings) : IProtocol<InterfaceColorType>(pixelCount), _settings{std::move(settings)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings)) { _gainValue = 0xff; }

  void begin() override {}

  void update(span<const InterfaceColorType> colors, span<uint8_t> buffer = span<uint8_t>{}) override
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
        const uint16_t value = toStripComponent(color[effectiveChannelOrder[channel]]);
        _byteBuffer[offset++] = static_cast<uint8_t>(value >> 8);
        _byteBuffer[offset++] = static_cast<uint8_t>(value & 0xFF);
      }
    }
  }

  ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

private:
  static constexpr uint8_t MaxGain = 0x1f;
  static constexpr size_t StripChannelCount = 3; // HD108 wire format: RGB (no white)
  static constexpr size_t BytesPerPixel = 2 + (StripChannelCount * 2);
  static constexpr size_t StartFrameSize = 16;
  static constexpr size_t EndFrameSize = 4;

  uint8_t encodedGainByte() const { return static_cast<uint8_t>(0xe0u | normalizeGainValue(_gainValue, MaxGain)); }

  static constexpr uint16_t toStripComponent(typename InterfaceColorType::ComponentType value)
  {
    if constexpr (std::is_same_v<typename InterfaceColorType::ComponentType, uint16_t>)
    {
      return value;
    }

    return static_cast<uint16_t>((static_cast<uint16_t>(value) << 8) | static_cast<uint16_t>(value));
  }

  SettingsType _settings;
  size_t _requiredBufferSize{0};
  span<uint8_t> _byteBuffer{};
};

} // namespace lw::protocols
