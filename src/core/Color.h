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

static constexpr size_t ColorComponentBitDepth = static_cast<size_t>(LW_COLOR_COMPONENT_SIZE);
static_assert(ColorComponentBitDepth == 8 || ColorComponentBitDepth == 16, "LW_COLOR_COMPONENT_SIZE must be 8 or 16.");

using ColorComponent = std::conditional_t<(ColorComponentBitDepth == 16), uint16_t, uint8_t>;
using Color = std::conditional_t<(ColorComponentBitDepth == 16), uint64_t, uint32_t>;

// Bit layout (WRGB): W at top, then R, G, B at bottom
static constexpr size_t colorBits = ColorComponentBitDepth;
static constexpr Color colorMask = (colorBits == 16) ? 0xFFFFull : 0xFFu;
static constexpr size_t shiftR = 2 * colorBits;
static constexpr size_t shiftG = 1 * colorBits;
static constexpr size_t shiftB = 0 * colorBits;
static constexpr size_t shiftW = 3 * colorBits;

// Component accessors (templated return type, defaults to ColorComponent)
template <typename T = ColorComponent> inline constexpr T colorR(Color c)
{
  return static_cast<T>((c >> shiftR) & colorMask);
}
template <typename T = ColorComponent> inline constexpr T colorG(Color c)
{
  return static_cast<T>((c >> shiftG) & colorMask);
}
template <typename T = ColorComponent> inline constexpr T colorB(Color c)
{
  return static_cast<T>((c >> shiftB) & colorMask);
}
template <typename T = ColorComponent> inline constexpr T colorW(Color c)
{
  return static_cast<T>((c >> shiftW) & colorMask);
}

// Component setters
inline void setColorR(Color& c, ColorComponent v)
{
  c = (c & ~(colorMask << shiftR)) | (static_cast<Color>(v) << shiftR);
}
inline void setColorG(Color& c, ColorComponent v)
{
  c = (c & ~(colorMask << shiftG)) | (static_cast<Color>(v) << shiftG);
}
inline void setColorB(Color& c, ColorComponent v)
{
  c = (c & ~(colorMask << shiftB)) | (static_cast<Color>(v) << shiftB);
}
inline void setColorW(Color& c, ColorComponent v)
{
  c = (c & ~(colorMask << shiftW)) | (static_cast<Color>(v) << shiftW);
}

// Construction
inline constexpr Color colorFromRGBW(ColorComponent r, ColorComponent g, ColorComponent b, ColorComponent w)
{
  return (static_cast<Color>(w) << shiftW) | (static_cast<Color>(r) << shiftR) | (static_cast<Color>(g) << shiftG) | (static_cast<Color>(b) << shiftB);
}
inline constexpr Color colorFromRGB(ColorComponent r, ColorComponent g, ColorComponent b)
{
  return colorFromRGBW(r, g, b, 0);
}

// Dynamic channel-by-tag (replaces operator[])
template <typename T = ColorComponent> inline constexpr T colorComponentByTag(Color c, char tag)
{
  switch (tag)
  {
    case 'R':
    case 'r':
      return colorR<T>(c);
    case 'G':
    case 'g':
      return colorG<T>(c);
    case 'B':
    case 'b':
      return colorB<T>(c);
    case 'W':
    case 'w':
      return colorW<T>(c);
    default:
      return 0;
  }
}
inline void setColorComponentByTag(Color& c, char tag, ColorComponent v)
{
  switch (tag)
  {
    case 'R':
    case 'r':
      setColorR(c, v);
      break;
    case 'G':
    case 'g':
      setColorG(c, v);
      break;
    case 'B':
    case 'b':
      setColorB(c, v);
      break;
    case 'W':
    case 'w':
      setColorW(c, v);
      break;
  }
}

// Channel-by-index (0=R, 1=G, 2=B, 3=W)
template <typename T = ColorComponent> inline constexpr T colorComponentByIndex(Color c, size_t i)
{
  switch (i)
  {
    case 0:
      return colorR<T>(c);
    case 1:
      return colorG<T>(c);
    case 2:
      return colorB<T>(c);
    case 3:
      return colorW<T>(c);
    default:
      return 0;
  }
}
inline void setColorComponentByIndex(Color& c, size_t i, ColorComponent v)
{
  switch (i)
  {
    case 0:
      setColorR(c, v);
      break;
    case 1:
      setColorG(c, v);
      break;
    case 2:
      setColorB(c, v);
      break;
    case 3:
      setColorW(c, v);
      break;
  }
}

