#pragma once

#include <cstdint>
#include <cstddef>
#include <cctype>
#include <limits>
#include <string_view>
#include <type_traits>

#include "core/Compat.h"

namespace lw
{

static constexpr size_t PixelComponentBitDepth = static_cast<size_t>(LW_PIXEL_COMPONENT_SIZE);
static_assert(PixelComponentBitDepth == 8 || PixelComponentBitDepth == 16, "LW_PIXEL_COMPONENT_SIZE must be 8 or 16.");

using PixelComponent = std::conditional_t<(PixelComponentBitDepth == 16), uint16_t, uint8_t>;
using Pixel = std::conditional_t<(PixelComponentBitDepth == 16), uint64_t, uint32_t>;

// Bit layout (WRGB): W at top, then R, G, B at bottom
static constexpr size_t pixelBits = PixelComponentBitDepth;
static constexpr Pixel pixelMask = (pixelBits == 16) ? 0xFFFFull : 0xFFu;
static constexpr size_t shiftR = 2 * pixelBits;
static constexpr size_t shiftG = 1 * pixelBits;
static constexpr size_t shiftB = 0 * pixelBits;
static constexpr size_t shiftW = 3 * pixelBits;

// Component accessors (templated return type, defaults to PixelComponent)
template <typename T = PixelComponent> inline constexpr T pixelR(Pixel c)
{
  return static_cast<T>((c >> shiftR) & pixelMask);
}
template <typename T = PixelComponent> inline constexpr T pixelG(Pixel c)
{
  return static_cast<T>((c >> shiftG) & pixelMask);
}
template <typename T = PixelComponent> inline constexpr T pixelB(Pixel c)
{
  return static_cast<T>((c >> shiftB) & pixelMask);
}
template <typename T = PixelComponent> inline constexpr T pixelW(Pixel c)
{
  return static_cast<T>((c >> shiftW) & pixelMask);
}

// Component setters
inline void setPixelR(Pixel& c, PixelComponent v)
{
  c = (c & ~(pixelMask << shiftR)) | (static_cast<Pixel>(v) << shiftR);
}
inline void setPixelG(Pixel& c, PixelComponent v)
{
  c = (c & ~(pixelMask << shiftG)) | (static_cast<Pixel>(v) << shiftG);
}
inline void setPixelB(Pixel& c, PixelComponent v)
{
  c = (c & ~(pixelMask << shiftB)) | (static_cast<Pixel>(v) << shiftB);
}
inline void setPixelW(Pixel& c, PixelComponent v)
{
  c = (c & ~(pixelMask << shiftW)) | (static_cast<Pixel>(v) << shiftW);
}

// Construction
inline constexpr Pixel pixelFromRGBW(PixelComponent r, PixelComponent g, PixelComponent b, PixelComponent w)
{
  return (static_cast<Pixel>(w) << shiftW) | (static_cast<Pixel>(r) << shiftR) | (static_cast<Pixel>(g) << shiftG) | (static_cast<Pixel>(b) << shiftB);
}
inline constexpr Pixel pixelFromRGB(PixelComponent r, PixelComponent g, PixelComponent b)
{
  return pixelFromRGBW(r, g, b, 0);
}

// Dynamic channel-by-tag — used by protocols for configurable channel-order serialization
template <typename T = PixelComponent> inline constexpr T pixelComponentByTag(Pixel c, char tag)
{
  switch (tag)
  {
    case 'R':
    case 'r':
      return pixelR<T>(c);
    case 'G':
    case 'g':
      return pixelG<T>(c);
    case 'B':
    case 'b':
      return pixelB<T>(c);
    case 'W':
    case 'w':
      return pixelW<T>(c);
    default:
      return 0;
  }
}

// Per-channel transform: f(componentValue, charTag) -> newComponentValue
template <typename Func> inline constexpr Pixel mapChannels(Pixel p, Func&& f)
{
  return pixelFromRGBW(static_cast<PixelComponent>(f(pixelR(p), 'R')), static_cast<PixelComponent>(f(pixelG(p), 'G')), static_cast<PixelComponent>(f(pixelB(p), 'B')), static_cast<PixelComponent>(f(pixelW(p), 'W')));
}

// Two-pixel per-channel blend: f(componentA, componentB, charTag) -> newComponentValue
template <typename Func> inline constexpr Pixel mapChannels(Pixel a, Pixel b, Func&& f)
{
  return pixelFromRGBW(static_cast<PixelComponent>(f(pixelR(a), pixelR(b), 'R')), static_cast<PixelComponent>(f(pixelG(a), pixelG(b), 'G')), static_cast<PixelComponent>(f(pixelB(a), pixelB(b), 'B')),
                       static_cast<PixelComponent>(f(pixelW(a), pixelW(b), 'W')));
}

// Parsing
namespace detail
{
  inline constexpr int hexNibble(char v)
  {
    if (v >= '0' && v <= '9')
      return v - '0';
    if (v >= 'a' && v <= 'f')
      return 10 + v - 'a';
    if (v >= 'A' && v <= 'F')
      return 10 + v - 'A';
    return -1;
  }
  inline span<const char> cStringSpan(const char* t)
  {
    if (!t)
      return {};
    return {t, std::string_view(t).size()};
  }
  inline span<const char> trimWs(span<const char> t)
  {
    size_t s = 0, e = t.size();
    while (s < e && std::isspace((unsigned char)t[s]))
      ++s;
    while (e > s && std::isspace((unsigned char)t[e - 1]))
      --e;
    return t.subspan(s, e - s);
  }
  inline bool tryParseHexToken(span<const char> tok, Pixel& c)
  {
    if (tok.empty())
      return false;
    size_t cur = 0;
    if (tok[cur] == '#')
      ++cur;
    else if (tok.size() >= 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))
      cur = 2;
    constexpr size_t dpc = sizeof(PixelComponent) * 2, ed = 4 * dpc;
    if (tok.size() - cur != ed)
      return false;
    PixelComponent comps[4] = {};
    for (size_t ch = 0; ch < 4; ++ch)
    {
      PixelComponent v = 0;
      for (size_t d = 0; d < dpc; ++d)
      {
        int n = hexNibble(tok[cur]);
        if (n < 0)
          return false;
        v = (PixelComponent)((v << 4) | (PixelComponent)n);
        ++cur;
      }
      comps[ch] = v;
    }
    c = pixelFromRGBW(comps[1], comps[2], comps[3], comps[0]);
    return true;
  }
  inline bool tryParseToken(span<const char> t, size_t& cons, Pixel& c)
  {
    size_t pl = 0;
    if (!t.empty() && t[0] == '#')
      pl = 1;
    else if (t.size() >= 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
      pl = 2;
    size_t sc = pl;
    constexpr size_t dpc = sizeof(PixelComponent) * 2, ed = 4 * dpc;
    size_t dc = 0;
    while (sc < t.size() && hexNibble(t[sc]) >= 0)
    {
      ++dc;
      ++sc;
    }
    if (dc != ed)
      return false;
    if (!tryParseHexToken(t.first(sc), c))
      return false;
    cons = sc;
    return true;
  }
} // namespace detail
inline bool tryParsePixel(span<const char> t, Pixel& c)
{
  auto tr = detail::trimWs(t);
  if (tr.empty())
    return false;
  size_t cons = 0;
  if (!detail::tryParseToken(tr, cons, c))
    return false;
  if (!detail::trimWs(tr.subspan(cons)).empty())
    return false;
  return true;
}
inline bool tryParsePixel(const char* t, Pixel& c)
{
  return tryParsePixel(detail::cStringSpan(t), c);
}
// Brightness application — scalar and pixel-level overloads
template <typename TValue, typename TBrightness, typename = std::enable_if_t<std::is_integral_v<TValue> && std::is_unsigned_v<TValue> && std::is_integral_v<TBrightness> && std::is_unsigned_v<TBrightness>>>
constexpr TValue applyBrightness(TValue value, TBrightness brightness)
{
  using ScaleWide = uint64_t;
  constexpr ScaleWide brightnessMax = static_cast<ScaleWide>(std::numeric_limits<TBrightness>::max());
  if constexpr (brightnessMax == 0)
    return static_cast<TValue>(0);
  return static_cast<TValue>(((static_cast<ScaleWide>(value) * static_cast<ScaleWide>(brightness)) + (brightnessMax / 2u)) / brightnessMax);
}

inline constexpr Pixel applyBrightness(Pixel c, PixelComponent brightness)
{
  if (brightness == std::numeric_limits<PixelComponent>::max())
    return c;
  return pixelFromRGBW(applyBrightness(pixelR(c), brightness), applyBrightness(pixelG(c), brightness), applyBrightness(pixelB(c), brightness), applyBrightness(pixelW(c), brightness));
}
