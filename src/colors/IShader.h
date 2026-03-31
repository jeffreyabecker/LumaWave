#pragma once

#include <cstdint>
#include <type_traits>

#include "Color.h"

namespace lw::shaders
{

namespace detail
{

  template <typename TColor, typename = void> struct ShaderBrightnessTraits
  {
    using Type = uint16_t;
  };

  template <typename TColor> struct ShaderBrightnessTraits<TColor, std::void_t<typename TColor::ComponentType>>
  {
    using Type = typename TColor::ComponentType;
  };

} // namespace detail

enum class BrightnessOwnership : uint8_t
{
  None,
  Owns,
  Conflict,
};

template <typename TColor> class IShader
{
public:
  using ColorType = TColor;
  using BrightnessType = typename detail::ShaderBrightnessTraits<TColor>::Type;

  virtual ~IShader() = default;

  virtual void apply(span<TColor> /*colors*/) = 0;

  virtual BrightnessOwnership brightnessOwnership() const { return BrightnessOwnership::None; }

  virtual void applyBrightness(span<TColor> /*colors*/, BrightnessType /*brightness*/) {}
};

} // namespace lw::shaders

namespace lw
{

template <typename TColor> using IShader = shaders::IShader<TColor>;

} // namespace lw
