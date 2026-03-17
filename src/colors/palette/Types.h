#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/Compat.h"
#include "colors/Color.h"
#include "colors/palette/ModeEnums.h"

namespace lw::colors::palettes
{
namespace detail
{
  constexpr uint32_t makePaletteTypeCode(char a, char b, char c, char d)
  {
    return (static_cast<uint32_t>(static_cast<uint8_t>(a)) << 24U) | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 16U) | (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 8U) |
           static_cast<uint32_t>(static_cast<uint8_t>(d));
  }

  struct PaletteTypeCodes
  {
    static constexpr uint32_t Palette = makePaletteTypeCode('P', 'A', 'L', 'T');
    static constexpr uint32_t StaticPalette = makePaletteTypeCode('S', 'P', 'A', 'L');
    static constexpr uint32_t RainbowPaletteGenerator = makePaletteTypeCode('R', 'B', 'O', 'W');
    static constexpr uint32_t TemporalRainbowPaletteGenerator = makePaletteTypeCode('T', 'R', 'B', 'W');
    static constexpr uint32_t RandomSmoothPaletteGenerator = makePaletteTypeCode('R', 'N', 'S', 'M');
    static constexpr uint32_t RandomCyclePaletteGenerator = makePaletteTypeCode('R', 'N', 'C', 'L');
  };

  template <typename TStops> bool isValidPaletteStops(const TStops& stops)
  {
    if (stops.size() < 2u)
    {
      return false;
    }

    using lw::colors::palettes::detail::PaletteDomainMaxIndex;
    if (stops.front().index != 0u || stops.back().index != PaletteDomainMaxIndex)
    {
      return false;
    }

    for (size_t i = 0; i < stops.size(); ++i)
    {
      if (stops[i].index > PaletteDomainMaxIndex)
      {
        return false;
      }

      if (i > 0u && stops[i].index < stops[i - 1u].index)
      {
        return false;
      }
    }

    return true;
  }
} // namespace detail

