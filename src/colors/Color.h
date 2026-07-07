#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <cctype>
#include <limits>
#include <string_view>
#include <type_traits>

#include "core/Compat.h"

namespace lw::colors
{
static constexpr size_t ColorComponentBitDepth = static_cast<size_t>(LW_COLOR_COMPONENT_SIZE);

static_assert(ColorComponentBitDepth == 8 || ColorComponentBitDepth == 16, "LW_COLOR_COMPONENT_SIZE must be 8 or 16.");

using ColorComponent = std::conditional_t<(ColorComponentBitDepth == 16), uint16_t, uint8_t>;

class Color
{
public:
  using ComponentType = ColorComponent;

  static constexpr ColorComponent MaxComponent = std::numeric_limits<ColorComponent>::max();

  constexpr Color() = default;

  template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) <= 4) && (std::is_convertible_v<Args, ColorComponent> && ...)>> constexpr Color(Args... args) : Channels{}
  {
    constexpr size_t ArgCount = sizeof...(Args);
    const std::array<ColorComponent, ArgCount> values{static_cast<ColorComponent>(args)...};

    for (size_t idx = 0; idx < ArgCount; ++idx)
    {
      Channels[idx] = values[idx];
    }
  }

  constexpr ColorComponent operator[](char channel) const { return Channels[channelIndex(channel)]; }

  constexpr ColorComponent& operator[](char channel) { return Channels[channelIndex(channel)]; }

  constexpr ColorComponent channelAtIndex(size_t index) const { return Channels[index]; }

  constexpr ColorComponent& channelAtIndex(size_t index) { return Channels[index]; }

  constexpr bool operator==(const Color& other) const { return Channels == other.Channels; }

  constexpr bool operator!=(const Color& other) const { return !(*this == other); }

  constexpr bool operator<(const Color& other) const { return compareCanonical(other) < 0; }

  constexpr bool operator<=(const Color& other) const { return compareCanonical(other) <= 0; }

  constexpr bool operator>(const Color& other) const { return compareCanonical(other) > 0; }

  constexpr bool operator>=(const Color& other) const { return compareCanonical(other) >= 0; }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint8_t>>> constexpr Color& operator=(uint32_t packed)
  {
    (*this)['R'] = static_cast<ColorComponent>((packed >> (2u * 8u)) & 0xFFu);
    (*this)['G'] = static_cast<ColorComponent>((packed >> (1u * 8u)) & 0xFFu);
    (*this)['B'] = static_cast<ColorComponent>((packed >> (0u * 8u)) & 0xFFu);
    (*this)['W'] = static_cast<ColorComponent>((packed >> (3u * 8u)) & 0xFFu);
    return *this;
  }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr Color& operator=(uint64_t packed)
  {
    (*this)['R'] = static_cast<ColorComponent>((packed >> (2u * 16u)) & 0xFFFFull);
    (*this)['G'] = static_cast<ColorComponent>((packed >> (1u * 16u)) & 0xFFFFull);
    (*this)['B'] = static_cast<ColorComponent>((packed >> (0u * 16u)) & 0xFFFFull);
    (*this)['W'] = static_cast<ColorComponent>((packed >> (3u * 16u)) & 0xFFFFull);
    return *this;
  }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr Color& operator=(int64_t packed) { return operator=(static_cast<uint64_t>(packed)); }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint8_t>>> constexpr operator uint32_t() const
  {
    const uint32_t r = static_cast<uint32_t>((*this)['R']);
    const uint32_t g = static_cast<uint32_t>((*this)['G']);
    const uint32_t b = static_cast<uint32_t>((*this)['B']);
    const uint32_t w = static_cast<uint32_t>((*this)['W']);

    return (w << (3u * 8u)) | (r << (2u * 8u)) | (g << (1u * 8u)) | (b << (0u * 8u));
  }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint8_t>>> constexpr operator int32_t() const { return static_cast<int32_t>(static_cast<uint32_t>(*this)); }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr operator uint64_t() const
  {
    const uint64_t r = static_cast<uint64_t>((*this)['R']);
    const uint64_t g = static_cast<uint64_t>((*this)['G']);
    const uint64_t b = static_cast<uint64_t>((*this)['B']);
    const uint64_t w = static_cast<uint64_t>((*this)['W']);

    return (w << (3u * 16u)) | (r << (2u * 16u)) | (g << (1u * 16u)) | (b << (0u * 16u));
  }

  template <typename T = ColorComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr operator int64_t() const { return static_cast<int64_t>(static_cast<uint64_t>(*this)); }

  static Color parse(span<const char> text)
  {
    Color color{};
    if (!tryParse(text, color))
    {
      return {};
    }

    return color;
  }

  static Color parse(const char* text) { return parse(cStringSpan(text)); }

  static bool tryParse(span<const char> text, Color& color)
  {
    const span<const char> trimmed = trimWhitespace(text);
    if (trimmed.empty())
    {
      return false;
    }

    Color parsed{};
    size_t consumed = 0;
    if (!tryParseToken(trimmed, consumed, parsed))
    {
      return false;
    }

    if (!trimWhitespace(trimmed.subspan(consumed)).empty())
    {
      return false;
    }

    color = parsed;
    return true;
  }

  static bool tryParse(const char* text, Color& color) { return tryParse(cStringSpan(text), color); }

  static constexpr size_t serializedLength() { return 4 * sizeof(ColorComponent) * 2u; }

  bool serialize(span<char> sink) const
  {
    static constexpr char HexDigits[] = "0123456789ABCDEF";

    if (sink.data() == nullptr)
    {
      return false;
    }

    const size_t requiredLength = serializedLength();
    if (sink.size() <= requiredLength)
    {
      return false;
    }

    constexpr const char* channelOrder = "RGBW";

    size_t outputIndex = 0;

    for (size_t channelIndex = 0; channelIndex < 4; ++channelIndex)
    {
      const ColorComponent component = (*this)[channelOrder[channelIndex]];
      for (size_t nibble = 0; nibble < (sizeof(ColorComponent) * 2u); ++nibble)
      {
        const size_t shift = ((sizeof(ColorComponent) * 2u) - nibble - 1u) * 4u;
        sink[outputIndex++] = HexDigits[(static_cast<size_t>(component) >> shift) & 0x0Fu];
      }
    }

    sink[outputIndex] = '\0';

    return true;
  }

  bool serialize(char* sink, size_t length) const { return serialize(span<char>(sink, length)); }

