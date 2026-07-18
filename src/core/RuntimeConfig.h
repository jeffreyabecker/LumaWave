#pragma once

#include <cstdint>

namespace lw
{

enum class RuntimeConfig : uint8_t
{
  Brightness = 0x00,
  Gamma = 0x02,
};

} // namespace lw
