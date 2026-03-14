#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

#include "colors/ColorMath.h"
#include "colors/HsbColor.h"
#include "colors/palette/RandomBackend.h"
#include "colors/palette/Types.h"

namespace lw::colors::palettes
{
namespace detail::palettegen
{
struct RandomBackendSelector
{
    using Type = LW_PALETTE_RANDOM_BACKEND;
};

inline uint32_t nextRandom(uint32_t& state)
{
    using Backend = typename RandomBackendSelector::Type;
    return Backend::next(state);
}

template <typename TComponent> TComponent randomComponent(uint32_t& state)
{
    const uint32_t value = nextRandom(state);
    return static_cast<TComponent>(value & static_cast<uint32_t>(std::numeric_limits<TComponent>::max()));
}

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> TColor randomColor(uint32_t& state)
{
    TColor color{};
    for (char channel : TColor::channelIndexes())
    {
        color[channel] = randomComponent<typename TColor::ComponentType>(state);
    }

    return color;
}

template <typename TStops, typename = std::enable_if_t<ColorType<typename TStops::value_type::ColorType>>>
void assignEvenStopIndexes(TStops& stops)
{
    for (size_t i = 0; i < stops.size(); ++i)
    {
        stops[i].index = i;
    }
}

inline size_t normalizeStopCount(size_t stopCount)
{
    return (stopCount < 2u) ? 2u : stopCount;
}
} // namespace detail::palettegen

template <typename TColor, RequireColorChannelsInRange<TColor, 3, 5> = 0>
class RainbowPaletteGenerator : public IPalette<TColor>
{
  public:
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::RainbowPaletteGenerator;

    explicit RainbowPaletteGenerator(size_t stopCount = 16, float saturation = 1.0f, float brightness = 1.0f,
                                     uint8_t hueOffset = 0)
        : IPalette<TColor>(TypeCode), _stops(detail::palettegen::normalizeStopCount(stopCount)),
          _saturation(saturation), _brightness(brightness), _hueOffset(hueOffset)
    {
        detail::palettegen::assignEvenStopIndexes(_stops);
        rebuild();
    }

    void setSaturation(float saturation)
    {
        _saturation = saturation;
        rebuild();
    }

    void setBrightness(float brightness)
    {
        _brightness = brightness;
        rebuild();
    }

    void setHueOffset(uint8_t hueOffset)
    {
        _hueOffset = hueOffset;
        rebuild();
    }

    void update(uint32_t hueStep = 1) override
    {
        _hueOffset = static_cast<uint8_t>(_hueOffset + hueStep);
        rebuild();
    }

    StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

    void syncTo(IPalette<TColor>* that) override
    {
        RainbowPaletteGenerator<TColor>* target = detail::syncTarget<RainbowPaletteGenerator<TColor>, TColor>(that);
        if (target == nullptr)
        {
            return;
        }

        target->_stops = _stops;
        target->_saturation = _saturation;
        target->_brightness = _brightness;
        target->_hueOffset = _hueOffset;
    }

  private:
    void rebuild()
    {
        const size_t stopCount = _stops.size();
        for (size_t i = 0; i < stopCount; ++i)
        {
            const uint8_t hue = static_cast<uint8_t>(_hueOffset + static_cast<uint8_t>((i * 256ull) / stopCount));
            const float hue01 = static_cast<float>(hue) / 255.0f;
            _stops[i].color = toRgb<TColor>(HsbColor(hue01, _saturation, _brightness));
        }
    }

    std::vector<PaletteStop<TColor>> _stops{};
    float _saturation{1.0f};
    float _brightness{1.0f};
    uint8_t _hueOffset{0};
};

template <typename TColor, RequireColorChannelsInRange<TColor, 3, 5> = 0>
class TemporalRainbowPaletteGenerator : public IPalette<TColor>
{
  public:
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::TemporalRainbowPaletteGenerator;

    explicit TemporalRainbowPaletteGenerator() : IPalette<TColor>(TypeCode), _rainbowGenerator()
    {
        _stops[0].index = 0;
        _stops[1].index = 255;
        rebuild();
    }

    void update(uint32_t step = 1) override
    {
        _stepIndex += step;
        rebuild();
    }

    StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

    void syncTo(IPalette<TColor>* that) override
    {
        TemporalRainbowPaletteGenerator<TColor>* target =
            detail::syncTarget<TemporalRainbowPaletteGenerator<TColor>, TColor>(that);
        if (target == nullptr)
        {
            return;
        }

        target->_stops = _stops;
        target->_rainbowGenerator = _rainbowGenerator;
        target->_stepIndex = _stepIndex;
    }

  private:
    void rebuild()
    {
        const TColor liveColor = samplePaletteAt<TColor>(_rainbowGenerator.stops(), _stepIndex,
                                                         PaletteSampleOptions<TColor>{.wrapMode = WrapMode::Circular,
                                                                                      .blendMode = BlendMode::Linear,
                                                                                      .quantizedLevels = 0});
        _stops[0].color = liveColor;
        _stops[1].color = liveColor;
    }

    std::array<PaletteStop<TColor>, 2> _stops{};
    RainbowPaletteGenerator<TColor> _rainbowGenerator;
    size_t _stepIndex = 0;
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
class RandomSmoothPaletteGenerator : public IPalette<TColor>
{
  public:
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::RandomSmoothPaletteGenerator;