private:
  static constexpr size_t channelIndex(char channel)
  {
    switch (channel)
    {
      case 'R':
      case 'r':
        return 0;

      case 'G':
      case 'g':
        return 1;

      case 'B':
      case 'b':
        return 2;

      case 'W':
      case 'w':
        return 3;

      default:
        return 0;
    }
  }

  static bool tryParseToken(span<const char> text, size_t& consumed, Color& color)
  {
    size_t prefixLength = 0;

    if (!text.empty() && text[0] == '#')
    {
      prefixLength = 1u;
    }
    else if (text.size() >= 2u && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
      prefixLength = 2u;
    }

    size_t scan = prefixLength;

    constexpr size_t DigitsPerComponent = sizeof(ColorComponent) * 2u;
    constexpr size_t ExpectedDigitCount = 4 * DigitsPerComponent;

    size_t digitCount = 0;
    while (scan < text.size() && hexNibble(text[scan]) >= 0)
    {
      ++digitCount;
      ++scan;
    }

    const span<const char> token = text.first(scan);

    if (digitCount != ExpectedDigitCount)
    {
      return false;
    }

    if (!tryParseHexColorToken(token, color))
    {
      return false;
    }

    consumed = token.size();
    return true;
  }

  static bool tryParseHexColorToken(span<const char> token, Color& color)
  {
    if (token.empty())
    {
      return false;
    }

    size_t cursor = 0;
    if (token[cursor] == '#')
    {
      ++cursor;
    }
    else if (token.size() >= 2u && token[0] == '0' && (token[1] == 'x' || token[1] == 'X'))
    {
      cursor = 2u;
    }

    constexpr size_t ParsedDigitsPerComponent = sizeof(ColorComponent) * 2u;
    constexpr size_t ExpectedDigitCount = 4 * ParsedDigitsPerComponent;

    if ((token.size() - cursor) != ExpectedDigitCount)
    {
      return false;
    }

    Color parsed{};
    for (size_t logicalChannel = 0; logicalChannel < 4; ++logicalChannel)
    {
      constexpr char ChannelTags[4] = {'R', 'G', 'B', 'W'};
      const char channelTag = ChannelTags[logicalChannel];

      ColorComponent value = 0;
      for (size_t digit = 0; digit < ParsedDigitsPerComponent; ++digit)
      {
        const int nibble = hexNibble(token[cursor]);
        if (nibble < 0)
        {
          return false;
        }

        value = static_cast<ColorComponent>((value << 4) | static_cast<ColorComponent>(nibble));
        ++cursor;
      }

      parsed[channelTag] = value;
    }

    color = parsed;
    return true;
  }

  static span<const char> trimWhitespace(span<const char> text)
  {
    size_t start = 0;
    size_t end = text.size();

    while (start < end && std::isspace(static_cast<unsigned char>(text[start])))
    {
      ++start;
    }

    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1u])))
    {
      --end;
    }

    return text.subspan(start, end - start);
  }

  static span<const char> cStringSpan(const char* text)
  {
    if (text == nullptr)
    {
      return {};
    }

    const size_t length = std::string_view(text).size();
    return span<const char>(text, length);
  }

  static int hexNibble(char value)
  {
    if (value >= '0' && value <= '9')
    {
      return value - '0';
    }

    if (value >= 'a' && value <= 'f')
    {
      return 10 + (value - 'a');
    }

    if (value >= 'A' && value <= 'F')
    {
      return 10 + (value - 'A');
    }

    return -1;
  }

  constexpr int compareCanonical(const Color& other) const
  {
    constexpr char CanonicalChannels[4] = {'R', 'G', 'B', 'W'};

    for (char channel : CanonicalChannels)
    {
      const auto left = (*this)[channel];
      const auto right = other[channel];
      if (left < right)
      {
        return -1;
      }

      if (left > right)
      {
        return 1;
      }
    }

    return 0;
  }

  std::array<ColorComponent, 4> Channels{}; // zero-initialized
};

