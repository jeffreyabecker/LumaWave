#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>

#include "colors/Color.h"
#include "protocols/ChannelOrder.h"
#include "core/Compat.h"
namespace lw::protocols
{

struct ProtocolSettings
{
};

enum class RuntimeConfig : uint8_t
{
  Brightness = 0x00,
  Gamma = 0x02,
};

class Protocol
{
public:
  using ColorType = lw::colors::Color;
  using SettingsType = void;
  static constexpr bool RequiresExternalBuffer = true;
  explicit Protocol(PixelCount pixelCount = 0) : _pixelCount{pixelCount} {}

  virtual ~Protocol() = default;

  PixelCount pixelCount() const { return _pixelCount; }

  virtual void begin() {}
  virtual void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) {}
  virtual ProtocolSettings& settings() { return _settings; }
  virtual bool alwaysUpdate() const { return false; }

  virtual void setRuntimeConfig(RuntimeConfig type, void* value)
  {
    (void)type;
    (void)value;
  }
  virtual void* getRuntimeConfig(RuntimeConfig type)
  {
    (void)type;
    return nullptr;
  }

protected:
  PixelCount _pixelCount;
  ProtocolSettings _settings{};
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
