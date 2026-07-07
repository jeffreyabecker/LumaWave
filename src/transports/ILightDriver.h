#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

#include "colors/Color.h"
#include "core/Compat.h"

namespace lw::transports
{

class ILightDriver
{
public:
  using ColorType = lw::colors::Color;
  using BrightnessType = lw::colors::ColorComponent;

  virtual ~ILightDriver() = default;

  virtual void begin() = 0;
  virtual bool isReadyToUpdate() const = 0;
  virtual void write(const ColorType& color) = 0;
  virtual void write(const ColorType& color, BrightnessType brightness)
  {
    (void)brightness;
    write(color);
  }
};

struct LightDriverSettingsBase
{
};

template <typename TDriver, typename = void> struct LightDriverLikeImpl : std::false_type
{
};

template <typename TDriver>
struct LightDriverLikeImpl<TDriver, std::void_t<typename TDriver::ColorType, typename TDriver::LightDriverSettingsType>> : std::integral_constant<bool, std::is_convertible_v<TDriver*, ILightDriver*>>
{
};

template <typename TDriver> static constexpr bool LightDriverLike = LightDriverLikeImpl<TDriver>::value;

template <typename TDriver> static constexpr bool SettingsConstructibleLightDriverLike = LightDriverLike<TDriver> && std::is_constructible_v<TDriver, typename TDriver::LightDriverSettingsType>;

} // namespace lw::transports
