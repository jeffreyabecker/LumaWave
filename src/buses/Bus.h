#pragma once

#include <cstddef>
#include <limits>

#include "core/Compat.h"
#include "core/IPixelBus.h"
#include "core/IOutputPipeline.h"
#include "colors/Color.h"

namespace lw::buses
{

struct PipelineRun
{
  IOutputPipeline* pipeline{nullptr};
  size_t length{0};
};

class Bus : public IPixelBus
{
public:
  Bus(span<lw::colors::Color> pixelStorage, span<const PipelineRun> runs) : _pixels(pixelStorage), _runs(runs) {}

  void begin() override
  {
    for (auto& run : _runs)
    {
      if (run.pipeline)
      {
        run.pipeline->begin();
      }
    }
  }

  void show() override
  {
    if (!_dirty)
    {
      return;
    }

    if (!isReadyToUpdate())
    {
      return;
    }

    size_t offset = 0;
    for (auto& run : _runs)
    {
      if (run.pipeline && run.length > 0)
      {
        run.pipeline->write(span<const lw::colors::Color>{_pixels.data() + offset, run.length}, _brightness);
      }
      offset += run.length;
    }

    _dirty = false;
  }

  bool isReadyToUpdate() const override
  {
    for (const auto& run : _runs)
    {
      if (run.pipeline && !run.pipeline->isReadyToUpdate())
      {
        return false;
      }
    }

    return true;
  }

  span<lw::colors::Color>& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const span<lw::colors::Color>& pixels() const override { return _pixels; }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }

  BrightnessType brightness() const override { return _brightness; }

  span<const PipelineRun> runs() const { return _runs; }

private:
  span<lw::colors::Color> _pixels;
  span<const PipelineRun> _runs;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::buses
