#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/Compat.h"
#include "colors/Color.h"
#include "colors/palette/ModeEnums.h"
#include "colors/palette/PaletteDomain.h"

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

struct PaletteSampleOptions
{
  lw::colors::ColorComponent brightnessScale{std::numeric_limits<lw::colors::ColorComponent>::max()};
  lw::Color outOfRangeColor{};
  WrapMode wrapMode{WrapMode::Clamp};
  BlendMode blendMode{BlendMode::Linear};
  TieBreakPolicy tieBreakPolicy{TieBreakPolicy::Stable};
  uint8_t quantizedLevels{8};
  palette_logical_domain_count_t scaledSampleCount{0};
};

struct PaletteStop
{
  using ColorType = lw::Color;

  static constexpr PaletteStop fromRgb8(palette_stop_index_t index, uint32_t rgb)
  {
    return fromRgb8(index, static_cast<uint8_t>((rgb >> 16U) & 0xFFU), static_cast<uint8_t>((rgb >> 8U) & 0xFFU), static_cast<uint8_t>(rgb & 0xFFU));
  }

  static constexpr PaletteStop fromRgb8(palette_stop_index_t index, uint8_t r, uint8_t g, uint8_t b) { return PaletteStop{index, lw::colorFromRGB(r, g, b)}; }

  palette_stop_index_t index{0};
  lw::Color color{};
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

class IPalette
{
protected:
  explicit constexpr IPalette(uint32_t typeCode) : _typeCode(typeCode) {}

  uint32_t _typeCode;

public:
  using ColorType = lw::Color;
  using SettingsEntry = std::pair<const char*, const char*>;
  using SettingsDescriptor = PaletteSettingDescriptor;
  using SettingsView = span<const SettingsEntry>;
  using SettingsDescriptorView = span<const SettingsDescriptor>;
  using StopsView = span<const PaletteStop>;

  virtual ~IPalette() = default;

  constexpr uint32_t typeCode() const { return _typeCode; }

  virtual StopsView stops() const = 0;
  virtual void syncTo(IPalette* that) { (void)that; }
  virtual void updateSettings(SettingsView settings) { (void)settings; }
  virtual void update(uint32_t step = 0) = 0;
};

namespace detail
{
  template <typename TDerived> TDerived* syncTarget(IPalette* that)
  {
    if (that == nullptr || that->typeCode() != TDerived::TypeCode)
    {
      return nullptr;
    }

    return static_cast<TDerived*>(that);
  }
} // namespace detail

class Palette : public IPalette
{
public:
  using StopsView = typename IPalette::StopsView;
  using StorageType = std::vector<PaletteStop>;

  static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::Palette;
  inline static constexpr std::array<PaletteSettingDescriptor, 0> AllowedSettings{};

  Palette() : IPalette(TypeCode) {}

  explicit Palette(StorageType stops) : IPalette(TypeCode), _stops(std::move(stops))
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  explicit Palette(StopsView stops) : IPalette(TypeCode), _stops(stops.begin(), stops.end())
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  template <size_t N> explicit Palette(const std::array<PaletteStop, N>& stops) : IPalette(TypeCode), _stops(stops.begin(), stops.end())
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  Palette(std::initializer_list<PaletteStop> stops) : IPalette(TypeCode), _stops(stops)
  {
    if (!detail::isValidPaletteStops(_stops))
    {
      _stops.clear();
    }
  }

  static std::unique_ptr<IPalette> parseDynamic(span<const char> stops)
  {
    StorageType parsedStops;
    if (!tryParseStops(stops, parsedStops))
    {
      return nullptr;
    }

    return std::make_unique<Palette>(std::move(parsedStops));
  }

  static std::unique_ptr<IPalette> parseDynamic(span<char> stops) { return parseDynamic(span<const char>(stops.data(), stops.size())); }

  static std::unique_ptr<IPalette> parseDynamic(const char* stops) { return parseDynamic(cStringSpan(stops)); }

  static Palette parse(span<const char> stops)
  {
    StorageType parsedStops;
    if (!tryParseStops(stops, parsedStops))
    {
      return Palette();
    }

    return Palette(std::move(parsedStops));
  }

  static Palette parse(span<char> stops) { return parse(span<const char>(stops.data(), stops.size())); }

  static Palette parse(const char* stops) { return parse(cStringSpan(stops)); }

  StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

  void update(uint32_t = 0) override {}

  void syncTo(IPalette* that) override
  {
    Palette* target = detail::syncTarget<Palette>(that);
    if (target == nullptr)
    {
      return;
    }

    target->_stops = _stops;
  }

  StorageType& storage() { return _stops; }

  const StorageType& storage() const { return _stops; }

private:
  static bool tryParseStops(span<const char> text, StorageType& parsedStops)
  {
    span<const char> remaining = trimLeadingWhitespace(text);
    if (remaining.empty())
    {
      return false;
    }

    size_t ignoredConsumed = 0;
    size_t ignoredIndex = 0;
    lw::Color ignoredColor{};
    const bool hasExplicitIndexes = tryParseStop(remaining, ignoredConsumed, ignoredIndex, ignoredColor);

    while (!remaining.empty())
    {
      size_t index = 0;
      lw::Color color{};
      size_t consumed = 0;

      if (hasExplicitIndexes)
      {
        if (!tryParseStop(remaining, consumed, index, color))
        {
          return false;
        }

        parsedStops.push_back(PaletteStop{index, color});
      }
      else
      {
        if (!tryParseColor(remaining, consumed, color))
        {
          return false;
        }

        parsedStops.push_back(PaletteStop{0u, color});
      }

      remaining = trimLeadingWhitespace(remaining.subspan(consumed));
      if (remaining.empty())
      {
        break;
      }

      if (remaining[0] != '|')
      {
        return false;
      }

      remaining = trimLeadingWhitespace(remaining.subspan(1));
      if (remaining.empty())
      {
        return false;
      }
    }

    if (!hasExplicitIndexes)
    {
      assignDistributedStopIndexes(parsedStops);
    }
    else
    {
      normalizeParsedStopIndexes(parsedStops);
    }

    return detail::isValidPaletteStops(parsedStops);
  }

