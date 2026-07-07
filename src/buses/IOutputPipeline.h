#pragma once

#include <cstdint>

#include "colors/Color.h"
#include "core/Compat.h"

namespace lw::buses
{

class IOutputPipeline
{
public:
  using BrightnessType = lw::colors::ColorComponent;

  virtual ~IOutputPipeline() = default;

  virtual void begin() = 0;

  virtual bool isReadyToUpdate() const = 0;

  virtual void write(span<const lw::colors::Color> colors, BrightnessType brightness) = 0;

  virtual bool alwaysUpdate() const { return false; }
};

} // namespace lw::buses
