#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

#include "core/Compat.h"
#include "core/OutputPipeline.h"
#include "core/Color.h"
#include "protocols/Protocol.h"
#include "transports/Transport.h"

namespace lw::buses
{

class ProtocolTransportPipeline : public OutputPipeline
{
public:
  ProtocolTransportPipeline(protocols::Protocol& protocol, transports::Transport& transport, span<uint8_t> protocolBuffer) : _protocol(protocol), _transport(transport), _protocolBuffer(protocolBuffer) {}

  void begin() override
  {
    _transport.begin();

    _protocol.begin();
  }

  bool isReadyToUpdate() const override { return _transport.isReadyToUpdate(); }

  bool alwaysUpdate() const override { return _protocol.alwaysUpdate(); }

  void write(span<const lw::Color> colors) override
  {
    if (colors.empty())
    {
      return;
    }

    _protocol.update(colors, _protocolBuffer);

    _transport.beginTransaction();
    _transport.transmitBytes(_protocolBuffer);
    _transport.endTransaction();
  }

private:
  protocols::Protocol& _protocol;
  transports::Transport& _transport;
  span<uint8_t> _protocolBuffer;
};

} // namespace lw::buses
