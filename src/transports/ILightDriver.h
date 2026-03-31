#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

#include "core/Compat.h"

namespace lw::transports
{

namespace detail
{

template <typename TColor, typename = void> struct LightDriverBrightnessTraits
{
    using Type = uint16_t;
};

template <typename TColor> struct LightDriverBrightnessTraits<TColor, std::void_t<typename TColor::ComponentType>>
{
    using Type = typename TColor::ComponentType;
};

} // namespace detail

template <typename TColor> class ILightDriver
{
  public:
    using ColorType = TColor;
    using BrightnessType = typename detail::LightDriverBrightnessTraits<TColor>::Type;

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
struct LightDriverLikeImpl<TDriver, std::void_t<typename TDriver::ColorType, typename TDriver::LightDriverSettingsType>>
    : std::integral_constant<bool, std::is_convertible<TDriver*, ILightDriver<typename TDriver::ColorType>*>::value>
{
};

template <typename TDriver> static constexpr bool LightDriverLike = LightDriverLikeImpl<TDriver>::value;

template <typename TDriver>
static constexpr bool SettingsConstructibleLightDriverLike =
    LightDriverLike<TDriver> && std::is_constructible<TDriver, typename TDriver::LightDriverSettingsType>::value;

} // namespace lw::transports