// Type traits for generic code that still needs to be parameterized on a color-like type
template <typename T, typename = void> struct ColorTypeImpl : std::false_type
{
};

template <typename T> struct ColorTypeImpl<T, std::void_t<typename T::ComponentType>> : std::true_type
{
};

template <typename T> static constexpr bool ColorType = ColorTypeImpl<T>::value;

template <typename T, typename TComponent> static constexpr bool ColorComponentTypeIs = ColorType<T> && std::is_same_v<typename T::ComponentType, TComponent>;

template <typename T, size_t BitDepth> static constexpr bool ColorHasBitDepth = ColorType<T> && ((sizeof(typename T::ComponentType) * 8) == BitDepth);

template <typename T, size_t BitDepth> using RequireColorBitDepth = std::enable_if_t<ColorHasBitDepth<T, BitDepth>, int>;

} // namespace lw::colors

namespace lw
{

inline constexpr size_t ColorComponentBitDepth = colors::ColorComponentBitDepth;

using Color = colors::Color;

template <typename T, typename Enable = void> using ColorTypeImpl = colors::ColorTypeImpl<T, Enable>;

template <typename T> inline constexpr bool ColorType = colors::ColorType<T>;

template <typename T, typename TComponent> inline constexpr bool ColorComponentTypeIs = colors::ColorComponentTypeIs<T, TComponent>;

template <typename T, size_t BitDepth> inline constexpr bool ColorHasBitDepth = colors::ColorHasBitDepth<T, BitDepth>;

template <typename T, size_t BitDepth> using RequireColorBitDepth = colors::RequireColorBitDepth<T, BitDepth>;

} // namespace lw
