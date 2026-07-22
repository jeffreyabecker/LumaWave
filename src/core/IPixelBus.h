#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Pixel.h"
#include "core/Compat.h"
#include "core/RuntimeConfig.h"

namespace lw
{

class IPixelBus
{
public:
  virtual ~IPixelBus() = default;

  virtual void begin() = 0;
  virtual void show() = 0;
  virtual bool isReadyToUpdate() const = 0;

  virtual span<Color>& pixels() = 0;
  virtual const span<Color>& pixels() const = 0;

  virtual void setRuntimeConfig(RuntimeConfig type, void* value)
  {
    (void)type;
    (void)value;
  }
};

} // namespace lw
