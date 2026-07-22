#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace lw
{

class GridMapping
{
public:
  enum class AxisOrder : uint8_t
  {
    RowsFirst = 0,    // scan rows left-to-right, top-to-bottom so the first row is pixels 0..(width-1), the second row is pixels width..(2*width-1), etc.
    ColumnsFirst = 1, // scan columns top-to-bottom, left-to-right so the first column is pixels 0..(height-1), the second column is pixels height..(2*height-1), etc.
  };

  enum class LinePattern : uint8_t
  {
    Progressive = 0, // scan lines in a progressive manner, restarting at the beginning of each line so every line goes in the same direction, e.g. left-to-right, left-to-right, left-to-right, etc.
    Serpentine = 1,  // scan lines in a serpentine manner, alternating direction on each line so line 0 would go from left-to-right, line 1 from right-to-left, line 2 from left-to-right, etc.
  };

  enum class QuarterTurn : uint8_t
  {
    None = 0,   // no rotation
    Deg90 = 1,  // 90 degrees clockwise
    Deg180 = 2, // 180 degrees
    Deg270 = 3, // 270 degrees clockwise
  };

  struct Components
  {
    AxisOrder axisOrder;
    LinePattern linePattern;
    QuarterTurn quarterTurn;
  };

  // Bit layout: [bit3: column-major][bit2: alternating][bit1..0: rotation]
  static constexpr uint8_t RotationMask = 0b0011;
  static constexpr uint8_t AlternatingBit = 0b0100;
  static constexpr uint8_t ColumnMajorBit = 0b1000;

  constexpr GridMapping() : _value(0) {}

  constexpr explicit GridMapping(uint8_t value) : _value(value) {}

  constexpr uint8_t raw() const { return _value; }

  constexpr uint8_t rotation() const { return static_cast<uint8_t>(_value & RotationMask); }

  constexpr QuarterTurn quarterTurn() const { return static_cast<QuarterTurn>(_value & RotationMask); }

  constexpr bool isAlternating() const { return (_value & AlternatingBit) != 0; }

  constexpr bool isColumnMajor() const { return (_value & ColumnMajorBit) != 0; }

  constexpr AxisOrder axisOrder() const { return isColumnMajor() ? AxisOrder::ColumnsFirst : AxisOrder::RowsFirst; }

  constexpr LinePattern linePattern() const { return isAlternating() ? LinePattern::Serpentine : LinePattern::Progressive; }

  constexpr Components unpack() const { return Components{axisOrder(), linePattern(), quarterTurn()}; }

  static constexpr GridMapping make(AxisOrder axisOrder, LinePattern linePattern, QuarterTurn quarterTurn)
  {
    const uint8_t family = static_cast<uint8_t>((axisOrder == AxisOrder::ColumnsFirst ? ColumnMajorBit : 0) | (linePattern == LinePattern::Serpentine ? AlternatingBit : 0));
    return GridMapping{static_cast<uint8_t>(family | (static_cast<uint8_t>(quarterTurn) & RotationMask))};
  }

  static const GridMapping RowsFirstProgressive;
  static const GridMapping RowsFirstProgressiveDeg90;
  static const GridMapping RowsFirstProgressiveDeg180;
  static const GridMapping RowsFirstProgressiveDeg270;

  static const GridMapping RowsFirstSerpentine;
  static const GridMapping RowsFirstSerpentineDeg90;
  static const GridMapping RowsFirstSerpentineDeg180;
  static const GridMapping RowsFirstSerpentineDeg270;

  static const GridMapping ColumnsFirstProgressive;
  static const GridMapping ColumnsFirstProgressiveDeg90;
  static const GridMapping ColumnsFirstProgressiveDeg180;
  static const GridMapping ColumnsFirstProgressiveDeg270;

  static const GridMapping ColumnsFirstSerpentine;
  static const GridMapping ColumnsFirstSerpentineDeg90;
  static const GridMapping ColumnsFirstSerpentineDeg180;
  static const GridMapping ColumnsFirstSerpentineDeg270;

  friend constexpr bool operator==(GridMapping a, GridMapping b) { return a._value == b._value; }

  friend constexpr bool operator!=(GridMapping a, GridMapping b) { return !(a == b); }

  constexpr explicit operator uint8_t() const { return _value; }

private:
  uint8_t _value;
};

inline constexpr GridMapping GridMapping::RowsFirstProgressive = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Progressive, QuarterTurn::None);
inline constexpr GridMapping GridMapping::RowsFirstProgressiveDeg90 = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Progressive, QuarterTurn::Deg90);
inline constexpr GridMapping GridMapping::RowsFirstProgressiveDeg180 = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Progressive, QuarterTurn::Deg180);
inline constexpr GridMapping GridMapping::RowsFirstProgressiveDeg270 = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Progressive, QuarterTurn::Deg270);

