#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "buses/Bus.h"
#include "buses/ProtocolTransportPipeline.h"
#include "protocols/Protocol.h"
#include "protocols/ShaderProtocol.h"
#include "protocols/IShader.h"
#include "transports/Transport.h"

namespace lw::buses
{

template <typename TProtocol, typename TTransport, typename... TShaders> class PixelBus : public lw::IPixelBus
{
  static constexpr bool HasShaders = (sizeof...(TShaders) > 0);

  using ProtocolSettings = typename TProtocol::SettingsType;
  using TransportSettings = typename TTransport::TransportSettingsType;

  template <size_t... Is> void populateShaderPtrs(std::index_sequence<Is...>) { ((_shaderPtrs[Is] = &std::get<Is>(_shaders)), ...); }

public:
  // Default-constructs all shaders
  PixelBus(size_t pixelCount, ProtocolSettings protocolSettings = {}, TransportSettings transportSettings = {})
      : _pixelCount(pixelCount), _pixels(pixelCount), _protocol(pixelCount, protocolSettings), _transport(std::move(transportSettings)), _protocolBuffer(TProtocol::requiredBufferSize(pixelCount, protocolSettings)),
        _scratchPixels(HasShaders ? pixelCount : 0), _shaderPtrs(sizeof...(TShaders)),
        _shaderProto(_protocol, lw::span<lw::protocols::IShader*>{_shaderPtrs.data(), _shaderPtrs.size()}, lw::span<lw::Color>{_scratchPixels.data(), _scratchPixels.size()}),
        _pipeline(_shaderProto, _transport, lw::span<uint8_t>{_protocolBuffer.data(), _protocolBuffer.size()}), _run{&_pipeline, pixelCount},
        _bus(lw::span<lw::Color>{_pixels.data(), _pixels.size()}, lw::span<const lw::buses::PipelineRun>{&_run, 1})
  {
    populateShaderPtrs(std::index_sequence_for<TShaders...>{});
  }

  // Constructs shaders from per-shader std::tuple args via std::make_from_tuple
  template <typename... TShaderArgTuples, typename = std::enable_if_t<(sizeof...(TShaderArgTuples) > 0)>>
  PixelBus(size_t pixelCount, ProtocolSettings protocolSettings, TransportSettings transportSettings, TShaderArgTuples&&... shaderArgs)
      : _pixelCount(pixelCount), _pixels(pixelCount), _shaders{std::make_from_tuple<TShaders>(std::forward<TShaderArgTuples>(shaderArgs))...}, _protocol(pixelCount, protocolSettings), _transport(std::move(transportSettings)),
        _protocolBuffer(TProtocol::requiredBufferSize(pixelCount, protocolSettings)), _scratchPixels(HasShaders ? pixelCount : 0), _shaderPtrs(sizeof...(TShaders)),
        _shaderProto(_protocol, lw::span<lw::protocols::IShader*>{_shaderPtrs.data(), _shaderPtrs.size()}, lw::span<lw::Color>{_scratchPixels.data(), _scratchPixels.size()}),
        _pipeline(_shaderProto, _transport, lw::span<uint8_t>{_protocolBuffer.data(), _protocolBuffer.size()}), _run{&_pipeline, pixelCount},
        _bus(lw::span<lw::Color>{_pixels.data(), _pixels.size()}, lw::span<const lw::buses::PipelineRun>{&_run, 1})
  {
    populateShaderPtrs(std::index_sequence_for<TShaders...>{});
  }

  void begin() override { _bus.begin(); }
  void show() override { _bus.show(); }
  bool isReadyToUpdate() const override { return _bus.isReadyToUpdate(); }

  lw::span<lw::Color>& pixels() override { return _bus.pixels(); }
  const lw::span<lw::Color>& pixels() const override { return _bus.pixels(); }

  size_t pixelCount() const { return _pixelCount; }

private:
  size_t _pixelCount;
  TProtocol _protocol;
  TTransport _transport;
  std::vector<lw::Color> _pixels;
  std::vector<uint8_t> _protocolBuffer;
  std::vector<lw::Color> _scratchPixels;
  std::tuple<TShaders...> _shaders{};
  std::vector<lw::protocols::IShader*> _shaderPtrs;
  lw::protocols::ShaderProtocol _shaderProto;
  lw::buses::ProtocolTransportPipeline _pipeline;
  lw::buses::PipelineRun _run;
  lw::buses::Bus _bus;
};

} // namespace lw::buses
