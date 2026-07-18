#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

#include "protocols/OneWireTiming.h"

namespace lw::protocols
{
template <typename TSettings, typename = void> struct OneWireSettingsHasclockRateHz : std::false_type
{
};

template <typename TSettings> struct OneWireSettingsHasclockRateHz<TSettings, std::void_t<decltype(std::declval<TSettings&>().clockRateHz)>> : std::true_type
{
};

template <typename TSettings, typename = void> struct OneWireSettingsHasBaudRate : std::false_type
{
};

template <typename TSettings> struct OneWireSettingsHasBaudRate<TSettings, std::void_t<decltype(std::declval<TSettings&>().baudRate)>> : std::true_type
{
};

template <typename TTransportSettings> void applyOneWireEncodedRateIfUnset(uint32_t encodedRateHz, TTransportSettings& transportSettings)
{
  if constexpr (OneWireSettingsHasclockRateHz<TTransportSettings>::value)
  {
    if (transportSettings.clockRateHz == 0)
    {
      transportSettings.clockRateHz = encodedRateHz;
    }
  }

  if constexpr (OneWireSettingsHasBaudRate<TTransportSettings>::value)
  {
    if (transportSettings.baudRate == 0)
    {
      transportSettings.baudRate = encodedRateHz;
    }
  }
}

template <typename TTransportSettings> void normalizeOneWireTransportClockDataBitRate(const OneWireTiming& timing, TTransportSettings& transportSettings)
{
  applyOneWireEncodedRateIfUnset(timing.encodedDataRateHz(), transportSettings);
}

struct OneWireEncoding
{
  static constexpr uint8_t EncodedOne3Step = 0b110;
  static constexpr uint8_t EncodedZero3Step = 0b100;
  static constexpr uint8_t EncodedOne4Step = 0b1110;
  static constexpr uint8_t EncodedZero4Step = 0b1000;
  static constexpr uint64_t NsPerSecond = 1000000000ULL;

  static constexpr size_t expandedPayloadSizeBytes(size_t sourceBytes, EncodedClockDataBitPattern bitPattern) { return sourceBytes * encodedBitsPerDataBitFromPattern(bitPattern); }

  static size_t encodeStepBytesReverseInPlace(uint8_t* buffer, size_t srcSize, uint8_t encodedOne, uint8_t encodedZero, uint8_t encodedBitsPerDataBit)
  {
    if (buffer == nullptr)
    {
      return 0;
    }

    for (size_t srcIndex = srcSize; srcIndex > 0; --srcIndex)
    {
      uint8_t value = buffer[srcIndex - 1];
      uint32_t packed = 0;

      for (uint8_t bit = 0; bit < 8; ++bit)
      {
        const uint8_t encoded = (value & 0x80) ? encodedOne : encodedZero;
        value <<= 1;
        packed = (packed << encodedBitsPerDataBit) | encoded;
      }

      const size_t outBase = (srcIndex - 1) * static_cast<size_t>(encodedBitsPerDataBit);
      for (size_t byteIndex = 0; byteIndex < encodedBitsPerDataBit; ++byteIndex)
      {
        const uint8_t shift = static_cast<uint8_t>((encodedBitsPerDataBit - 1 - byteIndex) * 8);
        buffer[outBase + byteIndex] = static_cast<uint8_t>((packed >> shift) & 0xFFu);
      }
    }

    return srcSize * static_cast<size_t>(encodedBitsPerDataBit);
  }

  static size_t encodeInPlace(uint8_t* protocolData, size_t protocolDataLength, uint8_t* transportBuffer, size_t transportCapacity, const OneWireTiming& timing, bool protocolIdleHigh = false)
  {
    if (protocolData == nullptr || transportBuffer == nullptr)
    {
      return 0;
    }

    const EncodedClockDataBitPattern pattern = timing.bitPattern();
    const uint8_t bitsPerDataBit = encodedBitsPerDataBitFromPattern(pattern);
    const size_t requiredBytes = expandedPayloadSizeBytes(protocolDataLength, pattern);
    if (requiredBytes > transportCapacity)
    {
      return 0;
    }

    const uint8_t encodedOne = (pattern == EncodedClockDataBitPattern::FourStep) ? EncodedOne4Step : EncodedOne3Step;
    const uint8_t encodedZero = (pattern == EncodedClockDataBitPattern::FourStep) ? EncodedZero4Step : EncodedZero3Step;
    const uint8_t wireEncodedOne = protocolIdleHigh ? invertEncodedPattern(encodedOne, bitsPerDataBit) : encodedOne;
    const uint8_t wireEncodedZero = protocolIdleHigh ? invertEncodedPattern(encodedZero, bitsPerDataBit) : encodedZero;

    if (protocolData != transportBuffer)
    {
      std::memmove(transportBuffer, protocolData, protocolDataLength);
    }

    return encodeStepBytesReverseInPlace(transportBuffer, protocolDataLength, wireEncodedOne, wireEncodedZero, bitsPerDataBit);
  }

