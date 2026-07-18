#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>

#include "colors/Color.h"
#include "colors/ChannelOrder.h"
#include "core/Compat.h"
namespace lw::protocols
{

struct ProtocolSettings
{
};

class IHaveGain
{
protected:
  uint8_t _gainValue = 0;

  static constexpr uint8_t normalizeGainValue(uint8_t gain, uint8_t maxValue) { return static_cast<uint8_t>((static_cast<uint16_t>(gain) * static_cast<uint16_t>(maxValue) + 127u) / 255u); }

public:
  virtual ~IHaveGain() = default;

  virtual void setGain(uint8_t gain) { _gainValue = gain; }

  virtual uint8_t getGain() { return _gainValue; }
};

class IProtocol
{
public:
  using ColorType = lw::colors::Color;
  using SettingsType = void;
  static constexpr bool RequiresExternalBuffer = true;
  explicit IProtocol(PixelCount pixelCount = 0) : _pixelCount{pixelCount} {}

  virtual ~IProtocol() = default;

  PixelCount pixelCount() const { return _pixelCount; }

  virtual void begin() = 0;
  virtual void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) = 0;
  virtual ProtocolSettings& settings() = 0;
  virtual bool alwaysUpdate() const = 0;

protected:
  PixelCount _pixelCount;
};

template <typename TProtocol, typename = void> struct ProtocolTypeImpl : std::false_type
{
};

template <typename TProtocol> struct ProtocolTypeImpl<TProtocol, std::void_t<typename TProtocol::SettingsType>> : std::true_type
{
};

template <typename TProtocol> static constexpr bool ProtocolType = ProtocolTypeImpl<TProtocol>::value;

template <typename TProtocol> static constexpr bool ProtocolMoveConstructible = ProtocolType<TProtocol> && std::is_move_constructible_v<TProtocol>;

template <typename TProtocol, typename = void> struct ProtocolExternalBufferRequiredImpl : std::false_type
{
};

template <typename TProtocol> struct ProtocolExternalBufferRequiredImpl<TProtocol, std::void_t<decltype(TProtocol::RequiresExternalBuffer)>> : std::integral_constant<bool, static_cast<bool>(TProtocol::RequiresExternalBuffer)>
{
};

template <typename TProtocol> static constexpr bool ProtocolExternalBufferRequired = ProtocolType<TProtocol> && ProtocolExternalBufferRequiredImpl<TProtocol>::value;

template <typename TProtocol, typename = void> struct ProtocolRequiredBufferSizeComputableImpl : std::false_type
{
};

template <typename TProtocol>
struct ProtocolRequiredBufferSizeComputableImpl<TProtocol, std::void_t<decltype(TProtocol::requiredBufferSize(std::declval<PixelCount>(), std::declval<const typename TProtocol::SettingsType&>()))>>
    : std::integral_constant<bool, std::is_convertible_v<decltype(TProtocol::requiredBufferSize(std::declval<PixelCount>(), std::declval<const typename TProtocol::SettingsType&>())), size_t>>
{
};

template <typename TProtocol> static constexpr bool ProtocolRequiredBufferSizeComputable = ProtocolType<TProtocol> && ProtocolRequiredBufferSizeComputableImpl<TProtocol>::value;

template <typename TProtocol>
static constexpr bool ProtocolPixelSettingsConstructible =
    ProtocolType<TProtocol> && ProtocolMoveConstructible<TProtocol> && ProtocolExternalBufferRequired<TProtocol> && ProtocolRequiredBufferSizeComputable<TProtocol> && !std::is_same_v<typename TProtocol::SettingsType, void> &&
    std::is_move_constructible_v<typename TProtocol::SettingsType> && std::is_constructible_v<TProtocol, uint16_t, typename TProtocol::SettingsType>;

} // namespace lw::protocols