// Comparison (integer comparison works natively for == and !=)
inline constexpr int colorCompare(Color a, Color b)
{
  return a == b ? 0 : (a < b ? -1 : 1);
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
  inline bool tryParseHexToken(span<const char> tok, Color& c)
  {
    if (tok.empty())
      return false;
    size_t cur = 0;
    if (tok[cur] == '#')
      ++cur;
    else if (tok.size() >= 2 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X'))
      cur = 2;
    constexpr size_t dpc = sizeof(ColorComponent) * 2, ed = 4 * dpc;
    if (tok.size() - cur != ed)
      return false;
    Color p = 0;
    constexpr char tags[4] = {'W', 'R', 'G', 'B'};
    for (size_t ch = 0; ch < 4; ++ch)
    {
      ColorComponent v = 0;
      for (size_t d = 0; d < dpc; ++d)
      {
        int n = hexNibble(tok[cur]);
        if (n < 0)
          return false;
        v = (ColorComponent)((v << 4) | (ColorComponent)n);
        ++cur;
      }
      setColorComponentByTag(p, tags[ch], v);
    }
    c = p;
    return true;
  }
  inline bool tryParseToken(span<const char> t, size_t& cons, Color& c)
  {
    size_t pl = 0;
    if (!t.empty() && t[0] == '#')
      pl = 1;
    else if (t.size() >= 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
      pl = 2;
    size_t sc = pl;
    constexpr size_t dpc = sizeof(ColorComponent) * 2, ed = 4 * dpc;
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
inline bool tryParseColor(span<const char> t, Color& c)
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
inline bool tryParseColor(const char* t, Color& c)
{
  return tryParseColor(detail::cStringSpan(t), c);
}
inline Color parseColor(span<const char> t)
{
  Color c = 0;
  tryParseColor(t, c);
  return c;
}
inline Color parseColor(const char* t)
{
  return parseColor(detail::cStringSpan(t));
}

// Serialization
inline constexpr size_t colorSerializedLength()
{
  return 4 * sizeof(ColorComponent) * 2u;
}
inline bool serializeColor(Color c, span<char> sink)
{
  static constexpr char hx[] = "0123456789ABCDEF";
  if (!sink.data())
    return false;
  size_t rq = colorSerializedLength();
  if (sink.size() <= rq)
    return false;
  size_t o = 0;
  constexpr char ch[] = "WRGB";
  for (size_t ci = 0; ci < 4; ++ci)
  {
    auto v = colorComponentByTag(c, ch[ci]);
    for (size_t n = 0; n < sizeof(ColorComponent) * 2; ++n)
    {
      size_t sh = (sizeof(ColorComponent) * 2 - n - 1) * 4;
      sink[o++] = hx[(size_t(v) >> sh) & 0xF];
    }
  }
  sink[o] = '\0';
  return true;
}
inline bool serializeColor(Color c, char* s, size_t l)
{
  return serializeColor(c, span<char>(s, l));
}

// Brightness application — scalar and color-level overloads
template <typename TValue, typename TBrightness, typename = std::enable_if_t<std::is_integral_v<TValue> && std::is_unsigned_v<TValue> && std::is_integral_v<TBrightness> && std::is_unsigned_v<TBrightness>>>
constexpr TValue applyBrightness(TValue value, TBrightness brightness)
{
  using ScaleWide = uint64_t;
  constexpr ScaleWide brightnessMax = static_cast<ScaleWide>(std::numeric_limits<TBrightness>::max());
  if constexpr (brightnessMax == 0)
    return static_cast<TValue>(0);
  return static_cast<TValue>(((static_cast<ScaleWide>(value) * static_cast<ScaleWide>(brightness)) + (brightnessMax / 2u)) / brightnessMax);
}

inline constexpr Color applyBrightness(Color c, ColorComponent brightness)
{
  if (brightness == std::numeric_limits<ColorComponent>::max())
    return c;
  return colorFromRGBW(applyBrightness(colorR(c), brightness), applyBrightness(colorG(c), brightness), applyBrightness(colorB(c), brightness), applyBrightness(colorW(c), brightness));
}

} // namespace lw
