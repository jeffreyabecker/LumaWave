#pragma once

#include "core/IOutputPipeline.h"

namespace lw::transports
{

struct NilLightDriverSettings
{
  static NilLightDriverSettings normalize(NilLightDriverSettings settings) { return settings; }
};

class NilLightDriver : public lw::buses::IOutputPipeline
{
public:
  using ColorType = lw::Color;
  using BrightnessType = lw::colors::ColorComponent;
  using LightDriverSettingsType = NilLightDriverSettings;

  explicit NilLightDriver(LightDriverSettingsType = {}) {}

  void begin() override {}

  bool isReadyToUpdate() const override { return true; }

  bool alwaysUpdate() const override { return false; }

  void write(span<const ColorType> colors, BrightnessType brightness) override
  {
    (void)colors;
    (void)brightness;
  }
};

} // namespace lw::transports
