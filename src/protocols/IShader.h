#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Pixel.h"
#include "core/Compat.h"
#include "core/RuntimeConfig.h"

namespace lw::protocols
{

using lw::RuntimeConfig;

class IShader
{
public:
  virtual ~IShader() = default;

  virtual void apply(span<const lw::Color> source, span<lw::Color> dest) = 0;

  virtual void setRuntimeConfig(RuntimeConfig type, void* value)
  {
    (void)type;
    (void)value;
  }
};

} // namespace lw::protocols
