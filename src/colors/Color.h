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
static constexpr size_t ColorMinimumComponentSizeBits = static_cast<size_t>(LW_COLOR_MINIMUM_COMPONENT_SIZE);

static_assert(ColorMinimumComponentSizeBits == 8 || ColorMinimumComponentSizeBits == 16, "LW_COLOR_MINIMUM_COMPONENT_SIZE must be 8 or 16.");

template <typename TComponent = uint8_t> class RgbwColor
{
public:
  using ComponentType = TComponent;

  static constexpr size_t ChannelCount = 4;
  static constexpr TComponent MaxComponent = std::numeric_limits<TComponent>::max();

  constexpr RgbwColor() = default;

  template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) <= 4) && (std::is_convertible_v<Args, TComponent> && ...)>> constexpr RgbwColor(Args... args) : Channels{}
  {
    constexpr size_t ArgCount = sizeof...(Args);
    const std::array<TComponent, ArgCount> values{static_cast<TComponent>(args)...};

    for (size_t idx = 0; idx < ArgCount; ++idx)
    {
      Channels[idx] = values[idx];
    }
  }

  constexpr TComponent operator[](char channel) const { return Channels[channelIndex(channel)]; }

  TComponent& operator[](char channel) { return Channels[channelIndex(channel)]; }

  constexpr TComponent channelAtIndex(size_t index) const { return Channels[index]; }

  TComponent& channelAtIndex(size_t index) { return Channels[index]; }

  constexpr bool operator==(const RgbwColor& other) const { return Channels == other.Channels; }

  constexpr bool operator!=(const RgbwColor& other) const { return !(*this == other); }

  constexpr bool operator<(const RgbwColor& other) const { return compareCanonical(other) < 0; }

  constexpr bool operator<=(const RgbwColor& other) const { return compareCanonical(other) <= 0; }

  constexpr bool operator>(const RgbwColor& other) const { return compareCanonical(other) > 0; }

  constexpr bool operator>=(const RgbwColor& other) const { return compareCanonical(other) >= 0; }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint8_t>>> constexpr RgbwColor& operator=(uint32_t packed)
  {
    (*this)['R'] = static_cast<TComponent>((packed >> (2u * 8u)) & 0xFFu);
    (*this)['G'] = static_cast<TComponent>((packed >> (1u * 8u)) & 0xFFu);
    (*this)['B'] = static_cast<TComponent>((packed >> (0u * 8u)) & 0xFFu);
    (*this)['W'] = static_cast<TComponent>((packed >> (3u * 8u)) & 0xFFu);
    return *this;
  }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr RgbwColor& operator=(uint64_t packed)
  {
    (*this)['R'] = static_cast<TComponent>((packed >> (2u * 16u)) & 0xFFFFull);
    (*this)['G'] = static_cast<TComponent>((packed >> (1u * 16u)) & 0xFFFFull);
    (*this)['B'] = static_cast<TComponent>((packed >> (0u * 16u)) & 0xFFFFull);
    (*this)['W'] = static_cast<TComponent>((packed >> (3u * 16u)) & 0xFFFFull);
    return *this;
  }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr RgbwColor& operator=(int64_t packed) { return operator=(static_cast<uint64_t>(packed)); }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint8_t>>> constexpr operator uint32_t() const
  {
    const uint32_t r = static_cast<uint32_t>((*this)['R']);
    const uint32_t g = static_cast<uint32_t>((*this)['G']);
    const uint32_t b = static_cast<uint32_t>((*this)['B']);
    const uint32_t w = static_cast<uint32_t>((*this)['W']);

    return (w << (3u * 8u)) | (r << (2u * 8u)) | (g << (1u * 8u)) | (b << (0u * 8u));
  }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint8_t>>> constexpr operator int32_t() const { return static_cast<int32_t>(static_cast<uint32_t>(*this)); }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr operator uint64_t() const
  {
    const uint64_t r = static_cast<uint64_t>((*this)['R']);
    const uint64_t g = static_cast<uint64_t>((*this)['G']);
    const uint64_t b = static_cast<uint64_t>((*this)['B']);
    const uint64_t w = static_cast<uint64_t>((*this)['W']);

    return (w << (3u * 16u)) | (r << (2u * 16u)) | (g << (1u * 16u)) | (b << (0u * 16u));
  }

  template <typename T = TComponent, typename = std::enable_if_t<std::is_same_v<T, uint16_t>>> constexpr operator int64_t() const { return static_cast<int64_t>(static_cast<uint64_t>(*this)); }

  static RgbwColor parse(span<const char> text)
  {
    RgbwColor color{};
    if (!tryParse(text, color))
    {
      return {};
    }

    return color;
  }

  static RgbwColor parse(const char* text) { return parse(cStringSpan(text)); }

  static bool tryParse(span<const char> text, RgbwColor& color)
  {
    const span<const char> trimmed = trimWhitespace(text);
    if (trimmed.empty())
    {
      return false;
    }

    RgbwColor parsed{};
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

  static bool tryParse(const char* text, RgbwColor& color) { return tryParse(cStringSpan(text), color); }

  static constexpr size_t serializedLength() { return 4 * sizeof(TComponent) * 2u; }

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
      const TComponent component = (*this)[channelOrder[channelIndex]];
      for (size_t nibble = 0; nibble < (sizeof(TComponent) * 2u); ++nibble)
      {
        const size_t shift = ((sizeof(TComponent) * 2u) - nibble - 1u) * 4u;
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

  static bool tryParseToken(span<const char> text, size_t& consumed, RgbwColor& color)
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

    constexpr size_t DigitsPerComponent = sizeof(TComponent) * 2u;
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
      switch (digitCount)
      {
        case 8u:
          return tryParseAndConvertColor<RgbwColor<uint8_t>>(token, consumed, color);

        case 16u:
          return tryParseAndConvertColor<RgbwColor<uint16_t>>(token, consumed, color);

        default:
          return false;
      }
    }

    if (!tryParseHexColorToken(token, color))
    {
      return false;
    }

    consumed = token.size();
    return true;
  }

  template <typename TParsedColor> static bool tryParseHexColorToken(span<const char> token, TParsedColor& color)
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

    constexpr size_t ParsedDigitsPerComponent = sizeof(typename TParsedColor::ComponentType) * 2u;
    constexpr size_t ExpectedDigitCount = 4 * ParsedDigitsPerComponent;

    if ((token.size() - cursor) != ExpectedDigitCount)
    {
      return false;
    }

    TParsedColor parsed{};
    for (size_t logicalChannel = 0; logicalChannel < 4; ++logicalChannel)
    {
      constexpr char ChannelTags[4] = {'R', 'G', 'B', 'W'};
      const char channelTag = ChannelTags[logicalChannel];

      typename TParsedColor::ComponentType value = 0;
      for (size_t digit = 0; digit < ParsedDigitsPerComponent; ++digit)
      {
        const int nibble = hexNibble(token[cursor]);
        if (nibble < 0)
        {
          return false;
        }

        value = static_cast<typename TParsedColor::ComponentType>((value << 4) | static_cast<typename TParsedColor::ComponentType>(nibble));
        ++cursor;
      }

      parsed[channelTag] = value;
    }

    color = parsed;
    return true;
  }

  template <typename TSourceColor> static bool tryParseAndConvertColor(span<const char> token, size_t& consumed, RgbwColor& color)
  {
    TSourceColor parsed{};
    if (!tryParseHexColorToken(token, parsed))
    {
      return false;
    }

    color = convertParsedColor(parsed);
    consumed = token.size();
    return true;
  }

  template <typename TSourceColor> static RgbwColor convertParsedColor(const TSourceColor& source)
  {
    using SourceComponent = typename TSourceColor::ComponentType;

    RgbwColor result{};
    constexpr char AllChannels[4] = {'R', 'G', 'B', 'W'};

    for (size_t i = 0; i < 4; ++i)
    {
      const char channel = AllChannels[i];
      const SourceComponent value = source[channel];

      if constexpr (sizeof(SourceComponent) == sizeof(TComponent))
      {
        result[channel] = static_cast<TComponent>(value);
      }
      else if constexpr (sizeof(SourceComponent) < sizeof(TComponent))
      {
        const TComponent widened = static_cast<TComponent>((static_cast<TComponent>(value) << 8) | value);
        result[channel] = widened;
      }
      else
      {
        result[channel] = static_cast<TComponent>(value >> 8);
      }
    }

    return result;
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

  constexpr int compareCanonical(const RgbwColor& other) const
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

  std::array<TComponent, 4> Channels{}; // zero-initialized
};

