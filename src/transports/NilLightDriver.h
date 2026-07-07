#pragma once

#include "transports/ILightDriver.h"

namespace lw::transports
{

struct NilLightDriverSettings : LightDriverSettingsBase
{
    static NilLightDriverSettings normalize(NilLightDriverSettings settings) { return settings; }
};

 class NilLightDriver : public ILightDriver
{
  public:
    using ColorType = lw::Color;
    using BrightnessType = typename ILightDriver::BrightnessType;
    using LightDriverSettingsType = NilLightDriverSettings;

    explicit NilLightDriver(LightDriverSettingsType = {}) {}

    void begin() override {}

    bool isReadyToUpdate() const override { return true; }

    void write(const ColorType&) override {}

    void write(const ColorType&, BrightnessType) override {}
};

} // namespace lw::transports
