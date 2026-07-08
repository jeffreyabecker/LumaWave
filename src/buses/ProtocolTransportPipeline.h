#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <limits>

#include "core/Compat.h"
#include "core/IOutputPipeline.h"
#include "colors/Color.h"
#include "colors/ColorMath.h"
#include "protocols/IProtocol.h"
#include "transports/ITransport.h"

namespace lw::buses
{

class ProtocolTransportPipeline : public IOutputPipeline
{
public:
  using BrightnessType = IOutputPipeline::BrightnessType;

  ProtocolTransportPipeline(protocols::IProtocol& protocol, transports::ITransport& transport, span<uint8_t> protocolBuffer, span<lw::colors::Color> scratchPixels)
      : _protocol(protocol), _transport(transport), _protocolBuffer(protocolBuffer), _scratchPixels(scratchPixels)
  {
  }

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

    const size_t requiredSize = _protocol.requiredBufferSizeBytes();
    if (requiredSize == 0)
    {
      return;
    }

    assert(_protocolBuffer.size() >= requiredSize);

    const bool applyBrightness = (brightness != std::numeric_limits<BrightnessType>::max());

    if (applyBrightness)
    {
      assert(!_scratchPixels.empty() && _scratchPixels.size() >= colors.size());

      for (size_t i = 0; i < colors.size(); ++i)
      {
        auto& dst = _scratchPixels[i];
        dst = colors[i];

        for (char channel : {'R', 'G', 'B', 'W'})
        {
          dst[channel] = static_cast<lw::colors::ColorComponent>(lw::colors::applyBrightness(colors[i][channel], brightness));
        }
      }

      _protocol.update(span<const lw::colors::Color>{_scratchPixels.data(), colors.size()}, _protocolBuffer);
    }
    else
    {
      _protocol.update(colors, _protocolBuffer);
    }

    _transport.beginTransaction();
    _transport.transmitBytes(_protocolBuffer);
    _transport.endTransaction();
  }

private:
  protocols::IProtocol& _protocol;
  transports::ITransport& _transport;
  span<uint8_t> _protocolBuffer;
  span<lw::colors::Color> _scratchPixels;
};

} // namespace lw::buses
