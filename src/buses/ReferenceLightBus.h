#pragma once

#include <array>
#include <memory>
#include <limits>

#include "colors/ColorMath.h"
#include "core/IPixelBus.h"
#include "transports/ILightDriver.h"

namespace lw::busses
{

 class ReferenceLightBus : public IPixelBus
{
public:
  using BrightnessType = typename IPixelBus::BrightnessType;

  ReferenceLightBus(std::unique_ptr<transports::ILightDriver> driver)
      : _rootBuffer(std::make_unique<lw::Color>()), _driver(std::move(driver)), _pixels(makePixelChunk(_rootBuffer.get()))
  {
  }

  void begin() override
  {
    if (_driver)
    {
      _driver->begin();
    }
  }

  void show() override
  {
    if (!_driver || !_rootBuffer)
    {
      return;
    }

    if (!_dirty)
    {
      return;
    }

    if (!_driver->isReadyToUpdate())
    {
      return;
    }

    _driver->write(*_rootBuffer, _brightness);
    _dirty = false;
  }

  bool isReadyToUpdate() const override
  {
    if (!_driver)
    {
      return false;
    }

    return _driver->isReadyToUpdate();
  }

  PixelView& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const PixelView& pixels() const override { return _pixels; }

  lw::Color* rootBuffer() { return _rootBuffer.get(); }

  const lw::Color* rootBuffer() const { return _rootBuffer.get(); }

  transports::ILightDriver* driver() { return _driver.get(); }

  const transports::ILightDriver* driver() const { return _driver.get(); }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

private:
  static span<lw::Color> makePixelChunk(lw::Color* buffer)
  {
    if (buffer == nullptr)
    {
      return span<lw::Color>{};
    }

    return span<lw::Color>{buffer, 1u};
  }

  std::unique_ptr<lw::Color> _rootBuffer;
  std::unique_ptr<transports::ILightDriver> _driver;
  PixelView _pixels;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses
