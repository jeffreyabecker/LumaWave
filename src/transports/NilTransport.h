#pragma once

#include <cstdint>

#include "ITransport.h"

namespace lw::transports
{

struct NilTransportSettings : TransportSettingsBase
{
    static NilTransportSettings normalize(NilTransportSettings settings) { return settings; }
};

class NilTransport : public ITransport
{
  public:
    using TransportSettingsType = NilTransportSettings;
    explicit NilTransport(NilTransportSettings = {}) {}

    void begin() override {}

    void beginTransaction() override {}

    void endTransaction() override {}

    void transmitBytes(span<uint8_t>) override {}

    void transmitBytes(span<uint8_t>, TransportBrightness) override {}
};

} // namespace lw::transports
