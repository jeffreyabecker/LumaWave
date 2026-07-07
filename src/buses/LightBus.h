#pragma once

#include <array>
#include <limits>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "colors/ColorMath.h"
#include "core/IPixelBus.h"
#include "transports/ILightDriver.h"
#include "transports/Transports.h"

namespace lw::busses
{

template <typename TDriver = transports::PlatformDefaultLightDriver> class LightBus : public IPixelBus
{
public:
  using ColorType = lw::colors::Color;
  using BrightnessType = IPixelBus::BrightnessType;
  using DriverType = TDriver;
  using DriverSettingsType = typename DriverType::LightDriverSettingsType;

  static_assert(transports::SettingsConstructibleLightDriverLike<DriverType>, "Driver type must derive from ILightDriver, declare LightDriverSettingsType, and be "
                                                                              "constructible from those settings.");
  static_assert(std::is_same_v<typename DriverType::ColorType, ColorType>, "Driver ColorType must match LightBus ColorType.");

  explicit LightBus(DriverSettingsType driverSettings) : _driver(normalizeDriverSettings(std::move(driverSettings))), _pixels(span<ColorType>{_rootPixel.data(), _rootPixel.size()}) {}

  void begin() override { _driver.begin(); }

  void show() override
  {
    if (!_dirty)
    {
      return;
    }

    if (!_driver.isReadyToUpdate())
    {
      return;
    }

    _driver.write(_rootPixel[0], _brightness);
    _dirty = false;
  }

  bool isReadyToUpdate() const override { return _driver.isReadyToUpdate(); }

  PixelView& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const PixelView& pixels() const override { return _pixels; }

  ColorType& pixel()
  {
    _dirty = true;
    return _rootPixel[0];
  }

  const ColorType& pixel() const { return _rootPixel[0]; }

  DriverType& driver() { return _driver; }

  const DriverType& driver() const { return _driver; }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

private:
  template <typename TSettings, typename = void> struct DriverSettingsHasNormalize : std::false_type
  {
  };

  template <typename TSettings> struct DriverSettingsHasNormalize<TSettings, std::void_t<decltype(TSettings::normalize(std::declval<TSettings>()))>> : std::true_type
  {
  };

  static DriverSettingsType normalizeDriverSettings(DriverSettingsType settings)
  {
    if constexpr (DriverSettingsHasNormalize<DriverSettingsType>::value)
    {
      return DriverSettingsType::normalize(std::move(settings));
    }

    return settings;
  }

  DriverType _driver;
  std::array<ColorType, 1> _rootPixel{};
  PixelView _pixels;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses
