#pragma once

namespace lw::colors::palettes::detail
{

// Canonical palette stop domain constants (configurable via macro)
#ifndef LW_PALETTE_DOMAIN_MAX_INDEX
#define LW_PALETTE_DOMAIN_MAX_INDEX 255u
#endif

inline constexpr palette_stop_index_t PaletteDomainMaxIndex = LW_PALETTE_DOMAIN_MAX_INDEX;
inline constexpr palette_logical_domain_count_t PaletteDomainSpan = PaletteDomainMaxIndex + 1u;
inline constexpr palette_canonical_fixed_t PaletteCanonicalFractionScale = PaletteDomainSpan;
inline constexpr palette_canonical_fixed_t PaletteCanonicalMaxFixed = static_cast<palette_canonical_fixed_t>(PaletteDomainMaxIndex * PaletteCanonicalFractionScale);
inline constexpr palette_canonical_fixed_t PaletteCanonicalWrapSpan = static_cast<palette_canonical_fixed_t>(PaletteDomainSpan * PaletteCanonicalFractionScale);

} // namespace lw::colors::palettes::detail
