#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>
#include <algorithm>

#include "IProtocol.h"

namespace lw::protocols
{

struct Sm168xProtocolSettings : public ProtocolSettings
{
  const char* channelOrder = ChannelOrder::RGB::value;
};

class Sm168xProtocol : public IProtocol
{
public:
  using SettingsType = Sm168xProtocolSettings;

  static_assert((std::is_same_v<lw::colors::ColorComponent, uint8_t> || std::is_same_v<lw::colors::ColorComponent, uint16_t>), "Sm168xProtocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType& settings) { return (static_cast<size_t>(pixelCount) * resolveChannelCount(settings.channelOrder)) + SettingsSize; }

  Sm168xProtocol(PixelCount pixelCount, SettingsType settings)
      : IProtocol(pixelCount), _settings{std::move(settings)}, _channelCount{resolveChannelCount(_settings.channelOrder)}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings))
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

    serializePixels(colors);
    encodeSettings();
  }

  ProtocolSettings& settings() override { return _settings; }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    if (type == RuntimeConfig::Brightness && value != nullptr)
    {
      _gainValue = *static_cast<uint8_t*>(value);
    }
  }

  void* getRuntimeConfig(RuntimeConfig type) override
  {
    if (type == RuntimeConfig::Brightness)
    {
      return &_gainValue;
    }
    return nullptr;
  }

private:
  static constexpr uint8_t normalizeGainValue(uint8_t gain, uint8_t maxValue) { return static_cast<uint8_t>((static_cast<uint16_t>(gain) * static_cast<uint16_t>(maxValue) + 127u) / 255u); }

  static constexpr size_t SettingsSize = 2;

  static constexpr size_t resolveChannelCount(const char* channelOrder)
  {
    if (channelOrder == nullptr || channelOrder[0] == '\0')
    {
      return ChannelOrder::RGB::length;
    }

    return std::min(std::char_traits<char>::length(channelOrder), size_t{4});
  }

  static constexpr uint8_t maxGain() { return 0x0f; }

  static constexpr size_t channelIndexFromTag(char channel)
  {
    switch (channel)
    {
      case 'R':
      case 'r':
        return 0;

      case 'G':
      case 'g':
        return 1;

      case 'B':
      case 'b':
        return 2;

      case 'W':
      case 'w':
        return 3;

      default:
        return 0;
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

  void serializePixels(span<const lw::colors::Color> colors)
  {
    size_t offset = 0;
    const size_t payloadSize = _frameBuffer.size() - SettingsSize;
    std::fill(_frameBuffer.begin(), _frameBuffer.begin() + payloadSize, 0);

    const size_t maxPixels = (_channelCount == 0) ? 0 : (payloadSize / _channelCount);
    const size_t pixelLimit = std::min(std::min(colors.size(), maxPixels), static_cast<size_t>(this->pixelCount()));
    for (size_t index = 0; index < pixelLimit; ++index)
    {
      const auto& color = colors[index];
      for (size_t channel = 0; channel < _channelCount; ++channel)
      {
        _frameBuffer[offset++] = toWireComponent8(color[_settings.channelOrder[channel]]);
      }
    }
  }

  void encodeSettings()
  {
    uint8_t ic[5] = {0, 0, 0, 0, 0};
    const uint8_t gain = maskedGain();
    for (size_t channel = 0; channel < _channelCount; ++channel)
    {
      ic[channel] = gain;
    }

    uint8_t* encoded = _frameBuffer.data() + (_frameBuffer.size() - SettingsSize);

    if (_channelCount == 3)
    {
      encoded[0] = ic[0];
      encoded[1] = static_cast<uint8_t>((ic[1] << 4) | ic[2]);
    }
    else
    {
      encoded[0] = static_cast<uint8_t>((ic[0] << 4) | ic[1]);
      encoded[1] = static_cast<uint8_t>((ic[2] << 4) | ic[3]);
    }
  }

  uint8_t maskedGain() const { return normalizeGainValue(_gainValue, maxGain()); }

  SettingsType _settings;
  size_t _channelCount{0};
  size_t _requiredBufferSize{0};
  span<uint8_t> _frameBuffer{};
  uint8_t _gainValue{0xff};
};

} // namespace lw::protocols
