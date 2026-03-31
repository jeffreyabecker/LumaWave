#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "IShader.h"

namespace lw::shaders
{

template <typename TColor> struct AggregateShaderSettings
{
    std::vector<std::unique_ptr<IShader<TColor>>> shaders;
};

template <typename TColor> class AggregateShader : public IShader<TColor>
{
  public:
    using ColorType = TColor;
    using SettingsType = AggregateShaderSettings<TColor>;
    using BrightnessType = typename IShader<TColor>::BrightnessType;

    AggregateShader() = default;

    explicit AggregateShader(SettingsType settings) : _shaders(std::move(settings.shaders)) {}

    void apply(span<TColor> colors) override
    {
        for (auto& shader : _shaders)
        {
            if (shader != nullptr)
            {
                shader->apply(colors);
            }
        }
    }

      BrightnessOwnership brightnessOwnership() const override
      {
        size_t ownerCount = 0;

        for (const auto& shader : _shaders)
        {
          if (shader == nullptr)
          {
            continue;
          }

          const BrightnessOwnership ownership = shader->brightnessOwnership();
          if (ownership == BrightnessOwnership::Conflict)
          {
            return BrightnessOwnership::Conflict;
          }

          if (ownership == BrightnessOwnership::Owns)
          {
            ++ownerCount;
            if (ownerCount > 1)
            {
              return BrightnessOwnership::Conflict;
            }
          }
        }

        return (ownerCount == 1U) ? BrightnessOwnership::Owns : BrightnessOwnership::None;
      }

      void applyBrightness(span<TColor> colors, BrightnessType brightness) override
      {
        if (brightnessOwnership() != BrightnessOwnership::Owns)
        {
          return;
        }

        for (auto& shader : _shaders)
        {
          if ((shader != nullptr) && (shader->brightnessOwnership() == BrightnessOwnership::Owns))
          {
            shader->applyBrightness(colors, brightness);
            return;
          }
        }
      }

    void addShader(std::unique_ptr<IShader<TColor>> shader) { _shaders.emplace_back(std::move(shader)); }

    std::unique_ptr<IShader<TColor>> removeShader(size_t index)
    {
        if (index >= _shaders.size())
        {
            return nullptr;
        }

        auto it = _shaders.begin() + static_cast<std::ptrdiff_t>(index);
        std::unique_ptr<IShader<TColor>> removed = std::move(*it);
        _shaders.erase(it);
        return removed;
    }

    size_t shaderCount() const { return _shaders.size(); }

  private:
    std::vector<std::unique_ptr<IShader<TColor>>> _shaders;
};

#if !LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
template <typename TColor, typename... TShaders> class CompositeShader : public IShader<TColor>
{
  public:
    using ColorType = TColor;
    static_assert(sizeof...(TShaders) > 0, "CompositeShader requires at least one shader");
    static_assert(std::conjunction<std::is_base_of<IShader<TColor>, TShaders>...>::value,
                  "All TShaders must derive from IShader<TColor>");

    explicit CompositeShader(TShaders... shaders) : _aggregate(makeAggregateSettings(std::move(shaders)...)) {}

    void apply(span<TColor> colors) override { _aggregate.apply(colors); }

    BrightnessOwnership brightnessOwnership() const override { return _aggregate.brightnessOwnership(); }

    void applyBrightness(span<TColor> colors, typename IShader<TColor>::BrightnessType brightness) override
    {
      _aggregate.applyBrightness(colors, brightness);
    }

  private:
    static typename AggregateShader<TColor>::SettingsType makeAggregateSettings(TShaders... shaders)
    {
        typename AggregateShader<TColor>::SettingsType settings{};
        settings.shaders.reserve(sizeof...(TShaders));

        (settings.shaders.emplace_back(std::make_unique<TShaders>(std::move(shaders))), ...);

        return settings;
    }

    AggregateShader<TColor> _aggregate;
};

#endif

} // namespace lw::shaders

namespace lw
{

template <typename TColor> using AggregateShaderSettings = shaders::AggregateShaderSettings<TColor>;

template <typename TColor> using AggregateShader = shaders::AggregateShader<TColor>;

#if !LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
template <typename TColor, typename... TShaders>
using OwningAggregateShaderT = shaders::CompositeShader<TColor, TShaders...>;
#endif

} // namespace lw
