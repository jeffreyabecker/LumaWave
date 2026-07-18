#pragma once

#include <cstdint>
#include <cstddef>

#include "core/Color.h"
#include "core/RuntimeConfig.h"
#include "protocols/ChannelOrder.h"
#include "core/Compat.h"
namespace lw::protocols
{

using lw::RuntimeConfig;

struct ProtocolSettings
{
};

class Protocol
{
public:
  using ColorType = lw::Color;
  using SettingsType = void;
  static constexpr bool RequiresExternalBuffer = true;
  explicit Protocol(PixelCount pixelCount = 0) : _pixelCount{pixelCount} {}

  virtual ~Protocol() = default;

  PixelCount pixelCount() const { return _pixelCount; }

  virtual void begin() {}
  virtual void update(span<const lw::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) {}
  virtual ProtocolSettings& settings() { return _settings; }
  virtual bool alwaysUpdate() const { return false; }

  virtual void setRuntimeConfig(RuntimeConfig type, void* value)
  {
    (void)type;
    (void)value;
  }

protected:
  PixelCount _pixelCount;
  ProtocolSettings _settings{};
};

} // namespace lw::protocols
