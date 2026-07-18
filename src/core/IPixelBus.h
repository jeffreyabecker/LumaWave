#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Color.h"
#include "core/Compat.h"

namespace lw
{

class IPixelBus
{
public:
  using BrightnessType = lw::ColorComponent;

  virtual ~IPixelBus() = default;

  virtual void begin() = 0;
  virtual void show() = 0;
  virtual bool isReadyToUpdate() const = 0;

  // Global brightness uses the color component range so 0 means off and the
  // component max means full-scale output without additional attenuation.
  virtual void setBrightness(BrightnessType brightness) = 0;
  virtual BrightnessType brightness() const = 0;

  virtual span<Color>& pixels() = 0;
  virtual const span<Color>& pixels() const = 0;
};

} // namespace lw
