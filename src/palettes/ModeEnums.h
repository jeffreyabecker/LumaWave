#pragma once

#include <cstdint>

namespace lw::colors::palettes
{
enum class WrapMode : uint8_t
{
    Clamp,
    Circular,
    Mirror,
    HoldFirst,
    HoldLast,
    Blackout,
};

enum class BlendMode : uint8_t
{
    Linear,
    Nearest,
    Step,
    HoldMidpoint,
    Smoothstep,
    Cubic,
    Cosine,
    GammaLinear,
    Quantized,
    DitheredLinear,
};

enum class TieBreakPolicy : uint8_t
{
    Stable,
    Left,
    Right,
};

} // namespace lw::colors::palettes