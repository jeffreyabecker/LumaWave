#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
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

  ProtocolTransportPipeline(std::unique_ptr<protocols::IProtocol> protocol, std::unique_ptr<transports::ITransport> transport) : _protocol(std::move(protocol)), _transport(std::move(transport)) {}

  void begin() override
  {
    if (_transport)
    {
      _transport->begin();
    }

    if (_protocol)
    {
      _protocol->begin();
    }
  }

  bool isReadyToUpdate() const override
  {
    if (!_transport)
    {
      return false;
    }

    return _transport->isReadyToUpdate();
  }

  bool alwaysUpdate() const override
  {
    if (!_protocol)
    {
      return false;
    }

    return _protocol->alwaysUpdate();
  }

  void write(span<const lw::colors::Color> colors, BrightnessType brightness) override
  {
    if (!_protocol || !_transport || colors.empty())
    {
      return;
    }

    const size_t requiredSize = _protocol->requiredBufferSizeBytes();
    if (requiredSize == 0)
    {
      return;
    }

    if (_protocolBuffer.size() != requiredSize)
    {
      _protocolBuffer.assign(requiredSize, 0);
    }

    const bool applyBrightness = (brightness != std::numeric_limits<BrightnessType>::max());

    if (applyBrightness)
    {
      // On-the-fly brightness: apply per-channel inline during encoding pass.
      // Use a single-element reuse buffer to avoid full-frame scratch allocation.
      if (_scratchPixel.empty())
      {
        _scratchPixel.resize(colors.size());
      }
      else if (_scratchPixel.size() != colors.size())
      {
        _scratchPixel.assign(colors.size(), lw::colors::Color{});
      }

      for (size_t i = 0; i < colors.size(); ++i)
      {
        auto& dst = _scratchPixel[i];
        dst = colors[i];

        for (char channel : {'R', 'G', 'B', 'W'})
        {
          dst[channel] = static_cast<lw::colors::ColorComponent>(lw::colors::applyBrightness(colors[i][channel], brightness));
        }
      }

      span<uint8_t> protocolBytes{_protocolBuffer.data(), _protocolBuffer.size()};
      _protocol->update(span<const lw::colors::Color>{_scratchPixel.data(), _scratchPixel.size()}, protocolBytes);
    }
    else
    {
      span<uint8_t> protocolBytes{_protocolBuffer.data(), _protocolBuffer.size()};
      _protocol->update(colors, protocolBytes);
    }

    _transport->beginTransaction();
    _transport->transmitBytes(span<uint8_t>{_protocolBuffer.data(), _protocolBuffer.size()});
    _transport->endTransaction();
  }

private:
  std::unique_ptr<protocols::IProtocol> _protocol;
  std::unique_ptr<transports::ITransport> _transport;
  std::vector<uint8_t> _protocolBuffer;
  std::vector<lw::colors::Color> _scratchPixel;
};

} // namespace lw::buses
