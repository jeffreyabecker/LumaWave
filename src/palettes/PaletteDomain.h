#pragma once

#include <cstddef>
#include <cstdint>

namespace lw::palettes::detail
{

// Canonical palette stop domain constants (configurable via macro)
#ifndef LW_PALETTE_DOMAIN_MAX_INDEX
#define LW_PALETTE_DOMAIN_MAX_INDEX 255u
#endif

inline constexpr size_t PaletteDomainMaxIndex = LW_PALETTE_DOMAIN_MAX_INDEX;
inline constexpr size_t PaletteDomainSpan = PaletteDomainMaxIndex + 1u;
inline constexpr uint32_t PaletteCanonicalFractionScale = static_cast<uint32_t>(PaletteDomainSpan);
inline constexpr uint32_t PaletteCanonicalMaxFixed = static_cast<uint32_t>(PaletteDomainMaxIndex * PaletteCanonicalFractionScale);
inline constexpr uint32_t PaletteCanonicalWrapSpan = static_cast<uint32_t>(PaletteDomainSpan * PaletteCanonicalFractionScale);

} // namespace lw::palettes::detail
