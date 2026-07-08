#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <limits>

#include "core/Compat.h"
#include "core/IPixelBus.h"
#include "core/IOutputPipeline.h"
#include "colors/Color.h"

namespace lw::busses
{

struct PipelineRun
{
  std::unique_ptr<IOutputPipeline> pipeline;
  size_t length;
};

class Bus : public IPixelBus
{
public:
  Bus(span<lw::colors::Color> pixelStorage, std::initializer_list<PipelineRun> runs) : _pixels(pixelStorage)
  {
    size_t total = 0;
    for (auto& run : runs)
    {
      total += run.length;
      _runs.push_back({std::move(const_cast<PipelineRun&>(run).pipeline), run.length});
    }
  }

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

  const std::vector<PipelineRun>& runs() const { return _runs; }

private:
  span<lw::colors::Color> _pixels;
  std::vector<PipelineRun> _runs;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses
