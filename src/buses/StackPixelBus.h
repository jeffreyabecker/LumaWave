#pragma once

/// @deprecated Use StackBusStorage + BusBuilder::buildInto() instead.
/// StackPixelBus remains available for backward compatibility but
/// StackBusStorage provides the same zero-heap result decoupled from the
/// builder, with BusBuilder handling validation. See docs/usage/bus-builder.md.

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

#include "buses/Bus.h"
#include "buses/ProtocolTransportPipeline.h"
#include "protocols/Protocol.h"
#include "protocols/ShaderProtocol.h"
#include "protocols/IShader.h"
#include "transports/Transport.h"

namespace lw::buses
{

template <size_t NPixelCount, typename TProtocol, typename TTransport, typename... TShaders> class StackPixelBus : public lw::IPixelBus
{
  static constexpr bool HasShaders = (sizeof...(TShaders) > 0);

  using ProtocolSettings = typename TProtocol::SettingsType;
  using TransportSettings = typename TTransport::TransportSettingsType;

  static constexpr size_t ProtocolBufferSize = TProtocol::requiredBufferSize(NPixelCount, ProtocolSettings{});

  template <size_t... Is> void populateShaderPtrs(std::index_sequence<Is...>) { ((_shaderPtrs[Is] = &std::get<Is>(_shaders)), ...); }

public:
  // Default-constructs all shaders
  StackPixelBus(ProtocolSettings protocolSettings = {}, TransportSettings transportSettings = {})
      : _protocol(NPixelCount, std::move(protocolSettings)), _transport(std::move(transportSettings)),
        _shaderProto(_protocol, lw::span<lw::protocols::IShader*>{_shaderPtrs.data(), _shaderPtrs.size()}, lw::span<lw::Pixel>{_scratchPixels, HasShaders ? NPixelCount : 0u}),
        _pipeline(_shaderProto, _transport, lw::span<uint8_t>{_protocolBuffer}), _run{&_pipeline, NPixelCount}, _bus(lw::span<lw::Pixel>{_pixels}, lw::span<const lw::buses::PipelineRun>{&_run, 1})
  {
    populateShaderPtrs(std::index_sequence_for<TShaders...>{});
  }

  // Constructs shaders from per-shader std::tuple args via std::make_from_tuple
  template <typename... TShaderArgTuples, typename = std::enable_if_t<(sizeof...(TShaderArgTuples) > 0)>>
  StackPixelBus(ProtocolSettings protocolSettings, TransportSettings transportSettings, TShaderArgTuples&&... shaderArgs)
      : _shaders{std::make_from_tuple<TShaders>(std::forward<TShaderArgTuples>(shaderArgs))...}, _protocol(NPixelCount, std::move(protocolSettings)), _transport(std::move(transportSettings)),
        _shaderProto(_protocol, lw::span<lw::protocols::IShader*>{_shaderPtrs.data(), _shaderPtrs.size()}, lw::span<lw::Pixel>{_scratchPixels, HasShaders ? NPixelCount : 0u}),
        _pipeline(_shaderProto, _transport, lw::span<uint8_t>{_protocolBuffer}), _run{&_pipeline, NPixelCount}, _bus(lw::span<lw::Pixel>{_pixels}, lw::span<const lw::buses::PipelineRun>{&_run, 1})
  {
    populateShaderPtrs(std::index_sequence_for<TShaders...>{});
  }

  void begin() override { _bus.begin(); }
  void show() override { _bus.show(); }
  bool isReadyToUpdate() const override { return _bus.isReadyToUpdate(); }

  lw::span<lw::Pixel>& pixels() override { return _bus.pixels(); }
  const lw::span<lw::Pixel>& pixels() const override { return _bus.pixels(); }

private:
  lw::Pixel _pixels[NPixelCount]{};
  TProtocol _protocol;
  TTransport _transport;
  uint8_t _protocolBuffer[ProtocolBufferSize]{};
  lw::Pixel _scratchPixels[HasShaders ? NPixelCount : 1]{};
  std::tuple<TShaders...> _shaders{};
  std::array<lw::protocols::IShader*, sizeof...(TShaders)> _shaderPtrs{};
  lw::protocols::ShaderProtocol _shaderProto;
  lw::buses::ProtocolTransportPipeline _pipeline;
  lw::buses::PipelineRun _run;
  lw::buses::Bus _bus;
};

} // namespace lw::buses
