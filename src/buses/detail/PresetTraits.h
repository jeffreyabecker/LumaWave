#pragma once

#include <type_traits>

namespace lw::buses
{
class BusBuilder;
}

namespace lw::buses::detail
{

// ---------------------------------------------------------------------------
// is_preset<T> — SFINAE gate for types with configure(BusBuilder&).
//
// A Preset is any type that provides:
//   void configure(BusBuilder& builder);
//
// Protocol presets, transport presets, and shader presets all satisfy this
// contract. They are distinguished by their role (argument position in
// addStrip), not by a type tag.
// ---------------------------------------------------------------------------

template <typename T, typename = void> struct is_preset : std::false_type
{
};

template <typename T> struct is_preset<T, std::void_t<decltype(std::declval<T&>().configure(std::declval<lw::buses::BusBuilder&>()))>> : std::true_type
{
};

template <typename T> inline constexpr bool is_preset_v = is_preset<T>::value;

} // namespace lw::buses::detail
