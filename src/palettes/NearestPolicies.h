#pragma once

#include "palettes/ModeEnums.h"

namespace lw::palettes
{
inline constexpr TieBreakPolicy NearestTieStable = TieBreakPolicy::Stable;
inline constexpr TieBreakPolicy NearestTieLeft = TieBreakPolicy::Left;
inline constexpr TieBreakPolicy NearestTieRight = TieBreakPolicy::Right;

} // namespace lw::palettes
