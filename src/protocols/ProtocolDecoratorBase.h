#pragma once

#include <cstdint>
#include <utility>

#include "IProtocol.h"

namespace lw::protocols
{

template <typename TDerived, typename TWrappedProtocol, typename TSettings> class ProtocolDecoratorBase : public IProtocol
{
public:
  using WrappedProtocolType = TWrappedProtocol;
  using SettingsType = TSettings;

  ProtocolDecoratorBase(PixelCount pixelCount, WrappedProtocolType wrappedProtocol, SettingsType settings) : IProtocol(pixelCount), _wrappedProtocol{std::move(wrappedProtocol)}, _settings{std::move(settings)} {}

  ProtocolSettings& settings() override { return _settings; }

  void begin() override
  {
    _wrappedProtocol.begin();

    static_cast<TDerived*>(this)->afterBegin();
  }

  void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    static_cast<TDerived*>(this)->beforeUpdate(colors, buffer);

    _wrappedProtocol.update(colors, buffer);

    static_cast<TDerived*>(this)->afterUpdate(colors, buffer);
  }

  bool alwaysUpdate() const override { return _wrappedProtocol.alwaysUpdate(); }

  size_t requiredBufferSizeBytes() const override { return _wrappedProtocol.requiredBufferSizeBytes(); }

protected:
  WrappedProtocolType& wrappedProtocol() { return _wrappedProtocol; }

  const WrappedProtocolType& wrappedProtocol() const { return _wrappedProtocol; }

  SettingsType& decoratorSettings() { return _settings; }

  const SettingsType& decoratorSettings() const { return _settings; }

private:
  WrappedProtocolType _wrappedProtocol;
  SettingsType _settings;
};

} // namespace lw::protocols
