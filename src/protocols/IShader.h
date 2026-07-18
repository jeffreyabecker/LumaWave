#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Color.h"
#include "core/Compat.h"

namespace lw::protocols
{

class IShader
{
public:
  virtual ~IShader() = default;

  virtual void apply(span<const lw::colors::Color> source, span<lw::colors::Color> dest) = 0;

  virtual void setRuntimeConfig(RuntimeConfig type, void* value)
  {
    (void)type;
    (void)value;
  }
  virtual void* getRuntimeConfig(RuntimeConfig type)
  {
    (void)type;
    return nullptr;
  }
};

} // namespace lw::protocols
