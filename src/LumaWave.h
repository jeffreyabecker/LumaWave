#pragma once

#include "core/Core.h"
#include "palettes/Palettes.h"
#include "transports/Transports.h"
#include "protocols/Protocols.h"
#include "buses/Busses.h"

#ifndef LW_USE_EXPLICIT_NAMESPACES

using pixel_count_t = lw::PixelCount;
using Color = lw::Color;

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

using Palette = lw::palettes::Palette;
using IPalette = lw::palettes::IPalette;

using lw::palettes::paletteSamples;
using lw::palettes::paletteTransitionSamples;
using lw::palettes::samplePalette;

using IStrip = lw::IPixelBus;
using TopologySettings = lw::TopologySettings;
using Topology = lw::Topology;
using GridMapping = lw::GridMapping;

namespace ChannelOrder
{

using RGB = lw::ChannelOrder::RGB;
using GRB = lw::ChannelOrder::GRB;
using BGR = lw::ChannelOrder::BGR;
using RGBW = lw::ChannelOrder::RGBW;
using GRBW = lw::ChannelOrder::GRBW;
using BGRW = lw::ChannelOrder::BGRW;
using WRGB = lw::ChannelOrder::WRGB;

} // namespace ChannelOrder

namespace Generator
{

using StaticStops = lw::palettes::Palette;
using DynamicStops = lw::palettes::Palette;

using Rainbow = lw::palettes::RainbowPaletteGenerator;

using TemporalRainbow = lw::palettes::TemporalRainbowPaletteGenerator;

using RandomSmooth = lw::palettes::RandomSmoothPaletteGenerator;

using RandomCycle = lw::palettes::RandomCyclePaletteGenerator;

} // namespace Generator

namespace PaletteBlend
{

inline constexpr lw::palettes::BlendMode Linear = lw::palettes::BlendMode::Linear;
inline constexpr lw::palettes::BlendMode Step = lw::palettes::BlendMode::Step;
inline constexpr lw::palettes::BlendMode HoldMidpoint = lw::palettes::BlendMode::HoldMidpoint;
inline constexpr lw::palettes::BlendMode SmoothStep = lw::palettes::BlendMode::Smoothstep;
inline constexpr lw::palettes::BlendMode Cubic = lw::palettes::BlendMode::Cubic;
inline constexpr lw::palettes::BlendMode Cosine = lw::palettes::BlendMode::Cosine;
inline constexpr lw::palettes::BlendMode GammaLinear = lw::palettes::BlendMode::GammaLinear;
inline constexpr lw::palettes::BlendMode Quantized = lw::palettes::BlendMode::Quantized;
inline constexpr lw::palettes::BlendMode DitheredLinear = lw::palettes::BlendMode::DitheredLinear;

} // namespace PaletteBlend

namespace BlendSampling
{

inline constexpr lw::palettes::TieBreakPolicy Stable = lw::palettes::TieBreakPolicy::Stable;
inline constexpr lw::palettes::TieBreakPolicy Left = lw::palettes::TieBreakPolicy::Left;
inline constexpr lw::palettes::TieBreakPolicy Right = lw::palettes::TieBreakPolicy::Right;

} // namespace BlendSampling

namespace BlendWrap
{

inline constexpr lw::palettes::WrapMode Clamp = lw::palettes::WrapMode::Clamp;
inline constexpr lw::palettes::WrapMode Circular = lw::palettes::WrapMode::Circular;
inline constexpr lw::palettes::WrapMode Mirror = lw::palettes::WrapMode::Mirror;
inline constexpr lw::palettes::WrapMode HoldFirst = lw::palettes::WrapMode::HoldFirst;
inline constexpr lw::palettes::WrapMode HoldLast = lw::palettes::WrapMode::HoldLast;
inline constexpr lw::palettes::WrapMode Blackout = lw::palettes::WrapMode::Blackout;

} // namespace BlendWrap

#endif // LW_USE_EXPLICIT_NAMESPACES
