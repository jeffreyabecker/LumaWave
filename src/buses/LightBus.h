#pragma once

#include <array>
#include <limits>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "colors/IShader.h"
#include "colors/NilShader.h"
#include "core/IPixelBus.h"
#include "transports/ILightDriver.h"
#include "transports/Transports.h"

namespace lw::busses
{

template <typename TColor = lw::colors::DefaultColorType, typename TDriver = transports::PlatformDefaultLightDriver<TColor>, typename TShader = NilShader<TColor>> class LightBus : public IPixelBus<TColor>
{
public:
  using ColorType = TColor;
  using BrightnessType = typename IPixelBus<ColorType>::BrightnessType;
  using DriverType = TDriver;
  using DriverSettingsType = typename DriverType::LightDriverSettingsType;
  using ShaderType = TShader;

  static constexpr bool UsesShaderScratch = !std::is_same<lw::remove_cvref_t<ShaderType>, NilShader<ColorType>>::value;

  static_assert(transports::SettingsConstructibleLightDriverLike<DriverType>, "Driver type must derive from ILightDriver<ColorType>, declare LightDriverSettingsType, and be "
                                                                              "constructible from those settings.");
  static_assert(std::is_same<typename DriverType::ColorType, ColorType>::value, "Driver ColorType must match LightBus ColorType.");
  static_assert(std::is_convertible<ShaderType*, IShader<ColorType>*>::value, "Shader type must derive from IShader<ColorType>.");

  LightBus(DriverSettingsType driverSettings, ShaderType shader) : _driver(normalizeDriverSettings(std::move(driverSettings))), _shader(std::move(shader)), _pixels(span<ColorType>{_rootPixel.data(), _rootPixel.size()}) {}

  template <typename TShaderAlias = ShaderType, typename = std::enable_if_t<std::is_same<lw::remove_cvref_t<TShaderAlias>, NilShader<ColorType>>::value>>
  explicit LightBus(DriverSettingsType driverSettings) : _driver(normalizeDriverSettings(std::move(driverSettings))), _shader{}, _pixels(span<ColorType>{_rootPixel.data(), _rootPixel.size()})
  {
  }

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

    const ColorType* outputPixel = _rootPixel.data();
    if constexpr (UsesShaderScratch)
    {
      _shaderScratch[0] = _rootPixel[0];
      span<ColorType> shaderPixel{_shaderScratch.data(), _shaderScratch.size()};
      _shader.apply(shaderPixel);
      outputPixel = _shaderScratch.data();
    }

    _driver.write(*outputPixel);
    _dirty = false;
  }

  bool isReadyToUpdate() const override { return _driver.isReadyToUpdate(); }

  PixelView<ColorType>& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const PixelView<ColorType>& pixels() const override { return _pixels; }

  ColorType& pixel()
  {
    _dirty = true;
    return _rootPixel[0];
  }

  const ColorType& pixel() const { return _rootPixel[0]; }

  DriverType& driver() { return _driver; }

  const DriverType& driver() const { return _driver; }

  ShaderType& shader() { return _shader; }

  const ShaderType& shader() const { return _shader; }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

  span<ColorType> shaderScratch()
  {
    if constexpr (UsesShaderScratch)
    {
      return span<ColorType>{_shaderScratch.data(), _shaderScratch.size()};
    }

    return span<ColorType>{};
  }

  span<const ColorType> shaderScratch() const
  {
    if constexpr (UsesShaderScratch)
    {
      return span<const ColorType>{_shaderScratch.data(), _shaderScratch.size()};
    }

    return span<const ColorType>{};
  }

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
  ShaderType _shader;
  std::array<ColorType, 1> _rootPixel{};
  std::array<ColorType, 1> _shaderScratch{};
  PixelView<ColorType> _pixels;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses
