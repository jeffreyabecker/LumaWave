#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "buses/Bus.h"
#include "buses/ProtocolTransportPipeline.h"
#include "buses/detail/BufferManager.h"
#include "buses/detail/ProtocolHolder.h"
#include "buses/detail/ShaderList.h"
#include "buses/detail/TransportHolder.h"
#include "core/IPixelBus.h"
#include "core/Pixel.h"
#include "shaders/ShaderProtocol.h"

namespace lw::buses
{

// ---------------------------------------------------------------------------
// BusStorage — single-owner RAII struct for a single-run IPixelBus.
//
// Owns all sub-objects (pixels, protocol, transport, shaders, buffers,
// ShaderProtocol, ProtocolTransportPipeline, PipelineRun, Bus) in
// correct dependency order. Non-copyable, non-movable (internal references
// would dangle).
//
// Constructed by BusBuilder::build() and returned as unique_ptr<IPixelBus>.
//
// Multi-strip configurations are achieved by using separate BusBuilder
// instances with setPixelStorage() over sub-spans of a shared pixel buffer.
// ---------------------------------------------------------------------------

class BusStorage : public lw::IPixelBus
{
public:
  /// Owned pixel storage: allocates pixelCount pixels internally.
  BusStorage(size_t pixelCount, detail::ProtocolHolder protocol, detail::TransportHolder transport, detail::ShaderList shaders, size_t protocolBufferSize)
      : _ownedPixels(pixelCount), _protocol(std::move(protocol)), _transport(std::move(transport)), _shaders(std::move(shaders)), _buffers(protocolBufferSize, pixelCount, _shaders.needsScratchBuffer()),
        _shaderProto(*_protocol.get(), _shaders.shaders(), _buffers.scratchPixels()), _pipeline(_shaderProto, *_transport.get(), _buffers.protocolBuffer()), _run{&_pipeline, pixelCount},
        _bus(lw::span<lw::Pixel>{_ownedPixels.data(), _ownedPixels.size()}, lw::span<const PipelineRun>{&_run, 1})
  {
  }

  /// External pixel storage: the provided span must outlive this object.
  BusStorage(lw::span<lw::Pixel> externalPixels, detail::ProtocolHolder protocol, detail::TransportHolder transport, detail::ShaderList shaders, size_t protocolBufferSize)
      : _protocol(std::move(protocol)), _transport(std::move(transport)), _shaders(std::move(shaders)), _buffers(protocolBufferSize, externalPixels.size(), _shaders.needsScratchBuffer()),
        _shaderProto(*_protocol.get(), _shaders.shaders(), _buffers.scratchPixels()), _pipeline(_shaderProto, *_transport.get(), _buffers.protocolBuffer()), _run{&_pipeline, externalPixels.size()},
        _bus(externalPixels, lw::span<const PipelineRun>{&_run, 1})
  {
  }

  BusStorage(const BusStorage&) = delete;
  BusStorage& operator=(const BusStorage&) = delete;
  BusStorage(BusStorage&&) = delete;
  BusStorage& operator=(BusStorage&&) = delete;

  // --- IPixelBus ---

  void begin() override { _bus.begin(); }
  void show() override { _bus.show(); }
  bool isReadyToUpdate() const override { return _bus.isReadyToUpdate(); }

  lw::span<lw::Pixel>& pixels() override { return _bus.pixels(); }
  const lw::span<lw::Pixel>& pixels() const override { return _bus.pixels(); }

  void setRuntimeConfig(lw::RuntimeConfig type, void* value) override { _bus.setRuntimeConfig(type, value); }

private:
  // Declaration order = initialization order (dependencies go top-to-bottom).
  std::vector<lw::Pixel> _ownedPixels;        // 1 (empty when external)
  detail::ProtocolHolder _protocol;           // 2
  detail::TransportHolder _transport;         // 3
  detail::ShaderList _shaders;                // 4
  detail::BufferManager _buffers;             // 5 (needs 4)
  lw::shaders::ShaderProtocol _shaderProto; // 6 (needs 2,4,5)
  ProtocolTransportPipeline _pipeline;        // 7 (needs 3,5,6)
  PipelineRun _run;                           // 8 (needs 7)
  Bus _bus;                                   // 9 (needs 1,8)
};

} // namespace lw::buses
