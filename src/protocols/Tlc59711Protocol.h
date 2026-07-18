#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <algorithm>
#include <type_traits>

#include "IProtocol.h"

namespace lw::protocols
{

// TLC59711 brightness and control configuration.
//
// Control flags:
//   outtmg  ? true = output on rising edge (default true)
//   extgck  ? true = use external clock on SCKI pin
//   tmgrst  ? true = enable display timer reset (default true)
//   dsprpt  ? true = enable auto display repeat (default true)
//   blank   ? true = outputs blanked
//
// Brightness: 7-bit global gain replicated to each RGB channel group (0?127).
//
struct Tlc59711Settings
{
  static constexpr uint8_t MaxBrightness = 127;

  bool outtmg{true};
  bool extgck{false};
  bool tmgrst{true};
  bool dsprpt{true};
  bool blank{false};
};

struct Tlc59711ProtocolSettings : public ProtocolSettings
{
  Tlc59711Settings config = {};
};

// TLC59711 protocol.
//
// SPI-like two-wire (clock + data), no chip-select.
// 12 channels per chip = 4 RGB pixels per chip.
// 16-bit per channel, big-endian (MSB first).
//
// Per-chip wire format (28 bytes):
//   [4-byte header] [24 bytes channel data]
//
// Header bit layout (32 bits, MSB first on wire):
//   bits [31:26] = 0b100101 (write command)
//   bit  [25]    = OUTTMG
//   bit  [24]    = EXTGCK
//   bit  [23]    = TMGRST
//   bit  [22]    = DSPRPT
//   bit  [21]    = BLANK
//   bits [20:14] = BC_Blue (7-bit)
//   bits [13:7]  = BC_Green (7-bit)
//   bits [6:0]   = BC_Red (7-bit)
//
// Data ordering is REVERSED: last chip transmitted first,
// and within each chip channels go BGR3, BGR2, BGR1, BGR0.
//
// Latch: ~20 ?s guard after transmission.
//
class Tlc59711ProtocolT : public IProtocol
{
public:
  using SettingsType = Tlc59711ProtocolSettings;

  static_assert((std::is_same_v<lw::colors::ColorComponent, uint8_t> || std::is_same_v<lw::colors::ColorComponent, uint16_t>), "Tlc59711Protocol requires uint8_t or uint16_t interface components.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType&)
  {
    const size_t chipCount = (static_cast<size_t>(pixelCount) + PixelsPerChip - 1u) / PixelsPerChip;
    return chipCount * BytesPerChip;
  }

  Tlc59711ProtocolT(PixelCount pixelCount, SettingsType settings)
      : IProtocol(pixelCount), _settings{std::move(settings)}, _chipCount{(pixelCount + PixelsPerChip - 1) / PixelsPerChip}, _requiredBufferSize(requiredBufferSize(pixelCount, _settings))
  {
    encodeHeader(_settings.config);
  }

  void begin() override {}

  void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (buffer.size() < _requiredBufferSize)
    {
      return;
    }

    _byteBuffer = span<uint8_t>{buffer.data(), _requiredBufferSize};

    // Serialize: reversed chip order, reversed pixel order within chip
    serialize(colors);
  }

  ProtocolSettings& settings() override { return _settings; }

  void updateSettings(const Tlc59711Settings& settings) { encodeHeader(settings); }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    if (type == RuntimeConfig::Brightness && value != nullptr)
    {
      _gainValue = *static_cast<uint8_t*>(value);
      encodeHeader(_settings.config);
    }
  }

  void* getRuntimeConfig(RuntimeConfig type) override
  {
    if (type == RuntimeConfig::Brightness)
    {
      return &_gainValue;
    }
    return nullptr;
  }

private:
  static constexpr uint8_t normalizeGainValue(uint8_t gain, uint8_t maxValue) { return static_cast<uint8_t>((static_cast<uint16_t>(gain) * static_cast<uint16_t>(maxValue) + 127u) / 255u); }

