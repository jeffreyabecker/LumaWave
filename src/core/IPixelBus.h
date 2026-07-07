#pragma once

#include <cstddef>
#include <cstdint>

#include "colors/Color.h"
#include "core/Compat.h"
#include "core/PixelView.h"

namespace lw
{

class IPixelBus
{
public:
  using BrightnessType = lw::colors::ColorComponent;

  virtual ~IPixelBus() = default;

  virtual void begin() = 0;
  virtual void show() = 0;
  virtual bool isReadyToUpdate() const = 0;

  // Global brightness uses the color component range so 0 means off and the
  // component max means full-scale output without additional attenuation.
  virtual void setBrightness(BrightnessType brightness) = 0;
  virtual BrightnessType brightness() const = 0;

  virtual PixelView& pixels() = 0;
  virtual const PixelView& pixels() const = 0;
};

} // namespace lw
