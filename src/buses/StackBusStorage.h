#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include "buses/Bus.h"
#include "buses/ProtocolTransportPipeline.h"
#include "core/IPixelBus.h"
#include "core/Pixel.h"
#include "shaders/IShader.h"
#include "protocols/Protocol.h"
#include "shaders/ShaderProtocol.h"
#include "transports/Transport.h"

namespace lw::buses
{

// ---------------------------------------------------------------------------
// StackBusStorage — compile-time-sized, zero-heap IPixelBus container.
//
// Owns all sub-objects (pixels, protocol buffer, scratch, shaders,
// ShaderProtocol, ProtocolTransportPipeline, PipelineRun, Bus) in
// correct dependency order using compile-time-sized arrays. No heap
// allocation — suitable for -fno-rtti -fno-exceptions embedded builds.
//
// Used with BusBuilder::buildInto():
//   StackBusStorage<90, Ws2812xProtocol, RpPioTransport> storage;
//   auto& bus = BusBuilder().setPixelCount(90)
//       .addStrip(ws2812x{}, rp_pio{2})
//       .buildInto(storage);
// ---------------------------------------------------------------------------

template <size_t NPixelCount, typename TProtocol, typename TTransport, typename... TShaders> class StackBusStorage
{
  static constexpr bool kHasShaders = (sizeof...(TShaders) > 0);

  using ProtocolSettings = typename TProtocol::SettingsType;
  using TransportSettings = typename TTransport::TransportSettingsType;

  static constexpr size_t kProtocolBufferSize = TProtocol::requiredBufferSize(NPixelCount, ProtocolSettings{});

  // Populate shader pointer array from tuple elements.
  template <size_t... Is> void _populateShaderPtrs(std::index_sequence<Is...>) { ((_shaderPtrs[Is] = &std::get<Is>(_shaders)), ...); }

public:
  StackBusStorage(ProtocolSettings protocolSettings = {}, TransportSettings transportSettings = {})
      : _protocol(NPixelCount, std::move(protocolSettings)), _transport(std::move(transportSettings)),
        _shaderProto(_protocol, lw::span<lw::shaders::IShader*>{_shaderPtrs.data(), _shaderPtrs.size()}, lw::span<lw::Pixel>{_scratchPixels, kHasShaders ? NPixelCount : 0u}),
        _pipeline(_shaderProto, _transport, lw::span<uint8_t>{_protocolBuffer}), _runs{{&_pipeline, NPixelCount}}, _bus(lw::span<lw::Pixel>{_pixels}, lw::span<const PipelineRun>{_runs})
  {
    _populateShaderPtrs(std::index_sequence_for<TShaders...>{});
  }

  StackBusStorage(const StackBusStorage&) = delete;
  StackBusStorage& operator=(const StackBusStorage&) = delete;
  StackBusStorage(StackBusStorage&&) = delete;
  StackBusStorage& operator=(StackBusStorage&&) = delete;

  /// The contained Bus (implements IPixelBus).
  Bus& bus() { return _bus; }
  const Bus& bus() const { return _bus; }

private:
  // Declaration order = initialization order.
  lw::Pixel _pixels[NPixelCount]{};                                       // 1
  TProtocol _protocol;                                                    // 2
  TTransport _transport;                                                  // 3
  uint8_t _protocolBuffer[kProtocolBufferSize]{};                         // 4
  lw::Pixel _scratchPixels[kHasShaders ? NPixelCount : 1]{};              // 5
  std::tuple<TShaders...> _shaders{};                                     // 6
  std::array<lw::shaders::IShader*, sizeof...(TShaders)> _shaderPtrs{}; // 7
  lw::shaders::ShaderProtocol _shaderProto;                             // 8 (needs 2,5,7)
  ProtocolTransportPipeline _pipeline;                                    // 9 (needs 3,4,8)
  PipelineRun _runs[1];                                                   // 10 (needs 9)
  Bus _bus;                                                               // 11 (needs 1,10)
};

} // namespace lw::buses
