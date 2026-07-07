#pragma once

#include "core/Core.h"
#include "colors/Colors.h"
#include "transports/Transports.h"
#include "protocols/Protocols.h"
#include "buses/Busses.h"

#ifndef LW_USE_EXPLICIT_NAMESPACES

using pixel_count_t = lw::PixelCount;
using Rgbw8Color = lw::Rgbw8Color;
using Rgbw16Color = lw::Rgbw16Color;
using Color = lw::colors::DefaultColorType;

template <typename TColor> using PixelView = lw::PixelView<TColor>;

using lw::fillPixels;
using lw::fillPixelsIndexed;

#if !LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
template <typename TProtocol, typename TTransport = lw::busses::PlatformDefaultTransport> using Strip = lw::busses::PixelBus<TProtocol, TTransport>;
#endif

template <typename TColor = lw::colors::DefaultColorType, typename TDriver = lw::transports::PlatformDefaultLightDriver<TColor>> using Light = lw::busses::LightBus<TColor, TDriver>;

template <typename TColor = lw::colors::DefaultColorType> using ReferenceLight = lw::busses::ReferenceLightBus<TColor>;

#if !LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
template <typename... TBuses> using CompositeStrip = lw::busses::CompositeBus<TBuses...>;
#endif

template <typename TColor = lw::colors::DefaultColorType> using ReferenceAggregateStrip = lw::busses::ReferenceAggregateBus<TColor>;

template <typename TColor = lw::colors::DefaultColorType> using AggregateStrip = lw::busses::AggregateBus<TColor>;

template <typename TColor = lw::colors::DefaultColorType> using Palette = lw::colors::palettes::Palette<TColor>;
template <typename TColor = lw::colors::DefaultColorType> using IPalette = lw::colors::palettes::IPalette<TColor>;

using lw::colors::palettes::paletteSamples;
using lw::colors::palettes::paletteTransitionSamples;
using lw::colors::palettes::samplePalette;

template <typename TColor = lw::colors::DefaultColorType> using IStrip = lw::IPixelBus<TColor>;
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

template <typename TColor = lw::colors::DefaultColorType> using StaticStops = lw::colors::palettes::Palette<TColor>;
template <typename TColor = lw::colors::DefaultColorType> using DynamicStops = lw::colors::palettes::Palette<TColor>;

template <typename TColor = lw::colors::DefaultColorType> using Rainbow = lw::colors::palettes::RainbowPaletteGenerator<TColor>;

template <typename TColor = lw::colors::DefaultColorType> using TemporalRainbow = lw::colors::palettes::TemporalRainbowPaletteGenerator<TColor>;

template <typename TColor = lw::colors::DefaultColorType> using RandomSmooth = lw::colors::palettes::RandomSmoothPaletteGenerator<TColor>;

template <typename TColor = lw::colors::DefaultColorType> using RandomCycle = lw::colors::palettes::RandomCyclePaletteGenerator<TColor>;

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

using APA102 = lw::protocols::DotStar<lw::Rgbw8Color, lw::ChannelOrder::BGR>;

using HD107S = APA102;

using HD108 = lw::protocols::Hd108<lw::Rgbw16Color, lw::ChannelOrder::BGR>;

using Lpd6803 = lw::protocols::Lpd6803ProtocolT<lw::Rgbw8Color>;
using Sm16716 = lw::protocols::Sm16716ProtocolT<lw::Rgbw8Color>;

template <typename TInterfaceColor = lw::Rgbw8Color, typename TStripColor = lw::Rgbw8Color> using Ws2801x = lw::protocols::Ws2801ProtocolT<TInterfaceColor, TStripColor>;

using Ws2801 = Ws2801x<>;

template <typename TInterfaceColor = lw::colors::DefaultColorType>

using Ws2812x = lw::protocols::Ws2812x<TInterfaceColor, lw::ChannelOrder::GRB, &lw::transports::timing::Generic800, false>;

using Ws2812 = Ws2812x<>;

using Apa107 = Ws2812;
using Hc2912 = Ws2812;

using Ws2811 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Ws2811, false>;

using Ws2813 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Ws2813, false>;

using Ws2813Rgbw = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::GRBW, &lw::transports::timing::Ws2813, false>;

// Ws2805 removed (RGBCW 5-channel chip, no longer supported)

using Sk6812 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::GRB, &lw::transports::timing::Sk6812, false>;
using Sk6812White = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Sk6812, false>;
using Sk6813 = Sk6812;

using Lc8812 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::GRB, &lw::transports::timing::Lc8812, false>;

using Tm1829 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Tm1829, true>;

using Tm1814 = lw::protocols::Tm1814<lw::colors::DefaultColorType>;

using Tm1914 = lw::protocols::Tm1914<lw::colors::DefaultColorType>;

using Ws2814 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGBW, &lw::transports::timing::Ws2814, false>;
using Ws2814A = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::WRGB, &lw::transports::timing::Ws2814, false>;

using Ws2815 = Ws2812;

using Ws2816 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::GRB, &lw::transports::timing::Ws2816, false>;

using Ws2818 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Generic800, false>;

using Apa106 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Apa106, false>;

using Tx1812 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Tx1812, false>;

using Gs1903 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Gs1903, false>;

using Tm1803 = lw::protocols::Ws2812x<lw::colors::DefaultColorType, lw::ChannelOrder::RGB, &lw::transports::timing::Generic400, false>;
using Tm1804 = Tm1803;
using Tm1809 = Tm1803;

} // namespace Protocols

namespace Driver
{

template <typename TColor = lw::colors::DefaultColorType> using PlatformDefault = lw::transports::PlatformDefaultLightDriver<TColor>;

} // namespace Driver

namespace Transport
{

using Default = lw::busses::PlatformDefaultTransport;
using DefaultSettings = lw::busses::PlatformDefaultTransportSettings;

} // namespace Transport

#endif // LW_USE_EXPLICIT_NAMESPACES