inline constexpr GridMapping GridMapping::RowsFirstSerpentine = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Serpentine, QuarterTurn::None);
inline constexpr GridMapping GridMapping::RowsFirstSerpentineDeg90 = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Serpentine, QuarterTurn::Deg90);
inline constexpr GridMapping GridMapping::RowsFirstSerpentineDeg180 = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Serpentine, QuarterTurn::Deg180);
inline constexpr GridMapping GridMapping::RowsFirstSerpentineDeg270 = GridMapping::make(AxisOrder::RowsFirst, LinePattern::Serpentine, QuarterTurn::Deg270);

inline constexpr GridMapping GridMapping::ColumnsFirstProgressive = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Progressive, QuarterTurn::None);
inline constexpr GridMapping GridMapping::ColumnsFirstProgressiveDeg90 = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Progressive, QuarterTurn::Deg90);
inline constexpr GridMapping GridMapping::ColumnsFirstProgressiveDeg180 = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Progressive, QuarterTurn::Deg180);
inline constexpr GridMapping GridMapping::ColumnsFirstProgressiveDeg270 = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Progressive, QuarterTurn::Deg270);

inline constexpr GridMapping GridMapping::ColumnsFirstSerpentine = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Serpentine, QuarterTurn::None);
inline constexpr GridMapping GridMapping::ColumnsFirstSerpentineDeg90 = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Serpentine, QuarterTurn::Deg90);
inline constexpr GridMapping GridMapping::ColumnsFirstSerpentineDeg180 = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Serpentine, QuarterTurn::Deg180);
inline constexpr GridMapping GridMapping::ColumnsFirstSerpentineDeg270 = GridMapping::make(AxisOrder::ColumnsFirst, LinePattern::Serpentine, QuarterTurn::Deg270);

struct TopologySettings
{
  uint16_t panelWidth;
  uint16_t panelHeight;
  GridMapping layout;
  uint16_t tilesWide;
  uint16_t tilesHigh;
  GridMapping tileLayout;
  bool mosaicRotation = false;
};

class Topology
{
public:
  static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

  static constexpr uint16_t mapLayout(GridMapping layout, uint16_t width, uint16_t height, uint16_t x, uint16_t y)
  {
    const auto [axisOrder, linePattern, quarterTurn] = layout.unpack();
    const bool columnMajor = axisOrder == GridMapping::AxisOrder::ColumnsFirst;
    const bool alternating = linePattern == GridMapping::LinePattern::Serpentine;
    const uint8_t rotation = static_cast<uint8_t>(quarterTurn);

    uint16_t rx = x;
    uint16_t ry = y;
    uint16_t rw = width;
    uint16_t rh = height;

    switch (rotation)
    {
      case 0:
        break;

      case 1:
        rx = y;
        ry = static_cast<uint16_t>(width - 1 - x);
        rw = height;
        rh = width;
        break;

      case 2:
        rx = static_cast<uint16_t>(width - 1 - x);
        ry = static_cast<uint16_t>(height - 1 - y);
        break;

      case 3:
        rx = static_cast<uint16_t>(height - 1 - y);
        ry = x;
        rw = height;
        rh = width;
        break;

      default:
        return 0;
    }

    if (!columnMajor)
    {
      const uint16_t rowBase = static_cast<uint16_t>(ry * rw);
      if (!alternating)
      {
        return static_cast<uint16_t>(rowBase + rx);
      }

      return (ry & 1) ? static_cast<uint16_t>(rowBase + (rw - 1 - rx)) : static_cast<uint16_t>(rowBase + rx);
    }

    const uint16_t colBase = static_cast<uint16_t>(rx * rh);
    if (!alternating)
    {
      return static_cast<uint16_t>(colBase + ry);
    }

    return (rx & 1) ? static_cast<uint16_t>(colBase + (rh - 1 - ry)) : static_cast<uint16_t>(colBase + ry);
  }

