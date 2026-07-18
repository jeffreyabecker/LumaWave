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

  virtual void begin() {}

  virtual void beginTransaction() {}

  virtual void transmitBytes(span<uint8_t> data) {}

  virtual void endTransaction() {}

  virtual bool isReadyToUpdate() const { return true; }
};

template <typename TTransportSettings, typename = void> struct TransportSettingsWithInvertImpl : std::false_type
{
};

template <typename TTransportSettings>
struct TransportSettingsWithInvertImpl<TTransportSettings, std::void_t<decltype(std::declval<TTransportSettings&>().invert)>>
    : std::integral_constant<bool, std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<TTransportSettings&>().invert)>>, bool>>
{
};

template <typename TTransportSettings> static constexpr bool TransportSettingsWithInvert = TransportSettingsWithInvertImpl<TTransportSettings>::value;

template <typename TTransport, typename = void> struct TransportLikeImpl : std::false_type
{
};

template <typename TTransport>
struct TransportLikeImpl<TTransport, std::void_t<typename TTransport::TransportSettingsType>>
    : std::integral_constant<bool, std::is_convertible_v<TTransport*, ITransport*> && TransportSettingsWithInvert<typename TTransport::TransportSettingsType>>
{
};

template <typename TTransport> static constexpr bool TransportLike = TransportLikeImpl<TTransport>::value;

template <typename TTransport> static constexpr bool SettingsConstructibleTransportLike = TransportLike<TTransport> && std::is_constructible_v<TTransport, typename TTransport::TransportSettingsType>;

} // namespace lw::transports
