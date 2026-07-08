#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>
#include <limits>

#include "core/Compat.h"

#if LW_HAS_ARDUINO
#include <Arduino.h>
#endif

#include "core/Writable.h"
#include "core/IOutputPipeline.h"

namespace lw::transports
{

#if LW_HAS_ARDUINO
using DefaultPrintLightDriverWritable = Print;
#else
using DefaultPrintLightDriverWritable = lw::detail::NullWritable;
#endif

template <typename TWritable = DefaultPrintLightDriverWritable, typename = std::enable_if_t<Writable<TWritable>>> struct PrintLightDriverSettingsT
{
  TWritable* output = nullptr;
  bool asciiOutput = false;
  bool debugOutput = false;
  const char* identifier = nullptr;

  static PrintLightDriverSettingsT<TWritable> normalize(PrintLightDriverSettingsT<TWritable> settings)
  {
#if defined(ARDUINO)
    if (settings.output == nullptr)
    {
      settings.output = &Serial;
    }
#endif
    return settings;
  }
};

template <typename TWritable, typename = std::enable_if_t<Writable<TWritable>>> class PrintLightDriverT : public IOutputPipeline
{
public:
  using ColorType = lw::Color;
  using BrightnessType = lw::colors::ColorComponent;
  using LightDriverSettingsType = PrintLightDriverSettingsT<TWritable>;

  explicit PrintLightDriverT(LightDriverSettingsType settings) : _settings(std::move(settings)) { captureIdentifier(); }

  explicit PrintLightDriverT(TWritable& output) : _settings{.output = &output} { captureIdentifier(); }

  void begin() override
  {
    if (_settings.debugOutput)
    {
      writeDebugPrefix();
      writeLine("begin");
    }
  }

  bool isReadyToUpdate() const override { return true; }

  bool alwaysUpdate() const override { return false; }

  void write(span<const ColorType> colors, BrightnessType brightness) override
  {
    if (_settings.output == nullptr || colors.empty())
    {
      return;
    }

    const auto& color = colors[0];

    if (_settings.debugOutput)
    {
      writeDebugPrefix();
      writeText("write bri=");
      writeUnsigned(static_cast<unsigned long>(brightness));
      writeNewline();
    }

    if (_settings.asciiOutput)
    {
      writeColorAscii(color);
      return;
    }

    writeColorBinary(color);
  }

private:
  void writeColorBinary(const ColorType& color)
  {
    using ComponentType = typename ColorType::ComponentType;
    using UnsignedComponentType = std::make_unsigned_t<ComponentType>;

    for (const char channelTag : {'R', 'G', 'B', 'W'})
    {
      const UnsignedComponentType component = static_cast<UnsignedComponentType>(color[channelTag]);
      for (size_t offset = 0; offset < sizeof(ComponentType); ++offset)
      {
        const size_t shift = (sizeof(ComponentType) - 1U - offset) * 8U;
        const uint8_t byte = static_cast<uint8_t>((component >> shift) & 0xFFU);
        writeBytes(&byte, 1);
      }
    }
  }

  void writeColorAscii(const ColorType& color)
  {
    using ComponentType = typename ColorType::ComponentType;
    using UnsignedComponentType = std::make_unsigned_t<ComponentType>;

    static constexpr char Hex[] = "0123456789ABCDEF";
    char componentBuffer[sizeof(ComponentType) * 2U]{};

    for (const char channelTag : {'R', 'G', 'B', 'W'})
    {
      UnsignedComponentType component = static_cast<UnsignedComponentType>(color[channelTag]);

      for (size_t nibble = 0; nibble < (sizeof(ComponentType) * 2U); ++nibble)
      {
        const size_t shift = ((sizeof(ComponentType) * 2U) - 1U - nibble) * 4U;
        componentBuffer[nibble] = Hex[(component >> shift) & 0x0FU];
      }

      writeBytes(reinterpret_cast<const uint8_t*>(componentBuffer), sizeof(componentBuffer));
    }
  }

  void writeBytes(const uint8_t* data, size_t length)
  {
    if (_settings.output == nullptr || data == nullptr || length == 0)
    {
      return;
    }

    _settings.output->write(data, length);
  }

  void writeText(const char* text)
  {
    if (text == nullptr)
    {
      return;
    }

    writeBytes(reinterpret_cast<const uint8_t*>(text), std::strlen(text));
  }

  void writeDebugPrefix()
  {
    writeText("[LIGHT");
    if (_settings.identifier != nullptr && _settings.identifier[0] != '\0')
    {
      writeText(":");
      writeText(_settings.identifier);
    }
    writeText("] ");
  }

  void writeLine(const char* text)
  {
    writeText(text);
    writeNewline();
  }

  void writeNewline() { writeText("\r\n"); }

  void writeUnsigned(unsigned long value)
  {
    char buffer[3 * sizeof(unsigned long)]{};
    size_t index = 0;

    do
    {
      if (index >= sizeof(buffer))
      {
        return;
      }

      const unsigned long digit = value % 10UL;
      buffer[index++] = static_cast<char>('0' + digit);
      value /= 10UL;
    } while (value > 0UL);

    for (size_t left = 0, right = index - 1; left < right; ++left, --right)
    {
      const char tmp = buffer[left];
      buffer[left] = buffer[right];
      buffer[right] = tmp;
    }

    writeBytes(reinterpret_cast<const uint8_t*>(buffer), index);
  }

  void captureIdentifier()
  {
    if (_settings.identifier == nullptr || _settings.identifier[0] == '\0')
    {
      return;
    }

    const size_t length = std::strlen(_settings.identifier);
    _identifierStorage.assign(_settings.identifier, _settings.identifier + length + 1U);
    _settings.identifier = _identifierStorage.data();
  }

  LightDriverSettingsType _settings;
  std::vector<char> _identifierStorage{};
};

#if LW_HAS_ARDUINO
using PrintLightDriverSettings = PrintLightDriverSettingsT<Print>;
using PrintLightDriver = PrintLightDriverT<lw::Color, Print>;
#endif

} // namespace lw::transports
