#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Compat.h"
#include "core/PixelView.h"

namespace lw
{
namespace detail
{

template <typename TColor, typename = void> struct BusBrightnessTraits
{
  using Type = uint16_t;
};

template <typename TColor> struct BusBrightnessTraits<TColor, std::void_t<typename TColor::ComponentType>>
{
  using Type = typename TColor::ComponentType;
};

} // namespace detail

template <typename TColor> class IPixelBus
{
public:
  using BrightnessType = typename detail::BusBrightnessTraits<TColor>::Type;

  virtual ~IPixelBus() = default;

  virtual void begin() = 0;
  virtual void show() = 0;
  virtual bool isReadyToUpdate() const = 0;

  // Global brightness uses the color component range so 0 means off and the
  // component max means full-scale output without additional attenuation.
  virtual void setBrightness(BrightnessType brightness) = 0;
  virtual BrightnessType brightness() const = 0;

  virtual PixelView<TColor>& pixels() = 0;
  virtual const PixelView<TColor>& pixels() const = 0;
};

} // namespace lw
