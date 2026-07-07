#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <cstring>

#include "IProtocol.h"
#include "NilProtocol.h"
#include "ProtocolDecoratorBase.h"
#include "core/Compat.h"
#include "core/Writable.h"

namespace lw::protocols
{

#if LW_HAS_ARDUINO
using DefaultDebugWritable = Print;
#else
using DefaultDebugWritable = lw::detail::NullWritable;
#endif

template <typename TWrappedProtocol = NilProtocol<Rgbw8Color>, typename TWritable = DefaultDebugWritable, typename = std::enable_if_t<Writable<TWritable>>> struct DebugProtocolSettingsT : public ProtocolSettings
{
  using WrappedSettingsType = typename TWrappedProtocol::SettingsType;

  WrappedSettingsType wrapped{};
  TWritable* output = nullptr;
  bool invert = false;
};

template <typename TWrappedProtocol = NilProtocol<Rgbw8Color>, typename TWritable = DefaultDebugWritable, typename = std::enable_if_t<Writable<TWritable>>>
class DebugProtocol : public ProtocolDecoratorBase<DebugProtocol<TWrappedProtocol, TWritable>, TWrappedProtocol, typename TWrappedProtocol::ColorType, DebugProtocolSettingsT<TWrappedProtocol, TWritable>>
{
public:
  using ColorType = typename TWrappedProtocol::ColorType;
  using BaseType = ProtocolDecoratorBase<DebugProtocol<TWrappedProtocol, TWritable>, TWrappedProtocol, ColorType, DebugProtocolSettingsT<TWrappedProtocol, TWritable>>;
  using SettingsType = DebugProtocolSettingsT<TWrappedProtocol, TWritable>;

  static_assert(std::is_base_of_v<IProtocol<ColorType>, TWrappedProtocol>, "DebugProtocol<TWrappedProtocol> requires TWrappedProtocol to derive from IProtocol<ColorType>.");

  static constexpr size_t requiredBufferSize(PixelCount pixelCount, const SettingsType& settings) { return TWrappedProtocol::requiredBufferSize(pixelCount, settings.wrapped); }

  DebugProtocol(PixelCount pixelCount, SettingsType settings) : BaseType(pixelCount, TWrappedProtocol{pixelCount, settings.wrapped}, std::move(settings)) {}

  void afterBegin()
  {
    const SettingsType& settings = this->decoratorSettings();
    if (settings.output != nullptr)
    {
      writeText("[PROTOCOL] begin pixelCount=");

      char countBuffer[8]{};
      const size_t countLength = formatUnsignedDecimal(countBuffer, sizeof(countBuffer), static_cast<unsigned long>(this->pixelCount()));
      if (countLength > 0)
      {
        writeBytes(reinterpret_cast<const uint8_t*>(countBuffer), countLength);
      }

      writeNewline();
    }
  }

  void beforeUpdate(span<const ColorType> colors, span<uint8_t> buffer = span<uint8_t>{})
  {
    (void)buffer;
    const SettingsType& settings = this->decoratorSettings();
    if (settings.output != nullptr)
    {
      static constexpr char HexDigits[] = "0123456789ABCDEF";

      writeText("[PROTOCOL] colors(");

      char countBuffer[3 * sizeof(unsigned long)]{};
      const size_t countLength = formatUnsignedDecimal(countBuffer, sizeof(countBuffer), static_cast<unsigned long>(colors.size()));
      if (countLength > 0)
      {
        writeBytes(reinterpret_cast<const uint8_t*>(countBuffer), countLength);
      }

      writeText("): ");

      for (size_t colorIndex = 0; colorIndex < colors.size(); ++colorIndex)
      {
        if (colorIndex > 0)
        {
          static constexpr char Space[] = " ";
          writeBytes(reinterpret_cast<const uint8_t*>(Space), sizeof(Space) - 1);
        }

        const auto& color = colors[colorIndex];
        for (char channel : {'R', 'G', 'B', 'W'})
        {
          using ComponentType = typename ColorType::ComponentType;
          using UnsignedComponentType = std::make_unsigned_t<ComponentType>;

          UnsignedComponentType value = static_cast<UnsignedComponentType>(color[channel]);
          if (settings.invert)
          {
            value = static_cast<UnsignedComponentType>(~value);
          }

          int shift = static_cast<int>(sizeof(UnsignedComponentType) * 8) - 4;
          while (shift >= 0)
          {
            const uint8_t nibble = static_cast<uint8_t>((value >> shift) & 0x0F);
            char hexCharBuffer[1] = {HexDigits[nibble]};
            writeBytes(reinterpret_cast<const uint8_t*>(hexCharBuffer), sizeof(hexCharBuffer));
            shift -= 4;
          }
        }
      }

      writeNewline();
    }
  }

  void afterUpdate(span<const ColorType> colors, span<uint8_t> buffer = span<uint8_t>{})
  {
    (void)colors;
    (void)buffer;
  }

private:
  void writeBytes(const uint8_t* data, size_t length)
  {
    SettingsType& settings = this->decoratorSettings();
    if (settings.output == nullptr || data == nullptr || length == 0)
    {
      return;
    }

    settings.output->write(data, length);
  }

  void writeText(const char* text)
  {
    if (text == nullptr)
    {
      return;
    }

    writeBytes(reinterpret_cast<const uint8_t*>(text), std::strlen(text));
  }

  void writeNewline()
  {
    static constexpr char Newline[] = "\r\n";
    writeBytes(reinterpret_cast<const uint8_t*>(Newline), sizeof(Newline) - 1);
  }

  static size_t formatUnsignedDecimal(char* buffer, size_t capacity, unsigned long value)
  {
    if (buffer == nullptr || capacity == 0)
    {
      return 0;
    }

    size_t index = 0;
    do
    {
      if (index >= capacity)
      {
        return 0;
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

    return index;
  }
};

} // namespace lw::protocols
