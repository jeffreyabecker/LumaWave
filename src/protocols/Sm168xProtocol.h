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

template <typename TInterfaceColor = Rgb8Color, typename TStripColor = TInterfaceColor>
class Sm168xProtocol : public IProtocol<TInterfaceColor>, public IHaveGain
{
  public:
    using InterfaceColorType = TInterfaceColor;
    using StripColorType = TStripColor;
    using SettingsType = Sm168xProtocolSettings;

    static_assert((std::is_same<typename InterfaceColorType::ComponentType, uint8_t>::value ||
                   std::is_same<typename InterfaceColorType::ComponentType, uint16_t>::value),
                  "Sm168xProtocol requires uint8_t or uint16_t interface components.");
    static_assert(std::is_same<typename StripColorType::ComponentType, uint8_t>::value,
                  "Sm168xProtocol requires uint8_t strip components.");
    static_assert(InterfaceColorType::ChannelCount >= 3 && InterfaceColorType::ChannelCount <= 5,
                  "Sm168xProtocol requires 3, 4, or 5 interface channels.");
    static_assert(StripColorType::ChannelCount >= 3 && StripColorType::ChannelCount <= 5,
                  "Sm168xProtocol requires 3, 4, or 5 strip channels.");

    static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&)
    {
        return (static_cast<size_t>(pixelCount) * StripChannelCount) + SettingsSize;
    }

    Sm168xProtocol(PixelCount pixelCount, SettingsType settings)
        : IProtocol<InterfaceColorType>(pixelCount), _settings{std::move(settings)},
          _requiredBufferSize(requiredBufferSize(pixelCount, _settings))
    {
        _gainValue = 0xff;
    }

    void begin() override {}

    void update(span<const InterfaceColorType> colors, span<uint8_t> buffer = span<uint8_t>{}) override
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

    bool alwaysUpdate() const override { return false; }

    size_t requiredBufferSizeBytes() const override { return _requiredBufferSize; }

  private:
    static constexpr size_t resolveSettingsSize(size_t channelCount)
    {
        switch (channelCount)
        {
            case 3:
            case 4:
                return 2;

            case 5:
                return 4;
        }

        return 2;
    }

    static constexpr size_t StripChannelCount = StripColorType::ChannelCount;
    static constexpr size_t SettingsSize = resolveSettingsSize(StripChannelCount);

    static constexpr uint8_t maxGain()
    {
        if constexpr (StripChannelCount == 5)
        {
            return 0x1f;
        }

        return 0x0f;
    }

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
                return (StripColorType::ChannelCount > 3) ? 3 : 0;

            case 'C':
            case 'c':
                return (StripColorType::ChannelCount > 4) ? 4 : 0;

            default:
                return 0;
        }
    }

    static constexpr uint8_t toWireComponent8(typename InterfaceColorType::ComponentType value)
    {
        if constexpr (std::is_same<typename InterfaceColorType::ComponentType, uint8_t>::value)
        {
            return value;
        }

        return static_cast<uint8_t>(value >> 8);
    }

    void serializePixels(span<const InterfaceColorType> colors)
    {
        size_t offset = 0;
        const size_t payloadSize = _frameBuffer.size() - SettingsSize;
        std::fill(_frameBuffer.begin(), _frameBuffer.begin() + payloadSize, 0);

        const size_t maxPixels = (StripChannelCount == 0) ? 0 : (payloadSize / StripChannelCount);
        const size_t pixelLimit = std::min(std::min(colors.size(), maxPixels), static_cast<size_t>(this->pixelCount()));
        for (size_t index = 0; index < pixelLimit; ++index)
        {
            const auto& color = colors[index];
            for (size_t channel = 0; channel < StripChannelCount; ++channel)
            {
                _frameBuffer[offset++] = toWireComponent8(color[_settings.channelOrder[channel]]);
            }
        }
    }

    void encodeSettings()
    {
        uint8_t ic[5] = {0, 0, 0, 0, 0};
        const uint8_t gain = maskedGain();
        for (size_t channel = 0; channel < StripChannelCount; ++channel)
        {
            ic[channel] = gain;
        }

        uint8_t* encoded = _frameBuffer.data() + (_frameBuffer.size() - SettingsSize);

        if constexpr (StripChannelCount == 3)
        {
            encoded[0] = ic[0];
            encoded[1] = static_cast<uint8_t>((ic[1] << 4) | ic[2]);
        }
        else if constexpr (StripChannelCount == 4)
        {
            encoded[0] = static_cast<uint8_t>((ic[0] << 4) | ic[1]);
            encoded[1] = static_cast<uint8_t>((ic[2] << 4) | ic[3]);
        }
        else
        {
            encoded[0] = static_cast<uint8_t>((ic[0] << 3) | (ic[1] >> 2));
            encoded[1] = static_cast<uint8_t>((ic[1] << 6) | (ic[2] << 1) | (ic[3] >> 4));
            encoded[2] = static_cast<uint8_t>((ic[3] << 4) | (ic[4] >> 1));
            encoded[3] = static_cast<uint8_t>((ic[4] << 7) | 0b00011111);
        }
    }

    uint8_t maskedGain() const { return normalizeGainValue(_gainValue, maxGain()); }

    SettingsType _settings;
    size_t _requiredBufferSize{0};
    span<uint8_t> _frameBuffer{};
};

} // namespace lw::protocols
