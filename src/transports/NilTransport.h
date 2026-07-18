#pragma once

#include "Transport.h"

namespace lw::transports
{

struct NilTransportSettings
{
};

class NilTransport : public Transport
{
public:
  using TransportSettingsType = NilTransportSettings;

  explicit NilTransport(NilTransportSettings = {}) {}
};

} // namespace lw::transports
