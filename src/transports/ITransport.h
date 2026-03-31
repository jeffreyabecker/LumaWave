#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <type_traits>

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#endif

#include "core/Compat.h"

namespace lw::transports
{

struct TransportBrightness
{
  uint32_t value{static_cast<uint32_t>(std::numeric_limits<uint16_t>::max())};
  uint32_t max{static_cast<uint32_t>(std::numeric_limits<uint16_t>::max())};
  bool upstreamApplied{false};

  template <typename TBrightness,
            typename = std::enable_if_t<std::is_integral_v<TBrightness> && std::is_unsigned_v<TBrightness> &&
                                        !std::is_same_v<remove_cvref_t<TBrightness>, bool>>>
  static constexpr TransportBrightness from(TBrightness brightness, bool alreadyApplied = false)
  {
    return TransportBrightness{static_cast<uint32_t>(brightness),
                               static_cast<uint32_t>(std::numeric_limits<TBrightness>::max()), alreadyApplied};
  }
};

static constexpr uint8_t BitOrderLsbFirst = 0;
static constexpr uint8_t BitOrderMsbFirst = 1;

static constexpr uint8_t SpiMode0 = 0x00;
static constexpr uint8_t SpiMode1 = 0x04;
static constexpr uint8_t SpiMode2 = 0x08;
static constexpr uint8_t SpiMode3 = 0x0C;

struct TransportSettingsBase
{
  bool invert = false;
  uint32_t clockRateHz = LW_SPI_CLOCK_DEFAULT_HZ;
  uint8_t bitOrder = BitOrderMsbFirst;
  uint8_t dataMode = SpiMode0;
  int clockPin = -1;
  int dataPin = -1;
};

class ITransport
{
public:
  virtual ~ITransport() = default;

  virtual void begin() = 0;

  virtual void beginTransaction() {}

  virtual void transmitBytes(span<uint8_t> data) = 0;

  virtual void transmitBytes(span<uint8_t> data, TransportBrightness brightness)
  {
    (void)brightness;
    transmitBytes(data);
  }

  virtual void endTransaction() {}

  virtual bool isReadyToUpdate() const { return true; }
};

template <typename TTransportSettings, typename = void> struct TransportSettingsWithInvertImpl : std::false_type
{
};

template <typename TTransportSettings>
struct TransportSettingsWithInvertImpl<TTransportSettings, std::void_t<decltype(std::declval<TTransportSettings&>().invert)>>
    : std::integral_constant<bool, std::is_same<remove_cvref_t<decltype(std::declval<TTransportSettings&>().invert)>, bool>::value>
{
};

template <typename TTransportSettings> static constexpr bool TransportSettingsWithInvert = TransportSettingsWithInvertImpl<TTransportSettings>::value;

template <typename TTransport, typename = void> struct TransportLikeImpl : std::false_type
{
};

template <typename TTransport>
struct TransportLikeImpl<TTransport, std::void_t<typename TTransport::TransportSettingsType>>
    : std::integral_constant<bool, std::is_convertible<TTransport*, ITransport*>::value && TransportSettingsWithInvert<typename TTransport::TransportSettingsType>>
{
};

template <typename TTransport> static constexpr bool TransportLike = TransportLikeImpl<TTransport>::value;

template <typename TTransport> static constexpr bool SettingsConstructibleTransportLike = TransportLike<TTransport> && std::is_constructible<TTransport, typename TTransport::TransportSettingsType>::value;

} // namespace lw::transports