  static void normalizeParsedStopIndexes(StorageType& parsedStops)
  {
    if (parsedStops.empty())
    {
      return;
    }

    using lw::colors::palettes::detail::PaletteDomainMaxIndex;

    if (parsedStops.size() == 1u)
    {
      parsedStops[0].index = 0u;
      parsedStops.push_back(PaletteStop{PaletteDomainMaxIndex, parsedStops[0].color});
      return;
    }

    const palette_stop_index_t minIndex = parsedStops.front().index;
    const palette_stop_index_t maxIndex = parsedStops.back().index;

    if (minIndex == 0u && maxIndex == PaletteDomainMaxIndex)
    {
      return;
    }

    if (maxIndex <= minIndex)
    {
      return;
    }

    const uint64_t sourceSpan = static_cast<uint64_t>(maxIndex - minIndex);
    for (auto& stop : parsedStops)
    {
      const uint64_t offset = static_cast<uint64_t>(stop.index - minIndex);
      stop.index = static_cast<palette_stop_index_t>((offset * PaletteDomainMaxIndex) / sourceSpan);
    }
  }

  static void assignDistributedStopIndexes(StorageType& parsedStops)
  {
    if (parsedStops.empty())
    {
      return;
    }

    if (parsedStops.size() == 1u)
    {
      using lw::colors::palettes::detail::PaletteDomainMaxIndex;
      parsedStops[0].index = 0u;
      parsedStops.push_back(PaletteStop{PaletteDomainMaxIndex, parsedStops[0].color});
      return;
    }

    using lw::colors::palettes::detail::PaletteDomainMaxIndex;
    const size_t stopSpan = parsedStops.size() - 1u;
    for (size_t i = 0; i < parsedStops.size(); ++i)
    {
      parsedStops[i].index = static_cast<palette_stop_index_t>((i * PaletteDomainMaxIndex) / stopSpan);
    }
  }

  static bool tryParseStop(span<const char> text, size_t& consumed, palette_stop_index_t& index, lw::Color& color)
  {
    const span<const char> trimmed = trimLeadingWhitespace(text);
    const size_t leadingWhitespace = text.size() - trimmed.size();

    size_t indexConsumed = 0;
    if (!tryParseIndex(trimmed, indexConsumed, index))
    {
      return false;
    }

    span<const char> remaining = trimLeadingWhitespace(trimmed.subspan(indexConsumed));
    if (remaining.empty() || remaining[0] != ',')
    {
      return false;
    }

    size_t colorConsumed = 0;
    if (!tryParseColor(remaining.subspan(1), colorConsumed, color))
    {
      return false;
    }

    consumed = leadingWhitespace + indexConsumed + (remaining.size() - remaining.subspan(1).size()) + colorConsumed;
    return true;
  }

  static bool tryParseIndex(span<const char> text, size_t& consumed, size_t& index)
  {
    const span<const char> trimmed = trimLeadingWhitespace(text);
    const size_t leadingWhitespace = text.size() - trimmed.size();
    if (trimmed.empty() || !std::isdigit(static_cast<unsigned char>(trimmed[0])))
    {
      return false;
    }

    size_t parsed = 0;
    size_t cursor = 0;
    while (cursor < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[cursor])))
    {
      parsed = (parsed * 10u) + static_cast<size_t>(trimmed[cursor] - '0');
      ++cursor;
    }

    index = parsed;
    consumed = leadingWhitespace + cursor;
    return true;
  }

  static bool tryParseColor(span<const char> text, size_t& consumed, lw::Color& color)
  {
    const span<const char> trimmed = trimLeadingWhitespace(text);
    const size_t leadingWhitespace = text.size() - trimmed.size();

    size_t tokenLength = 0;
    while (tokenLength < trimmed.size() && !std::isspace(static_cast<unsigned char>(trimmed[tokenLength])) && trimmed[tokenLength] != '|')
    {
      ++tokenLength;
    }

    if (tokenLength == 0)
    {
      return false;
    }

    if (!lw::tryParseColor(trimmed.first(tokenLength), color))
    {
      return false;
    }

    consumed = leadingWhitespace + tokenLength;
    return true;
  }

  static span<const char> trimLeadingWhitespace(span<const char> text)
  {
    size_t offset = 0;
    while (offset < text.size() && std::isspace(static_cast<unsigned char>(text[offset])))
    {
      ++offset;
    }

    return text.subspan(offset);
  }

  static span<const char> cStringSpan(const char* text)
  {
    if (text == nullptr)
    {
      return {};
    }

    return span<const char>(text, std::strlen(text));
  }

  StorageType _stops{};
};

} // namespace lw::colors::palettes
