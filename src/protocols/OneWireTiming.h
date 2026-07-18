#pragma once

#include <cstdint>

namespace lw::protocols
{

enum class EncodedClockDataBitPattern : uint8_t
{
  Auto = 0,
  ThreeStep = 3,
  FourStep = 4
};

/// NRZ bit-encoding durations and reset interval for one-wire LED protocols.
/// Signal inversion is NOT part of timing. It is a separate hardware-output
/// concern handled by each platform protocol.
struct OneWireTiming
{
  uint32_t t0hNs;   // T0H ? high time for a zero bit (nanoseconds)
  uint32_t t0lNs;   // T0L ? low  time for a zero bit (nanoseconds)
  uint32_t t1hNs;   // T1H ? high time for a one  bit (nanoseconds)
  uint32_t t1lNs;   // T1L ? low  time for a one  bit (nanoseconds)
  uint32_t resetNs; // reset / latch interval (nanoseconds)
  EncodedClockDataBitPattern cadence = EncodedClockDataBitPattern::ThreeStep;

  static const OneWireTiming Ws2812x;
  static const OneWireTiming Ws2811;
  static const OneWireTiming Ws2805;
  static const OneWireTiming Sk6812;
  static const OneWireTiming Tm1814;
  static const OneWireTiming Tm1914;
  static const OneWireTiming Tm1829;
  static const OneWireTiming Apa106;
  static const OneWireTiming Tx1812;
  static const OneWireTiming Gs1903;
  static const OneWireTiming Generic800;
  static const OneWireTiming Generic400;

  template <EncodedClockDataBitPattern TCadence = EncodedClockDataBitPattern::ThreeStep> static constexpr OneWireTiming fromTargetKbps(uint32_t targetKbps, uint32_t bitTimesPerReset = 225)
  {
    if (targetKbps == 0)
    {
      return OneWireTiming{0, 0, 0, 0, 0};
    }

    uint32_t bitPeriodNs = 1000000UL / targetKbps;
    uint32_t resetNs = bitPeriodNs * bitTimesPerReset;

    if (resolveCadence(TCadence) == EncodedClockDataBitPattern::FourStep)
    {
      uint32_t t0h = bitPeriodNs / 4;
      uint32_t t1h = (bitPeriodNs * 3) / 4;

      return OneWireTiming{t0h, bitPeriodNs - t0h, t1h, bitPeriodNs - t1h, resetNs, EncodedClockDataBitPattern::FourStep};
    }

    uint32_t t0h = bitPeriodNs / 3;
    uint32_t t1h = (bitPeriodNs * 2) / 3;

    return OneWireTiming{t0h, bitPeriodNs - t0h, t1h, bitPeriodNs - t1h, resetNs, EncodedClockDataBitPattern::ThreeStep};
  }

  // Aliases ? identical timing, different chip branding
  static const OneWireTiming Ws2816;
  static const OneWireTiming Ws2813;
  static const OneWireTiming Ws2814;
  static const OneWireTiming Lc8812;

  /// Encoded transport data rate in Hz, derived from bit rate and selected encoding pattern.
  constexpr uint32_t encodedDataRateHz() const { return (1000000000UL / (t0hNs + t0lNs)) * static_cast<uint32_t>(bitPattern()); }

  constexpr EncodedClockDataBitPattern bitPattern() const { return resolveCadence(cadence); }

private:
  static constexpr EncodedClockDataBitPattern resolveCadence(EncodedClockDataBitPattern value)
  {
    return (value == EncodedClockDataBitPattern::FourStep) ? EncodedClockDataBitPattern::FourStep : EncodedClockDataBitPattern::ThreeStep;
  }
};

inline constexpr OneWireTiming OneWireTiming::Ws2812x{400, 850, 800, 450, 300000};
inline constexpr OneWireTiming OneWireTiming::Ws2811{500, 2000, 1200, 1300, 50000};
inline constexpr OneWireTiming OneWireTiming::Ws2805{300, 790, 790, 300, 300000};
inline constexpr OneWireTiming OneWireTiming::Sk6812{400, 850, 800, 450, 80000};
inline constexpr OneWireTiming OneWireTiming::Tm1814{360, 720, 720, 360, 200000};
inline constexpr OneWireTiming OneWireTiming::Tm1914{360, 720, 720, 360, 200000};
inline constexpr OneWireTiming OneWireTiming::Tm1829{300, 800, 800, 300, 500000};
inline constexpr OneWireTiming OneWireTiming::Apa106{350, 1360, 1360, 350, 50000};
inline constexpr OneWireTiming OneWireTiming::Tx1812{300, 600, 600, 300, 80000};
inline constexpr OneWireTiming OneWireTiming::Gs1903{300, 900, 900, 300, 40000};
inline constexpr OneWireTiming OneWireTiming::Generic800{400, 850, 800, 450, 50000};
inline constexpr OneWireTiming OneWireTiming::Generic400{500, 2000, 1200, 1300, 50000};
inline constexpr OneWireTiming Generic1100 = OneWireTiming::fromTargetKbps<>(1100);

inline constexpr OneWireTiming OneWireTiming::Ws2816 = OneWireTiming::Ws2812x;
inline constexpr OneWireTiming OneWireTiming::Ws2813 = OneWireTiming::Ws2812x;
inline constexpr OneWireTiming OneWireTiming::Ws2814 = OneWireTiming::Ws2805;
inline constexpr OneWireTiming OneWireTiming::Lc8812 = OneWireTiming::Sk6812;

namespace timing
{
  inline constexpr auto& Ws2812x = OneWireTiming::Ws2812x;
  inline constexpr auto& Ws2811 = OneWireTiming::Ws2811;
  inline constexpr auto& Ws2805 = OneWireTiming::Ws2805;
  inline constexpr auto& Sk6812 = OneWireTiming::Sk6812;
  inline constexpr auto& Tm1814 = OneWireTiming::Tm1814;
  inline constexpr auto& Tm1914 = OneWireTiming::Tm1914;
  inline constexpr auto& Tm1829 = OneWireTiming::Tm1829;
  inline constexpr auto& Apa106 = OneWireTiming::Apa106;
  inline constexpr auto& Tx1812 = OneWireTiming::Tx1812;
  inline constexpr auto& Gs1903 = OneWireTiming::Gs1903;
  inline constexpr auto& Generic800 = OneWireTiming::Generic800;
  inline constexpr auto& Generic400 = OneWireTiming::Generic400;

  inline constexpr auto& Ws2816 = OneWireTiming::Ws2816;
  inline constexpr auto& Ws2813 = OneWireTiming::Ws2813;
  inline constexpr auto& Ws2814 = OneWireTiming::Ws2814;
  inline constexpr auto& Lc8812 = OneWireTiming::Lc8812;
} // namespace timing

} // namespace lw::protocols
