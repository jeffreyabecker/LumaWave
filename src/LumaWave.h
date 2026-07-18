#pragma once

#include "core/Core.h"
#include "palettes/Palettes.h"
#include "transports/Transports.h"
#include "protocols/Protocols.h"
#include "buses/Busses.h"

#ifndef LW_USE_EXPLICIT_NAMESPACES

using pixel_count_t = lw::PixelCount;
using Color = lw::colors::Color;

using lw::colorB;
using lw::colorCompare;
using lw::colorComponentByIndex;
using lw::colorComponentByTag;
using lw::colorFromRGB;
using lw::colorFromRGBW;
using lw::colorG;
using lw::colorR;
using lw::colorW;
using lw::parseColor;
using lw::serializeColor;
using lw::setColorB;
using lw::setColorComponentByIndex;
using lw::setColorComponentByTag;
using lw::setColorG;
using lw::setColorR;
using lw::setColorW;
using lw::tryParseColor;

using lw::fillPixels;
using lw::fillPixelsIndexed;

using Bus = lw::buses::Bus;

using Palette = lw::colors::palettes::Palette;
using IPalette = lw::colors::palettes::IPalette;

using lw::colors::palettes::paletteSamples;
using lw::colors::palettes::paletteTransitionSamples;
using lw::colors::palettes::samplePalette;

using IStrip = lw::IPixelBus;
using TopologySettings = lw::TopologySettings;
using Topology = lw::Topology;
using GridMapping = lw::GridMapping;

namespace ChannelOrder
{

using RGB = lw::colors::ChannelOrder::RGB;
using GRB = lw::colors::ChannelOrder::GRB;
using BGR = lw::colors::ChannelOrder::BGR;
using RGBW = lw::colors::ChannelOrder::RGBW;
using GRBW = lw::colors::ChannelOrder::GRBW;
using BGRW = lw::colors::ChannelOrder::BGRW;
using WRGB = lw::colors::ChannelOrder::WRGB;

} // namespace ChannelOrder

namespace Generator
{

using StaticStops = lw::colors::palettes::Palette;
using DynamicStops = lw::colors::palettes::Palette;

using Rainbow = lw::colors::palettes::RainbowPaletteGenerator;

using TemporalRainbow = lw::colors::palettes::TemporalRainbowPaletteGenerator;

using RandomSmooth = lw::colors::palettes::RandomSmoothPaletteGenerator;

using RandomCycle = lw::colors::palettes::RandomCyclePaletteGenerator;

} // namespace Generator

namespace PaletteBlend
{

inline constexpr lw::colors::palettes::BlendMode Linear = lw::colors::palettes::BlendMode::Linear;
inline constexpr lw::colors::palettes::BlendMode Step = lw::colors::palettes::BlendMode::Step;
inline constexpr lw::colors::palettes::BlendMode HoldMidpoint = lw::colors::palettes::BlendMode::HoldMidpoint;
inline constexpr lw::colors::palettes::BlendMode SmoothStep = lw::colors::palettes::BlendMode::Smoothstep;
inline constexpr lw::colors::palettes::BlendMode Cubic = lw::colors::palettes::BlendMode::Cubic;
inline constexpr lw::colors::palettes::BlendMode Cosine = lw::colors::palettes::BlendMode::Cosine;
inline constexpr lw::colors::palettes::BlendMode GammaLinear = lw::colors::palettes::BlendMode::GammaLinear;
inline constexpr lw::colors::palettes::BlendMode Quantized = lw::colors::palettes::BlendMode::Quantized;
inline constexpr lw::colors::palettes::BlendMode DitheredLinear = lw::colors::palettes::BlendMode::DitheredLinear;

} // namespace PaletteBlend

namespace BlendSampling
{

inline constexpr lw::colors::palettes::TieBreakPolicy Stable = lw::colors::palettes::TieBreakPolicy::Stable;
inline constexpr lw::colors::palettes::TieBreakPolicy Left = lw::colors::palettes::TieBreakPolicy::Left;
inline constexpr lw::colors::palettes::TieBreakPolicy Right = lw::colors::palettes::TieBreakPolicy::Right;

} // namespace BlendSampling

namespace BlendWrap
{

inline constexpr lw::colors::palettes::WrapMode Clamp = lw::colors::palettes::WrapMode::Clamp;
inline constexpr lw::colors::palettes::WrapMode Circular = lw::colors::palettes::WrapMode::Circular;
inline constexpr lw::colors::palettes::WrapMode Mirror = lw::colors::palettes::WrapMode::Mirror;
inline constexpr lw::colors::palettes::WrapMode HoldFirst = lw::colors::palettes::WrapMode::HoldFirst;
inline constexpr lw::colors::palettes::WrapMode HoldLast = lw::colors::palettes::WrapMode::HoldLast;
inline constexpr lw::colors::palettes::WrapMode Blackout = lw::colors::palettes::WrapMode::Blackout;

} // namespace BlendWrap

#endif // LW_USE_EXPLICIT_NAMESPACES
