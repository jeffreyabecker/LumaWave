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
