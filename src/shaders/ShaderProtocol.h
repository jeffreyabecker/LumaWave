#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "../protocols/Protocol.h"
#include "IShader.h"
#include "core/Pixel.h"

namespace lw::shaders
{

class ShaderProtocol : public lw::protocols::Protocol
{
public:
  ShaderProtocol(lw::protocols::Protocol& inner, span<IShader*> shaders, span<lw::Pixel> scratchPixels) : _inner(inner), _shaders(shaders), _scratchPixels(scratchPixels) {}

  lw::PixelCount pixelCount() const { return _inner.pixelCount(); }

  void begin() override { _inner.begin(); }

  void update(span<const lw::Pixel> colors, span<uint8_t> buffer = span<uint8_t>{}) override
  {
    if (_shaders.empty())
    {
      _inner.update(colors, buffer);
      return;
    }

    if (_scratchPixels.empty())
    {
      // Destructive mode: operate directly on the input pixel buffer.
      // The caller must fully repaint before each show() — the pixel buffer
      // is in an undefined (post-shader) state afterward.
      auto* mutablePixels = const_cast<lw::Pixel*>(colors.data());
      lw::span<lw::Pixel> pixels{mutablePixels, colors.size()};
      for (size_t i = 0; i < _shaders.size(); ++i)
      {
        _shaders[i]->apply(pixels, pixels);
      }
      _inner.update(pixels, buffer);
      return;
    }

    // Scratch mode: ping-pong through scratch buffer.
    // Only the first shader needs the separate destination; subsequent
    // shaders operate in-place on the scratch buffer itself.
    assert(_scratchPixels.size() >= colors.size());

    span<const lw::Pixel> src = colors;
    span<lw::Pixel> dst = _scratchPixels;

    for (size_t i = 0; i < _shaders.size(); ++i)
    {
      _shaders[i]->apply(src, dst);
      src = dst;
    }

    _inner.update(dst.first(colors.size()), buffer);
  }

  lw::protocols::ProtocolSettings& settings() override { return _inner.settings(); }

  bool alwaysUpdate() const override { return _inner.alwaysUpdate(); }

  void setRuntimeConfig(lw::RuntimeConfig type, void* value) override
  {
    for (auto* shader : _shaders)
    {
      shader->setRuntimeConfig(type, value);
    }
    _inner.setRuntimeConfig(type, value);
  }

private:
  lw::protocols::Protocol& _inner;
  span<IShader*> _shaders;
  span<lw::Pixel> _scratchPixels;
};

} // namespace lw::shaders
