#pragma once

#include <cstddef>
#include <utility>

#include "core/Color.h"
#include "core/Compat.h"

namespace lw
{

inline void fillPixels(span<Color> pixels, const Color& color)
{
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = color;
  }
}

template <typename TGenerator> inline void fillPixelsIndexed(span<Color> pixels, TGenerator&& generator)
{
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = generator(i);
  }
}

} // namespace lw
