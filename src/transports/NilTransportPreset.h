#pragma once

#include "buses/BusBuilder.h"
#include "transports/NilTransport.h"

namespace lw::buses::presets
{

// ---------------------------------------------------------------------------
// nil_transport — transport preset for NilTransport (no-op, testing).
//
// Usage:
//   builder.addStrip(ws2812x{}, nil_transport{});
// ---------------------------------------------------------------------------

struct nil_transport
{
  void configure(BusBuilder& b) { b.setTransport(lw::transports::NilTransport{}); }
};

} // namespace lw::buses::presets
