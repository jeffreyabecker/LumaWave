#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "Protocol.h"
#include "IShader.h"
#include "core/Color.h"

namespace lw::protocols
{

class ShaderProtocol : public Protocol
{
public:
  ShaderProtocol(Protocol& inner, span<IShader*> shaders, span<lw::colors::Color> scratchPixels) : _inner(inner), _shaders(shaders), _scratchPixels(scratchPixels) {}

  PixelCount pixelCount() const { return _inner.pixelCount(); }

  void begin() override { _inner.begin(); }

  void update(span<const lw::colors::Color> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (_shaders.empty())
    {
      _inner.update(colors, buffer);
      return;
    }

    assert(!_scratchPixels.empty() && _scratchPixels.size() >= colors.size());

    span<const lw::colors::Color> src = colors;
    span<lw::colors::Color> dst = _scratchPixels;

    for (size_t i = 0; i < _shaders.size(); ++i)
    {
      _shaders[i]->apply(src, dst);
      src = dst;
    }

    _inner.update(dst.first(colors.size()), buffer);
  }

  ProtocolSettings& settings() override { return _inner.settings(); }

  bool alwaysUpdate() const override { return _inner.alwaysUpdate(); }

  void setRuntimeConfig(RuntimeConfig type, void* value) override
  {
    for (auto* shader : _shaders)
    {
      shader->setRuntimeConfig(type, value);
    }
    _inner.setRuntimeConfig(type, value);
  }

  void* getRuntimeConfig(RuntimeConfig type) override
  {
    for (auto* shader : _shaders)
    {
      void* result = shader->getRuntimeConfig(type);
      if (result != nullptr)
      {
        return result;
      }
    }
    return _inner.getRuntimeConfig(type);
  }

private:
  Protocol& _inner;
  span<IShader*> _shaders;
  span<lw::colors::Color> _scratchPixels;
};

} // namespace lw::protocols