  static constexpr size_t PixelsPerChip = 4;
  static constexpr size_t ChannelsPerChip = 12;
  static constexpr size_t DataBytesPerChip = 24; // 12 ? 2
  static constexpr size_t HeaderBytesPerChip = 4;
  static constexpr size_t BytesPerChip = 28; // 4 + 24
  SettingsType _settings;
  size_t _chipCount;
  size_t _requiredBufferSize{0};
  span<uint8_t> _byteBuffer{};
  uint8_t _gainValue{0xff};
  std::array<uint8_t, HeaderBytesPerChip> _header{};

  void encodeHeader(const Tlc59711Settings& config)
  {
    const uint8_t normalizedGain = normalizeGainValue(_gainValue, Tlc59711Settings::MaxBrightness);
    uint8_t bcR = normalizedGain;
    uint8_t bcG = normalizedGain;
    uint8_t bcB = normalizedGain;

    // Control bits packed into one byte for convenience
    uint8_t ctrl = 0;
    if (config.outtmg)
      ctrl |= 0x02;
    if (config.extgck)
      ctrl |= 0x01;
    if (config.tmgrst)
      ctrl |= 0x80;
    if (config.dsprpt)
      ctrl |= 0x40;
    if (config.blank)
      ctrl |= 0x20;

    // byte[0] = 0b100101_OE  (write command + OUTTMG + EXTGCK)
    _header[0] = static_cast<uint8_t>(0x94 | (ctrl & 0x03));

    // byte[1] = 0bTDB_bbbbb  (TMGRST, DSPRPT, BLANK, BC_Blue[6:2])
    _header[1] = static_cast<uint8_t>((ctrl & 0xE0) | (bcB >> 2));

    // byte[2] = 0bbb_gggggg  (BC_Blue[1:0], BC_Green[6:1])
    _header[2] = static_cast<uint8_t>((bcB << 6) | (bcG >> 1));

    // byte[3] = 0bg_rrrrrrr  (BC_Green[0], BC_Red[6:0])
    _header[3] = static_cast<uint8_t>((bcG << 7) | bcR);
  }

  void serialize(span<const lw::colors::Color> colors)
  {
    // Walk chips in reverse order (last chip first on wire)
    size_t bufOffset = 0;

    for (size_t chip = _chipCount; chip > 0; --chip)
    {
      size_t chipStartPixel = (chip - 1) * PixelsPerChip;

      // Per-chip header (same for all chips)
      _byteBuffer[bufOffset++] = _header[0];
      _byteBuffer[bufOffset++] = _header[1];
      _byteBuffer[bufOffset++] = _header[2];
      _byteBuffer[bufOffset++] = _header[3];

      // Channel data: reversed pixel order within chip, BGR per pixel
      for (size_t px = PixelsPerChip; px > 0; --px)
      {
        size_t pixelIdx = chipStartPixel + (px - 1);

        uint16_t b = 0, g = 0, r = 0;
        if (pixelIdx < colors.size())
        {
          b = toWireComponent16(colors[pixelIdx]['B']);
          g = toWireComponent16(colors[pixelIdx]['G']);
          r = toWireComponent16(colors[pixelIdx]['R']);
        }

        // BGR order, big-endian 16-bit each
        _byteBuffer[bufOffset++] = static_cast<uint8_t>(b >> 8);
        _byteBuffer[bufOffset++] = static_cast<uint8_t>(b & 0xFF);
        _byteBuffer[bufOffset++] = static_cast<uint8_t>(g >> 8);
        _byteBuffer[bufOffset++] = static_cast<uint8_t>(g & 0xFF);
        _byteBuffer[bufOffset++] = static_cast<uint8_t>(r >> 8);
        _byteBuffer[bufOffset++] = static_cast<uint8_t>(r & 0xFF);
      }
    }
  }

  static constexpr uint16_t toWireComponent16(lw::colors::ColorComponent value)
  {
    if constexpr (std::is_same_v<lw::colors::ColorComponent, uint16_t>)
    {
      return value;
    }

    return static_cast<uint16_t>((static_cast<uint16_t>(value) << 8) | static_cast<uint16_t>(value));
  }
};

} // namespace lw::protocols
