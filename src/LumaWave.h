#pragma once

#include "core/Core.h"
#include "colors/Colors.h"
#include "transports/Transports.h"
#include "protocols/Protocols.h"
#include "buses/Busses.h"

#ifndef LW_USE_EXPLICIT_NAMESPACES

using pixel_count_t = lw::PixelCount;
using Color = lw::colors::Color;

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

namespace Protocols
{

using APA102 = lw::protocols::DotStar<lw::ChannelOrder::BGR>;

using HD107S = APA102;

using HD108 = lw::protocols::Hd108<lw::ChannelOrder::BGR>;

using Lpd6803 = lw::protocols::Lpd6803ProtocolT;
using Sm16716 = lw::protocols::Sm16716ProtocolT;

using Ws2801x = lw::protocols::Ws2801ProtocolT;

using Ws2801 = Ws2801x;

using Ws2812x = lw::protocols::Ws2812x<lw::ChannelOrder::GRB, &lw::transports::timing::Generic800, false>;

using Ws2812 = Ws2812x;

using Apa107 = Ws2812;
using Hc2912 = Ws2812;

using Ws2811 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Ws2811, false>;

using Ws2813 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Ws2813, false>;

using Ws2813Rgbw = lw::protocols::Ws2812x<lw::ChannelOrder::GRBW, &lw::transports::timing::Ws2813, false>;

// Ws2805 removed (RGBCW 5-channel chip, no longer supported)

using Sk6812 = lw::protocols::Ws2812x<lw::ChannelOrder::GRB, &lw::transports::timing::Sk6812, false>;
using Sk6812White = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Sk6812, false>;
using Sk6813 = Sk6812;

using Lc8812 = lw::protocols::Ws2812x<lw::ChannelOrder::GRB, &lw::transports::timing::Lc8812, false>;

using Tm1829 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Tm1829, true>;

using Tm1814 = lw::protocols::Tm1814;

using Tm1914 = lw::protocols::Tm1914;

using Ws2814 = lw::protocols::Ws2812x<lw::ChannelOrder::RGBW, &lw::transports::timing::Ws2814, false>;
using Ws2814A = lw::protocols::Ws2812x<lw::ChannelOrder::WRGB, &lw::transports::timing::Ws2814, false>;

using Ws2815 = Ws2812;

using Ws2816 = lw::protocols::Ws2812x<lw::ChannelOrder::GRB, &lw::transports::timing::Ws2816, false>;

using Ws2818 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Generic800, false>;

using Apa106 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Apa106, false>;

using Tx1812 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Tx1812, false>;

using Gs1903 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Gs1903, false>;

using Tm1803 = lw::protocols::Ws2812x<lw::ChannelOrder::RGB, &lw::transports::timing::Generic400, false>;
using Tm1804 = Tm1803;
using Tm1809 = Tm1803;

} // namespace Protocols

namespace Transport
{

// Platform-specific transport defaults are selected via make_unique<ProtocolTransportPipeline>(...)
// or by constructing concrete transports directly.

} // namespace Transport

#endif // LW_USE_EXPLICIT_NAMESPACES
