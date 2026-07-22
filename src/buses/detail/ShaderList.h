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
  bool needsScratchBuffer() const { return !_shaderPtrs.empty(); }

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
};

} // namespace lw::buses::detail
