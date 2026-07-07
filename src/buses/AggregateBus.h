#pragma once

#include <memory>
#include <vector>
#include <limits>

#include "core/IPixelBus.h"

namespace lw::busses
{

class ReferenceAggregateBus : public IPixelBus
{
public:
  using BusType = IPixelBus;
  using BrightnessType = typename BusType::BrightnessType;
  using ChunkType = typename PixelView::ChunkType;

  explicit ReferenceAggregateBus(span<BusType*> buses) : _buses(buses.begin(), buses.end()) {}

  void begin() override
  {
    for (auto* bus : _buses)
    {
      if (bus != nullptr)
      {
        bus->begin();
      }
    }
  }

  void show() override
  {
    for (auto* bus : _buses)
    {
      if (bus != nullptr)
      {
        bus->show();
      }
    }
  }

  bool isReadyToUpdate() const override
  {
    for (const auto* bus : _buses)
    {
      if (bus == nullptr || !bus->isReadyToUpdate())
      {
        return false;
      }
    }

    return true;
  }

  PixelView& pixels() override { return _pixels; }

  const PixelView& pixels() const override { return _pixels; }

  void setBrightness(BrightnessType brightness) override
  {
    for (const auto& bus : _buses)
    {
      if (bus)
      {
        bus->setBrightness(brightness);
      }
    }
  }

  BrightnessType brightness() const override
  {
    for (const auto& bus : _buses)
    {
      if (bus)
      {
        return bus->brightness();
      }
    }

    return std::numeric_limits<BrightnessType>::max();
  }

private:
  std::vector<BusType*> _buses;
  std::vector<ChunkType> _pixelChunks;
  PixelView _pixels;
};

} // namespace lw::busses