  static constexpr size_t computeResetBytes(const OneWireTiming& timing, uint32_t encodedClockRateHz, uint8_t resetMultiplier)
  {
    if (resetMultiplier == 0)
    {
      return 0;
    }

    const uint64_t clockRateHz = (encodedClockRateHz == 0) ? static_cast<uint64_t>(timing.encodedDataRateHz()) : static_cast<uint64_t>(encodedClockRateHz);
    if (clockRateHz == 0)
    {
      return 0;
    }

    const uint64_t resetNs = static_cast<uint64_t>(timing.resetNs) * static_cast<uint64_t>(resetMultiplier);
    const uint64_t resetBits = (resetNs * clockRateHz + (NsPerSecond - 1ULL)) / NsPerSecond;
    return static_cast<size_t>((resetBits + 7ULL) / 8ULL);
  }

  static size_t encodeWithResetBytes(uint8_t* protocolData, size_t protocolDataLength, uint8_t* transportBuffer, size_t transportCapacity, const OneWireTiming& timing, size_t prefixResetBytes, size_t suffixResetBytes,
                                     bool protocolIdleHigh = false)
  {
    if (transportBuffer == nullptr)
    {
      return 0;
    }

    const size_t payloadCapacity = expandedPayloadSizeBytes(protocolDataLength, timing.bitPattern());
    const size_t requiredCapacity = prefixResetBytes + payloadCapacity + suffixResetBytes;
    if (requiredCapacity > transportCapacity)
    {
      return 0;
    }

    const size_t payloadSize = encodeInPlace(protocolData, protocolDataLength, transportBuffer + prefixResetBytes, transportCapacity - prefixResetBytes - suffixResetBytes, timing, protocolIdleHigh);
    if (payloadSize == 0 && protocolDataLength != 0)
    {
      return 0;
    }

    const uint8_t fillByte = resetFillByte(protocolIdleHigh);
    for (size_t index = 0; index < prefixResetBytes; ++index)
    {
      transportBuffer[index] = fillByte;
    }

    const size_t suffixStart = prefixResetBytes + payloadSize;
    for (size_t index = 0; index < suffixResetBytes; ++index)
    {
      transportBuffer[suffixStart + index] = fillByte;
    }

    return suffixStart + suffixResetBytes;
  }

  static size_t encodeWithResets(uint8_t* protocolData, size_t protocolDataLength, uint8_t* transportBuffer, size_t transportCapacity, const OneWireTiming& timing, uint32_t encodedClockRateHz, uint8_t prefixResetMultiplier,
                                 uint8_t suffixResetMultiplier, bool protocolIdleHigh = false)
  {
    const size_t prefixResetBytes = computeResetBytes(timing, encodedClockRateHz, prefixResetMultiplier);
    const size_t suffixResetBytes = computeResetBytes(timing, encodedClockRateHz, suffixResetMultiplier);

    return encodeWithResetBytes(protocolData, protocolDataLength, transportBuffer, transportCapacity, timing, prefixResetBytes, suffixResetBytes, protocolIdleHigh);
  }

private:
  static constexpr uint8_t resetFillByte(bool protocolIdleHigh) { return protocolIdleHigh ? 0xFF : 0x00; }

  static constexpr uint8_t invertEncodedPattern(uint8_t pattern, uint8_t bits) { return static_cast<uint8_t>((~pattern) & ((1u << bits) - 1u)); }

  static constexpr uint8_t encodedBitsPerDataBitFromPattern(EncodedClockDataBitPattern pattern) { return static_cast<uint8_t>(pattern); }
};

} // namespace lw::protocols
