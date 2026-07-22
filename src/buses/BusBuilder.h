#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "buses/BusStorage.h"
#include "buses/detail/BufferManager.h"
#include "buses/detail/ProtocolHolder.h"
#include "buses/detail/ShaderList.h"
#include "buses/detail/TransportHolder.h"
#include "core/IPixelBus.h"
#include "core/Pixel.h"

namespace lw::buses
{

// ---------------------------------------------------------------------------
// BusBuilder — incrementally constructs a single-owner IPixelBus.
//
// Accumulates pixel storage, transport, protocol, and shader configuration,
// then finalizes via build() which returns a heap-allocated BusStorage as
// unique_ptr<IPixelBus>. The builder is consumed (moved-from) after build().
//
// Usage:
//   auto bus = BusBuilder()
//       .setPixelCount(30)
//       .setTransport(SpiTransport{})
//       .setProtocol(Ws2812xProtocol{30})
//       .build();
// ---------------------------------------------------------------------------

class BusBuilder
{
public:
  BusBuilder() = default;

  // Move-only (holders are move-only)
  BusBuilder(BusBuilder&&) noexcept = default;
  BusBuilder& operator=(BusBuilder&&) noexcept = default;
  BusBuilder(const BusBuilder&) = delete;
  BusBuilder& operator=(const BusBuilder&) = delete;

  /// Set dynamic pixel count. Allocates internal pixel storage.
  /// Mutually exclusive with setPixelStorage().
  BusBuilder& setPixelCount(size_t count)
  {
    _pixelCount = count;
    _hasExternalPixels = false;
    return *this;
  }

  /// Use externally-owned pixel storage. The provided span must outlive
  /// the returned IPixelBus. Mutually exclusive with setPixelCount().
  BusBuilder& setPixelStorage(lw::span<lw::Pixel> externalPixels)
  {
    _externalPixels = externalPixels;
    _hasExternalPixels = true;
    _pixelCount = externalPixels.size();
    return *this;
  }

  // --- Transport ---

  /// Attach a fully-constructed transport by move. The builder takes ownership.
  template <typename TTransport> BusBuilder& setTransport(TTransport transport)
  {
    _transport = detail::TransportHolder(std::move(transport));
    _transportSet = true;
    return *this;
  }

  // --- Protocol ---

  /// Attach a fully-constructed protocol by move. The builder caches the
  /// protocol buffer size from TProtocol::requiredBufferSize().
  template <typename TProtocol> BusBuilder& setProtocol(TProtocol protocol)
  {
    using SettingsType = typename TProtocol::SettingsType;
    _protocolBufferSize = TProtocol::requiredBufferSize(protocol.pixelCount(), static_cast<const SettingsType&>(protocol.settings()));
    _protocol = detail::ProtocolHolder(std::move(protocol));
    _protocolSet = true;
    return *this;
  }

  // --- Shaders ---

  /// Add a shader. Shaders apply in insertion order.
  template <typename TShader> BusBuilder& addShader(TShader shader)
  {
    _shaders.addShader(std::move(shader));
    return *this;
  }

  /// Enable destructive (in-place) shader mode. No scratch buffer allocated;
  /// shaders mutate the pixel buffer directly. Caller must fully repaint
  /// before each show().
  BusBuilder& enableDestructiveShaders()
  {
    _shaders.setDestructiveMode(true);
    return *this;
  }

  // --- Validation ---

  /// Returns an empty string on success, or an error description on failure.
  /// Does not allocate or consume the builder.
  std::string validate() const
  {
    if (_pixelCount == 0 && !_hasExternalPixels)
    {
      return "pixel count not set";
    }
    if (!_transportSet)
    {
      return "transport not set";
    }
    if (!_protocolSet)
    {
      return "protocol not set";
    }
    if (_hasExternalPixels && _externalPixels.empty())
    {
      return "external pixel storage is empty";
    }
    return {};
  }

  // --- Finalization ---

  /// Validate and build. Returns a heap-allocated IPixelBus on success,
  /// or nullptr on failure. The builder is consumed (moved-from) after
  /// this call.
  std::unique_ptr<lw::IPixelBus> build()
  {
    auto err = validate();
    if (!err.empty())
    {
      return nullptr;
    }

    auto storage = std::make_unique<BusStorage>(_pixelCount, std::move(_protocol), std::move(_transport), std::move(_shaders), _protocolBufferSize);

    // Mark as consumed
    _pixelCount = 0;
    _transportSet = false;
    _protocolSet = false;

    return storage;
  }

private:
  size_t _pixelCount = 0;
  bool _hasExternalPixels = false;
  lw::span<lw::Pixel> _externalPixels{};

  detail::ProtocolHolder _protocol;
  detail::TransportHolder _transport;
  detail::ShaderList _shaders;

  bool _protocolSet = false;
  bool _transportSet = false;
  size_t _protocolBufferSize = 0;
};

} // namespace lw::buses
