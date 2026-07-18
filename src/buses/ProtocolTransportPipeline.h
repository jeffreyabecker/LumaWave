#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

#include "core/Compat.h"
#include "core/IOutputPipeline.h"
#include "colors/Color.h"
#include "protocols/IProtocol.h"
#include "transports/ITransport.h"

namespace lw::buses
{

class ProtocolTransportPipeline : public IOutputPipeline
{
public:
  using BrightnessType = IOutputPipeline::BrightnessType;

  ProtocolTransportPipeline(protocols::IProtocol& protocol, transports::ITransport& transport, span<uint8_t> protocolBuffer) : _protocol(protocol), _transport(transport), _protocolBuffer(protocolBuffer) {}

  void begin() override
  {
    _transport.begin();

    _protocol.begin();
  }

  bool isReadyToUpdate() const override { return _transport.isReadyToUpdate(); }

  bool alwaysUpdate() const override { return _protocol.alwaysUpdate(); }

  void write(span<const lw::colors::Color> colors, BrightnessType brightness) override
  {
    if (colors.empty())
    {
      return;
    }

    _protocol.setRuntimeConfig(protocols::RuntimeConfig::Brightness, &brightness);
    _protocol.update(colors, _protocolBuffer);

    _transport.beginTransaction();
    _transport.transmitBytes(_protocolBuffer);
    _transport.endTransaction();
  }

private:
  protocols::IProtocol& _protocol;
  transports::ITransport& _transport;
  span<uint8_t> _protocolBuffer;
};

} // namespace lw::buses