    explicit RandomSmoothPaletteGenerator(size_t stopCount = 8, uint32_t seed = 0xC0FFEE11u, uint8_t progressStep = 12)
        : IPalette<TColor>(TypeCode), _stops(detail::palettegen::normalizeStopCount(stopCount)),
          _sourceColors(detail::palettegen::normalizeStopCount(stopCount)),
          _targetColors(detail::palettegen::normalizeStopCount(stopCount)), _rngState(seed), _progressStep(progressStep)
    {
        detail::palettegen::assignEvenStopIndexes(_stops);

        for (size_t i = 0; i < _stops.size(); ++i)
        {
            _sourceColors[i] = detail::palettegen::randomColor<TColor>(_rngState);
            _targetColors[i] = detail::palettegen::randomColor<TColor>(_rngState);
        }

        rebuild();
    }

    void setSeed(uint32_t seed) { _rngState = seed; }

    void update(uint32_t progressStep = 0) override
    {
        const uint32_t step = (progressStep == 0) ? static_cast<uint32_t>(_progressStep) : progressStep;
        const uint32_t nextProgress = static_cast<uint32_t>(_progress) + step;
        const uint32_t cycleCount = nextProgress / 255u;

        for (uint32_t cycle = 0; cycle < cycleCount; ++cycle)
        {
            for (size_t i = 0; i < _stops.size(); ++i)
            {
                _sourceColors[i] = _targetColors[i];
                _targetColors[i] = detail::palettegen::randomColor<TColor>(_rngState);
            }
        }

        _progress = static_cast<uint8_t>(nextProgress % 255u);
        rebuild();
    }

    StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

    void syncTo(IPalette<TColor>* that) override
    {
        RandomSmoothPaletteGenerator<TColor>* target =
            detail::syncTarget<RandomSmoothPaletteGenerator<TColor>, TColor>(that);
        if (target == nullptr)
        {
            return;
        }

        target->_stops = _stops;
        target->_sourceColors = _sourceColors;
        target->_targetColors = _targetColors;
        target->_rngState = _rngState;
        target->_progress = _progress;
        target->_progressStep = _progressStep;
    }

  private:
    void rebuild()
    {
        for (size_t i = 0; i < _stops.size(); ++i)
        {
            _stops[i].color = lw::linearBlendProgress8(_sourceColors[i], _targetColors[i], _progress);
        }
    }

    std::vector<PaletteStop<TColor>> _stops{};
    std::vector<TColor> _sourceColors{};
    std::vector<TColor> _targetColors{};
    uint32_t _rngState{0xC0FFEE11u};
    uint8_t _progress{0};
    uint8_t _progressStep{12};
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
class RandomCyclePaletteGenerator : public IPalette<TColor>
{
  public:
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::RandomCyclePaletteGenerator;

    explicit RandomCyclePaletteGenerator(size_t stopCount = 8, uint32_t seed = 0x13579BDFu, uint8_t cycleStep = 8)
        : IPalette<TColor>(TypeCode), _stops(detail::palettegen::normalizeStopCount(stopCount)),
          _colors(detail::palettegen::normalizeStopCount(stopCount)), _rngState(seed), _cycleStep(cycleStep)
    {
        detail::palettegen::assignEvenStopIndexes(_stops);
        for (size_t i = 0; i < _stops.size(); ++i)
        {
            _colors[i] = detail::palettegen::randomColor<TColor>(_rngState);
        }

        rebuild();
    }

    void setSeed(uint32_t seed) { _rngState = seed; }

    void update(uint32_t cycleStep = 0) override
    {
        const uint32_t step = (cycleStep == 0) ? static_cast<uint32_t>(_cycleStep) : cycleStep;
        const uint32_t nextPhase = static_cast<uint32_t>(_phase) + step;
        const uint32_t cycleCount = nextPhase / 255u;

        for (uint32_t cycle = 0; cycle < cycleCount; ++cycle)
        {
            rotateCycle();
        }

        _phase = static_cast<uint8_t>(nextPhase % 255u);
        rebuild();
    }

    StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

    void syncTo(IPalette<TColor>* that) override
    {
        RandomCyclePaletteGenerator<TColor>* target =
            detail::syncTarget<RandomCyclePaletteGenerator<TColor>, TColor>(that);
        if (target == nullptr)
        {
            return;
        }

        target->_stops = _stops;
        target->_colors = _colors;
        target->_rngState = _rngState;
        target->_phase = _phase;
        target->_cycleStep = _cycleStep;
    }

  private:
    void rotateCycle()
    {
        for (size_t i = 0; i + 1 < _colors.size(); ++i)
        {
            _colors[i] = _colors[i + 1];
        }

        _colors.back() = detail::palettegen::randomColor<TColor>(_rngState);
    }

    void rebuild()
    {
        const size_t stopCount = _stops.size();
        for (size_t i = 0; i < stopCount; ++i)
        {
            const size_t next = (i + 1u) % stopCount;
            _stops[i].color = lw::linearBlendProgress8(_colors[i], _colors[next], _phase);
        }
    }

    std::vector<PaletteStop<TColor>> _stops{};
    std::vector<TColor> _colors{};
    uint32_t _rngState{0x13579BDFu};
    uint8_t _phase{0};
    uint8_t _cycleStep{8};
};

} // namespace lw::colors::palettes