  // Returns the preferred per-tile mapping orientation for a tile position.
  // Input parity is tile-grid parity (not pixel-row/pixel-column parity).
  // This is used by mosaicRotation to keep adjacent tiles visually consistent.
  static constexpr GridMapping tilePreferredLayout(GridMapping baseLayout, bool isOddTileRow, bool isOddTileColumn)
  {
    const auto [axisOrder, linePattern, _] = baseLayout.unpack();

    if (linePattern == GridMapping::LinePattern::Progressive)
    {
      GridMapping::QuarterTurn quarterTurn = GridMapping::QuarterTurn::None;
      if (!isOddTileRow)
      {
        quarterTurn = isOddTileColumn ? GridMapping::QuarterTurn::Deg270 : GridMapping::QuarterTurn::None;
      }
      else
      {
        quarterTurn = isOddTileColumn ? GridMapping::QuarterTurn::Deg180 : GridMapping::QuarterTurn::Deg90;
      }

      return GridMapping::make(axisOrder, linePattern, quarterTurn);
    }

    if (axisOrder == GridMapping::AxisOrder::RowsFirst)
    {
      const auto quarterTurn = isOddTileRow ? GridMapping::QuarterTurn::Deg90 : GridMapping::QuarterTurn::Deg270;
      return GridMapping::make(axisOrder, linePattern, quarterTurn);
    }

    if (axisOrder == GridMapping::AxisOrder::ColumnsFirst)
    {
      const auto quarterTurn = isOddTileRow ? GridMapping::QuarterTurn::Deg180 : GridMapping::QuarterTurn::None;
      return GridMapping::make(axisOrder, linePattern, quarterTurn);
    }

    return baseLayout;
  }

  static constexpr Topology linear(size_t length)
  {
    return Topology{TopologySettings{static_cast<uint16_t>(length), 1, GridMapping::make(GridMapping::AxisOrder::RowsFirst, GridMapping::LinePattern::Progressive, GridMapping::QuarterTurn::None), 1, 1,
                                     GridMapping::make(GridMapping::AxisOrder::RowsFirst, GridMapping::LinePattern::Progressive, GridMapping::QuarterTurn::None), false}};
  }

  constexpr explicit Topology(TopologySettings config)
      : _config(config), _effectiveLayoutByTileParity(makeEffectiveLayoutCache(config)), _pixelCount(static_cast<size_t>(config.panelWidth) * config.panelHeight * config.tilesWide * config.tilesHigh)
  {
  }

  constexpr uint16_t width() const { return _config.panelWidth * _config.tilesWide; }

  constexpr uint16_t height() const { return _config.panelHeight * _config.tilesHigh; }

  constexpr size_t pixelCount() const { return _pixelCount; }

  constexpr bool isInBounds(int16_t x, int16_t y) const { return x >= 0 && y >= 0 && x < static_cast<int16_t>(width()) && y < static_cast<int16_t>(height()); }

  constexpr size_t map(int16_t x, int16_t y) const
  {
    if (!isInBounds(x, y) || _config.panelWidth == 0 || _config.panelHeight == 0)
    {
      return InvalidIndex;
    }

    uint16_t px = static_cast<uint16_t>(x);
    uint16_t py = static_cast<uint16_t>(y);

    uint16_t tileX = px / _config.panelWidth;
    uint16_t localX = px % _config.panelWidth;
    uint16_t tileY = py / _config.panelHeight;
    uint16_t localY = py % _config.panelHeight;

    uint16_t tileIndex = mapLayout(_config.tileLayout, _config.tilesWide, _config.tilesHigh, tileX, tileY);

    const bool isOddTileRow = (tileY & 1) != 0;
    const bool isOddTileColumn = (tileX & 1) != 0;
    const GridMapping effectiveLayout = _effectiveLayoutByTileParity[parityIndex(isOddTileRow, isOddTileColumn)];

    uint16_t localIndex = mapLayout(effectiveLayout, _config.panelWidth, _config.panelHeight, localX, localY);

    size_t panelPixels = static_cast<size_t>(_config.panelWidth) * _config.panelHeight;
    return static_cast<size_t>(tileIndex) * panelPixels + localIndex;
  }

  constexpr size_t panelPixelCount() const { return static_cast<size_t>(_config.panelWidth) * _config.panelHeight; }

  constexpr const TopologySettings& settings() const { return _config; }

  constexpr bool empty() const { return width() == 0 || height() == 0; }

private:
  static constexpr size_t parityIndex(bool isOddTileRow, bool isOddTileColumn) { return static_cast<size_t>((isOddTileRow ? 2u : 0u) | (isOddTileColumn ? 1u : 0u)); }

  static constexpr std::array<GridMapping, 4> makeEffectiveLayoutCache(const TopologySettings& config)
  {
    if (!config.mosaicRotation)
    {
      return std::array<GridMapping, 4>{config.layout, config.layout, config.layout, config.layout};
    }

    return std::array<GridMapping, 4>{tilePreferredLayout(config.layout, false, false), tilePreferredLayout(config.layout, false, true), tilePreferredLayout(config.layout, true, false),
                                      tilePreferredLayout(config.layout, true, true)};
  }

  TopologySettings _config;
  std::array<GridMapping, 4> _effectiveLayoutByTileParity;
  size_t _pixelCount;
};

} // namespace lw
