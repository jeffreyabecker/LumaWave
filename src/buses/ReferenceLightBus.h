#pragma once

#include <array>
#include <memory>
#include <limits>

#include "colors/IShader.h"
#include "colors/ColorMath.h"
#include "core/IPixelBus.h"
#include "transports/ILightDriver.h"

namespace lw::busses
{

template <typename TColor> class ReferenceLightBus : public IPixelBus<TColor>
{
public:
  using BrightnessType = typename IPixelBus<TColor>::BrightnessType;

  ReferenceLightBus(std::unique_ptr<transports::ILightDriver<TColor>> driver, std::unique_ptr<IShader<TColor>> shader = nullptr)
      : _rootBuffer(std::make_unique<TColor>()), _driver(std::move(driver)), _shader(std::move(shader)), _shaderBuffer(std::make_unique<TColor>()), _pixels(makePixelChunk(_rootBuffer.get()))
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

    const TColor* outputPixel = _rootBuffer.get();
    BrightnessType effectiveBrightness = _brightness;
    if (_shader && _shaderBuffer)
    {
      *_shaderBuffer = *_rootBuffer;
      span<TColor> shaderSpan{_shaderBuffer.get(), 1u};
      _shader->apply(shaderSpan);
      if (_shader->brightnessOwnership() == shaders::BrightnessOwnership::Owns)
      {
        _shader->applyBrightness(shaderSpan, _brightness);
        effectiveBrightness = std::numeric_limits<BrightnessType>::max();
      }
      outputPixel = _shaderBuffer.get();
    }
    else if (_shader)
    {
      span<TColor> rootSpan{_rootBuffer.get(), 1u};
      _shader->apply(rootSpan);
      if (_shader->brightnessOwnership() == shaders::BrightnessOwnership::Owns)
      {
        _shader->applyBrightness(rootSpan, _brightness);
        effectiveBrightness = std::numeric_limits<BrightnessType>::max();
      }
      outputPixel = _rootBuffer.get();
    }

    _driver->write(*outputPixel, effectiveBrightness);
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

  PixelView<TColor>& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const PixelView<TColor>& pixels() const override { return _pixels; }

  TColor* rootBuffer() { return _rootBuffer.get(); }

  const TColor* rootBuffer() const { return _rootBuffer.get(); }

  transports::ILightDriver<TColor>* driver() { return _driver.get(); }

  const transports::ILightDriver<TColor>* driver() const { return _driver.get(); }

  IShader<TColor>* shader() { return _shader.get(); }

  const IShader<TColor>* shader() const { return _shader.get(); }

  TColor* shaderBuffer() { return _shaderBuffer.get(); }

  const TColor* shaderBuffer() const { return _shaderBuffer.get(); }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

private:
  static span<TColor> makePixelChunk(TColor* buffer)
  {
    if (buffer == nullptr)
    {
      return span<TColor>{};
    }

    return span<TColor>{buffer, 1u};
  }

  std::unique_ptr<TColor> _rootBuffer;
  std::unique_ptr<transports::ILightDriver<TColor>> _driver;
  std::unique_ptr<IShader<TColor>> _shader;
  std::unique_ptr<TColor> _shaderBuffer;
  PixelView<TColor> _pixels;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses