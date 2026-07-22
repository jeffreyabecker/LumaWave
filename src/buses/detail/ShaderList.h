#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "core/Compat.h"
#include "core/Pixel.h"
#include "core/RuntimeConfig.h"
#include "protocols/IShader.h"

namespace lw::buses::detail
{

// ---------------------------------------------------------------------------
// ShaderList — owning, type-erased list of shaders.
// Stores unique_ptr<IShader> for lifetime management and produces
// a span<IShader*> for consumption by ShaderProtocol.
// Shaders apply in insertion order.
// ---------------------------------------------------------------------------

class ShaderList
{
public:
  ShaderList() = default;

  /// Construct a shader in place and take ownership.
  template <typename TShader> void addShader(TShader shader)
  {
    auto ptr = std::make_unique<TShader>(std::move(shader));
    _shaderPtrs.push_back(ptr.get());
    _shaders.push_back(std::move(ptr));
  }

  /// Number of shaders currently stored.
  size_t size() const { return _shaderPtrs.size(); }

  /// True when no shaders are stored.
  bool empty() const { return _shaderPtrs.empty(); }

  /// True when a scratch pixel buffer is required for shader transforms.
  /// This is the signal BufferManager uses to decide whether to allocate
  /// the scratch buffer alongside the protocol buffer.
  /// Returns false when shaders are absent OR when destructive mode is enabled
  /// (shaders mutate the pixel buffer in place; caller must repaint every frame).
  bool needsScratchBuffer() const { return !_shaderPtrs.empty() && !_destructive; }

  /// Enable destructive (in-place) shader mode.
  /// When destructive mode is on, shaders operate directly on the pixel buffer
  /// without a scratch allocation. The caller must fully repaint the pixel
  /// buffer before each show() — the buffer is in an undefined state afterward.
  /// Scratch mode (default) is safe for incremental updates.
  void setDestructiveMode(bool destructive) { _destructive = destructive; }

  /// True when destructive mode is active.
  bool isDestructiveMode() const { return _destructive; }

  /// Return a span of raw IShader* suitable for ShaderProtocol.
  lw::span<lw::protocols::IShader*> shaders() { return lw::span<lw::protocols::IShader*>{_shaderPtrs.data(), _shaderPtrs.size()}; }

  lw::span<lw::protocols::IShader* const> shaders() const { return lw::span<lw::protocols::IShader* const>{_shaderPtrs.data(), _shaderPtrs.size()}; }

  /// Forward setRuntimeConfig to every shader.
  void setRuntimeConfig(lw::RuntimeConfig type, void* value)
  {
    for (auto* shader : _shaderPtrs)
    {
      shader->setRuntimeConfig(type, value);
    }
  }

private:
  std::vector<std::unique_ptr<lw::protocols::IShader>> _shaders;
  std::vector<lw::protocols::IShader*> _shaderPtrs;
  bool _destructive = false;
};

} // namespace lw::buses::detail