using Rgbw8Color = RgbwColor<uint8_t>;
using Rgbw16Color = RgbwColor<uint16_t>;

using DefaultColorType = std::conditional_t<(ColorMinimumComponentSizeBits == 16), Rgbw16Color, Rgbw8Color>;

using Color = DefaultColorType;

template <typename TColor, typename = void> struct ColorTypeImpl : std::false_type
{
};

template <typename TColor> struct ColorTypeImpl<TColor, std::void_t<typename TColor::ComponentType>> : std::true_type
{
};

template <typename TColor> static constexpr bool ColorType = ColorTypeImpl<TColor>::value;

template <typename TColor, typename TComponent> static constexpr bool ColorComponentTypeIs = ColorType<TColor> && std::is_same_v<typename TColor::ComponentType, TComponent>;

template <typename TColor, size_t BitDepth> static constexpr bool ColorComponentBitDepth = ColorType<TColor> && ((sizeof(typename TColor::ComponentType) * 8) == BitDepth);

template <typename TColor, size_t BitDepth> using RequireColorComponentBitDepth = std::enable_if_t<ColorComponentBitDepth<TColor, BitDepth>, int>;

} // namespace lw::colors

namespace lw
{

inline constexpr size_t ColorMinimumComponentSizeBits = colors::ColorMinimumComponentSizeBits;

template <typename TComponent = uint8_t> using RgbwColor = colors::RgbwColor<TComponent>;

using Rgbw8Color = colors::Rgbw8Color;
using Rgbw16Color = colors::Rgbw16Color;
using DefaultColorType = colors::DefaultColorType;
using Color = colors::Color;

template <typename TColor, typename Enable = void> using ColorTypeImpl = colors::ColorTypeImpl<TColor, Enable>;

template <typename TColor> inline constexpr bool ColorType = colors::ColorType<TColor>;

template <typename TColor, typename TComponent> inline constexpr bool ColorComponentTypeIs = colors::ColorComponentTypeIs<TColor, TComponent>;

template <typename TColor, size_t BitDepth> inline constexpr bool ColorComponentBitDepth = colors::ColorComponentBitDepth<TColor, BitDepth>;

template <typename TColor, size_t BitDepth> using RequireColorComponentBitDepth = colors::RequireColorComponentBitDepth<TColor, BitDepth>;

} // namespace lw