using palette_logical_index_t = size_t;
using palette_logical_domain_count_t = size_t;
using palette_stop_index_t = size_t;
using palette_canonical_fixed_t = uint32_t;

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> struct PaletteSampleOptions
{
  typename TColor::ComponentType brightnessScale{std::numeric_limits<typename TColor::ComponentType>::max()};
  TColor outOfRangeColor{};
  WrapMode wrapMode{WrapMode::Clamp};
  BlendMode blendMode{BlendMode::Linear};
  TieBreakPolicy tieBreakPolicy{TieBreakPolicy::Stable};
  uint8_t quantizedLevels{8};
  palette_logical_domain_count_t scaledSampleCount{0};
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> struct PaletteStop
{
  using ColorType = TColor;

  static constexpr PaletteStop fromRgb8(palette_stop_index_t index, uint32_t rgb)
  {
    return fromRgb8(index, static_cast<uint8_t>((rgb >> 16U) & 0xFFU), static_cast<uint8_t>((rgb >> 8U) & 0xFFU), static_cast<uint8_t>(rgb & 0xFFU));
  }

  static constexpr PaletteStop fromRgb8(palette_stop_index_t index, uint8_t r, uint8_t g, uint8_t b) { return PaletteStop{index, TColor{r, g, b}}; }

  palette_stop_index_t index{0};
  TColor color{};
};

enum class PaletteSettingValueType : uint8_t
{
  UnsignedSize,
  UnsignedColorComponent,
  UInt32,
  UInt8,
};

struct PaletteSettingDescriptor
{
  const char* key;
  PaletteSettingValueType valueType;
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> class IPalette
{
protected:
  explicit constexpr IPalette(uint32_t typeCode) : _typeCode(typeCode) {}

  uint32_t _typeCode;

public:
  using ColorType = TColor;
  using SettingsEntry = std::pair<const char*, const char*>;
  using SettingsDescriptor = PaletteSettingDescriptor;
  using SettingsView = span<const SettingsEntry>;
  using SettingsDescriptorView = span<const SettingsDescriptor>;
  using StopsView = span<const PaletteStop<TColor>>;

  virtual ~IPalette() = default;

  constexpr uint32_t typeCode() const { return _typeCode; }

  virtual StopsView stops() const = 0;
  virtual void syncTo(IPalette<TColor>* that) { (void)that; }
  virtual void updateSettings(SettingsView settings) { (void)settings; }
  virtual void update(uint32_t step = 0) = 0;
};

namespace detail
{
  template <typename TDerived, typename TColor> TDerived* syncTarget(IPalette<TColor>* that)
  {
    if (that == nullptr || that->typeCode() != TDerived::TypeCode)
    {
      return nullptr;
    }

    return static_cast<TDerived*>(that);
  }
} // namespace detail

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> class Palette : public IPalette<TColor>
{
public:
  using StopsView = typename IPalette<TColor>::StopsView;
  using StorageType = std::vector<PaletteStop<TColor>>;

  static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::Palette;
  inline static constexpr std::array<PaletteSettingDescriptor, 0> AllowedSettings{};

  Palette() : IPalette<TColor>(TypeCode) {}

  explicit Palette(StorageType stops) : IPalette<TColor>(TypeCode), _stops(std::move(stops))
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  explicit Palette(StopsView stops) : IPalette<TColor>(TypeCode), _stops(stops.begin(), stops.end())
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  template <size_t N> explicit Palette(const std::array<PaletteStop<TColor>, N>& stops) : IPalette<TColor>(TypeCode), _stops(stops.begin(), stops.end())
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  Palette(std::initializer_list<PaletteStop<TColor>> stops) : IPalette<TColor>(TypeCode), _stops(stops)
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }
  static std::unique_ptr<IPalette<TColor>> parseDynamic(const char* stops)
  {
    StorageType parsedStops;
    if (!tryParseStops(stops, parsedStops))
    {
      return nullptr;
    }

    return std::make_unique<Palette<TColor>>(std::move(parsedStops));
  }

  static Palette parse(const char* stops)
  {
    StorageType parsedStops;
    if (!tryParseStops(stops, parsedStops))
    {
      return Palette();
    }

    return Palette(std::move(parsedStops));
  }

  StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

  void update(uint32_t = 0) override {}

  void syncTo(IPalette<TColor>* that) override
  {
    Palette<TColor>* target = detail::syncTarget<Palette<TColor>, TColor>(that);
    if (target == nullptr)
    {
      return;
    }

    target->_stops = _stops;
  }

  StorageType& storage() { return _stops; }

  const StorageType& storage() const { return _stops; }

private:
  static bool tryParseStops(const char* text, StorageType& parsedStops)
  {
    if (text == nullptr)
    {
      return false;
    }

    const char* cursor = skipWhitespace(text);
    if (*cursor == '\0')
    {
      return false;
    }

    const char* formatCursor = cursor;
    size_t ignoredIndex = 0;
    TColor ignoredColor{};
    const bool hasExplicitIndexes = tryParseStop(formatCursor, ignoredIndex, ignoredColor);

    while (*cursor != '\0')
    {
      size_t index = 0;
      TColor color{};

      if (hasExplicitIndexes)
      {
        if (!tryParseStop(cursor, index, color))
        {
          return false;
        }

        parsedStops.push_back(PaletteStop<TColor>{index, color});
      }
      else
      {
        if (!tryParseColor(cursor, color))
        {
          return false;
        }

        parsedStops.push_back(PaletteStop<TColor>{0u, color});
      }

      cursor = skipWhitespace(cursor);
      if (*cursor == '\0')
      {
        break;
      }

      if (*cursor != '|')
      {
        return false;
      }

      cursor = skipWhitespace(cursor + 1);
      if (*cursor == '\0')
      {
        return false;
      }
    }

    if (!hasExplicitIndexes)
    {
      assignDistributedStopIndexes(parsedStops);
    }

    return detail::isValidPaletteStops(parsedStops);
  }

  static void assignDistributedStopIndexes(StorageType& parsedStops)
  {
    if (parsedStops.empty())
    {
      return;
    }

    if (parsedStops.size() == 1u)
    {
      parsedStops[0].index = 0u;
      return;
    }

    using lw::colors::palettes::detail::PaletteDomainMaxIndex;
    const size_t stopSpan = parsedStops.size() - 1u;
    for (size_t i = 0; i < parsedStops.size(); ++i)
    {
      parsedStops[i].index = static_cast<palette_stop_index_t>((i * PaletteDomainMaxIndex) / stopSpan);
    }
  }

  static bool tryParseStop(const char*& cursor, palette_stop_index_t& index, TColor& color)
  {
    if (!tryParseIndex(cursor, index))
    {
      return false;
    }

    cursor = skipWhitespace(cursor);
    if (*cursor != ',')
    {
      return false;
    }

    cursor = skipWhitespace(cursor + 1);
    return tryParseColor(cursor, color);
  }

  static bool tryParseIndex(const char*& cursor, size_t& index)
  {
    cursor = skipWhitespace(cursor);
    if (!std::isdigit(static_cast<unsigned char>(*cursor)))
    {
      return false;
    }

    size_t parsed = 0;
    while (std::isdigit(static_cast<unsigned char>(*cursor)))
    {
      parsed = (parsed * 10u) + static_cast<size_t>(*cursor - '0');
      ++cursor;
    }

    index = parsed;
    return true;
  }

  static bool tryParseColor(const char*& cursor, TColor& color)
  {
    const char* tokenStart = cursor;
    const char* scan = cursor;

    if (*scan == '#')
    {
      ++scan;
    }

    if (scan[0] == '0' && (scan[1] == 'x' || scan[1] == 'X'))
    {
      scan += 2;
    }

    constexpr size_t DigitsPerComponent = sizeof(typename TColor::ComponentType) * 2u;
    constexpr size_t ExpectedDigitCount = static_cast<size_t>(TColor::ChannelCount) * DigitsPerComponent;

    size_t digitCount = 0;
    while (hexNibble(*scan) >= 0)
    {
      ++digitCount;
      ++scan;
    }

    if (digitCount != ExpectedDigitCount)
    {
      switch (digitCount)
      {
        case 6u:
          return tryParseAndUpscaleColor<Rgb8Color>(tokenStart, scan, cursor, color);

        case 8u:
          return tryParseAndUpscaleColor<Rgbw8Color>(tokenStart, scan, cursor, color);

        case 10u:
          return tryParseAndUpscaleColor<Rgbcw8Color>(tokenStart, scan, cursor, color);

        case 12u:
          return tryParseAndUpscaleColor<Rgb16Color>(tokenStart, scan, cursor, color);

        case 16u:
          return tryParseAndUpscaleColor<Rgbw16Color>(tokenStart, scan, cursor, color);

        case 20u:
          return tryParseAndUpscaleColor<Rgbcw16Color>(tokenStart, scan, cursor, color);

        default:
          return false;
      }
    }

    color = ColorHexCodec::parseHex<TColor>(tokenStart);
    cursor = scan;
    return true;
  }

  template <typename TSourceColor> static bool tryParseAndUpscaleColor(const char* tokenStart, const char* tokenEnd, const char*& cursor, TColor& color)
  {
    using SourceComponent = typename TSourceColor::ComponentType;
    using TargetComponent = typename TColor::ComponentType;

    constexpr bool ChannelsCompatible = TSourceColor::ChannelCount <= TColor::ChannelCount;
    constexpr bool ComponentCompatible = sizeof(SourceComponent) <= sizeof(TargetComponent);

    if constexpr (!ChannelsCompatible || !ComponentCompatible)
    {
      (void)tokenStart;
      (void)tokenEnd;
      (void)cursor;
      (void)color;
      return false;
    }
    else
    {
      const TSourceColor parsed = ColorHexCodec::parseHex<TSourceColor>(tokenStart);
      color = upscaleParsedColor(parsed);
      cursor = tokenEnd;
      return true;
    }
  }

  template <typename TSourceColor> static TColor upscaleParsedColor(const TSourceColor& source)
  {
    using SourceComponent = typename TSourceColor::ComponentType;
    using TargetComponent = typename TColor::ComponentType;

    TColor result{};
    for (auto channel : TSourceColor::channelIndexes())
    {
      const SourceComponent value = source[channel];

      if constexpr (sizeof(SourceComponent) == sizeof(TargetComponent))
      {
        result[channel] = static_cast<TargetComponent>(value);
      }
      else
      {
        const TargetComponent widened = static_cast<TargetComponent>((static_cast<TargetComponent>(value) << 8) | value);
        result[channel] = widened;
      }
    }

    return result;
  }

  static const char* skipWhitespace(const char* cursor)
  {
    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor)))
    {
      ++cursor;
    }

    return cursor;
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

  StorageType _stops{};
};

} // namespace lw::colors::palettes
