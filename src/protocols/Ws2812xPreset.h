#pragma once

#include "buses/BusBuilder.h"
#include "protocols/ChannelOrder.h"
#include "protocols/Ws2812xProtocol.h"

namespace lw::buses::presets
{

// ---------------------------------------------------------------------------
// ws2812x — protocol preset for WS2812/WS2812B/WS2811/SK6812 LED chips.
//
// Usage:
//   builder.addStrip(ws2812x{}, spi{});
//   builder.addStrip(ws2812x{.channelOrder = "RGB"}, rp_pio{2});
// ---------------------------------------------------------------------------

struct ws2812x
{
  const char* channelOrder = lw::ChannelOrder::GRB::value;

  void configure(BusBuilder& b) { b.setProtocol(lw::protocols::Ws2812xProtocol(static_cast<lw::PixelCount>(b.pixelCount()), lw::protocols::Ws2812xProtocolSettings{channelOrder})); }
};

} // namespace lw::buses::presets
