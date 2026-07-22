#pragma once

#include <cstddef>
#include <utility>

#include "core/Pixel.h"
#include "core/Compat.h"

namespace lw
{

inline void fillPixels(span<Pixel> pixels, const Pixel& color)
{
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = color;
  }
}

template <typename TGenerator> inline void fillPixelsIndexed(span<Pixel> pixels, TGenerator&& generator)
{
  for (size_t i = 0; i < pixels.size(); ++i)
  {
    pixels[i] = generator(i);
  }
}

} // namespace lw
