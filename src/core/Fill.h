#pragma once

#include <cstddef>
#include <utility>

#include "colors/Color.h"
#include "core/Compat.h"

namespace lw
{

inline void fillPixels(span<colors::Color> pixels, const colors::Color& color)
{
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = color;
  }
}

template <typename TGenerator> inline void fillPixelsIndexed(span<colors::Color> pixels, TGenerator&& generator)
{
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = generator(i);
  }
}

} // namespace lw
