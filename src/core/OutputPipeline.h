#pragma once

#include <cstdint>

#include "core/Color.h"
#include "core/Compat.h"

namespace lw::buses
{

class OutputPipeline
{
public:
  using BrightnessType = lw::colors::ColorComponent;

  virtual ~OutputPipeline() = default;

  virtual void begin() {}

  virtual bool isReadyToUpdate() const { return true; }

  virtual void write(span<const lw::colors::Color> colors, BrightnessType brightness)
  {
    (void)colors;
    (void)brightness;
  }

  virtual bool alwaysUpdate() const { return false; }
};

} // namespace lw::buses
