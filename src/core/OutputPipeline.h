#pragma once

#include <cstdint>

#include "core/Color.h"
#include "core/Compat.h"

namespace lw
{

class OutputPipeline
{
public:
  virtual ~OutputPipeline() = default;

  virtual void begin() {}

  virtual bool isReadyToUpdate() const { return true; }

  virtual void write(span<const lw::Color> colors) { (void)colors; }

  virtual bool alwaysUpdate() const { return false; }
};

} // namespace lw
