#pragma once

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

#include "colors/ColorMath.h"
#include "colors/palette/Types.h"

#if LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#elif LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_ESP8266)
#include <osapi.h>
#elif LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_RP2040)
#if defined(__has_include)
#if __has_include(<pico/rand.h>)
#define LW_PALETTEGEN_HAS_PICO_RAND 1
#include <pico/rand.h>
#elif __has_include(<Arduino.h>)
#define LW_PALETTEGEN_HAS_ARDUINO_RANDOM 1
#include <Arduino.h>
#endif
#endif
#endif

namespace lw::colors::palettes
{
namespace detail::palettegen
{
using SettingsEntry = std::pair<const char*, const char*>;

inline constexpr std::array<PaletteSettingDescriptor, 0> NoAllowedSettings{};
inline constexpr std::array<PaletteSettingDescriptor, 4> RainbowAllowedSettings{{
    PaletteSettingDescriptor{"stop_count", PaletteSettingValueType::UnsignedSize},
    PaletteSettingDescriptor{"saturation", PaletteSettingValueType::UnsignedColorComponent},
    PaletteSettingDescriptor{"brightness", PaletteSettingValueType::UnsignedColorComponent},
    PaletteSettingDescriptor{"hue_offset", PaletteSettingValueType::UnsignedColorComponent},
}};
inline constexpr std::array<PaletteSettingDescriptor, 5> TemporalRainbowAllowedSettings{{
    PaletteSettingDescriptor{"stop_count", PaletteSettingValueType::UnsignedSize},
    PaletteSettingDescriptor{"saturation", PaletteSettingValueType::UnsignedColorComponent},
    PaletteSettingDescriptor{"brightness", PaletteSettingValueType::UnsignedColorComponent},
    PaletteSettingDescriptor{"hue_offset", PaletteSettingValueType::UnsignedColorComponent},
    PaletteSettingDescriptor{"step_index", PaletteSettingValueType::UnsignedSize},
}};
inline constexpr std::array<PaletteSettingDescriptor, 3> RandomSmoothAllowedSettings{{
    PaletteSettingDescriptor{"stop_count", PaletteSettingValueType::UnsignedSize},
    PaletteSettingDescriptor{"seed", PaletteSettingValueType::UInt32},
    PaletteSettingDescriptor{"progress_step", PaletteSettingValueType::UInt8},
}};
inline constexpr std::array<PaletteSettingDescriptor, 3> RandomCycleAllowedSettings{{
    PaletteSettingDescriptor{"stop_count", PaletteSettingValueType::UnsignedSize},
    PaletteSettingDescriptor{"seed", PaletteSettingValueType::UInt32},
    PaletteSettingDescriptor{"cycle_step", PaletteSettingValueType::UInt8},
}};

inline uint32_t nextRandom(uint32_t& state)
{
#if LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_ESP32)
    state = esp_random();
#elif LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_ESP8266)
    state = static_cast<uint32_t>(os_random());
#elif LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_RP2040) && defined(LW_PALETTEGEN_HAS_PICO_RAND)
    state = get_rand_32();
#elif LW_USE_PLATFORM_RANDOM && defined(ARDUINO_ARCH_RP2040) && defined(LW_PALETTEGEN_HAS_ARDUINO_RANDOM)
    state = static_cast<uint32_t>(random(static_cast<long>(0x7FFFFFFFL)));
#else
    if (state == 0u)
    {
        state = 0x6D2B79F5u;
    }
    state ^= (state << 13u);
    state ^= (state >> 17u);
    state ^= (state << 5u);
#endif
    return state;
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
void assignDistributedStopIndexes(TStops& stops)
{
    if (stops.empty())
    {
        return;
    }

    if (stops.size() == 1u)
    {
        stops[0].index = 0u;
        return;
    }

    constexpr size_t paletteDomainMaxIndex = static_cast<size_t>(std::numeric_limits<uint8_t>::max());
    const size_t stopSpan = stops.size() - 1u;

    for (size_t i = 0; i < stops.size(); ++i)
    {
        stops[i].index = (i * paletteDomainMaxIndex) / stopSpan;
    }
}

inline size_t normalizeStopCount(size_t stopCount)
{
    return (stopCount < 2u) ? 2u : stopCount;
}

inline const char* skipWhitespace(const char* cursor)
{
    if (cursor == nullptr)
    {
        return nullptr;
    }

    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor)) != 0)
    {
        ++cursor;
    }

    return cursor;
}

inline bool keyEquals(const char* actual, const char* expected)
{
    if (actual == nullptr || expected == nullptr)
    {
        return false;
    }

    while (*actual != '\0' && *expected != '\0')
    {
        if (*actual != *expected)
        {
            return false;
        }

        ++actual;
        ++expected;
    }

    return (*actual == '\0') && (*expected == '\0');
}

template <typename TValue> bool tryParseUnsigned(const char* text, TValue& value)
{
    static_assert(std::is_integral<TValue>::value, "Palette settings parser expects integral types");
    static_assert(std::is_unsigned<TValue>::value, "Palette settings parser expects unsigned types");

    if (text == nullptr)
    {
        return false;
    }

    const char* cursor = skipWhitespace(text);
    if (cursor == nullptr || *cursor == '\0')
    {
        return false;
    }

    TValue parsed = 0;
    bool hasDigit = false;
    constexpr TValue maxValue = std::numeric_limits<TValue>::max();

    while (*cursor >= '0' && *cursor <= '9')
    {
        const TValue digit = static_cast<TValue>(*cursor - '0');
        if (parsed > static_cast<TValue>((maxValue - digit) / 10u))
        {
            return false;
        }

        parsed = static_cast<TValue>((parsed * 10u) + digit);
        hasDigit = true;
        ++cursor;
    }

    cursor = skipWhitespace(cursor);
    if (!hasDigit || cursor == nullptr || *cursor != '\0')
    {
        return false;
    }

    value = parsed;
    return true;
}
} // namespace detail::palettegen

template <typename TColor, RequireColorChannelsInRange<TColor, 3, 5> = 0>
class RainbowPaletteGenerator : public IPalette<TColor>
{
  public:
    using ComponentType = typename TColor::ComponentType;
    using SettingsView = typename IPalette<TColor>::SettingsView;
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::RainbowPaletteGenerator;
    static constexpr size_t DefaultStopCount = 16u;
    inline static constexpr auto AllowedSettings = detail::palettegen::RainbowAllowedSettings;

    RainbowPaletteGenerator()
        : IPalette<TColor>(TypeCode), _stops(detail::palettegen::normalizeStopCount(DefaultStopCount)),
          _saturation(TColor::MaxComponent), _brightness(TColor::MaxComponent), _hueOffset(0)
    {
        detail::palettegen::assignDistributedStopIndexes(_stops);
        rebuild();
    }

    void setStopCount(size_t stopCount)
    {
        _stops.resize(detail::palettegen::normalizeStopCount(stopCount));
        detail::palettegen::assignDistributedStopIndexes(_stops);
        rebuild();
    }

    void setSaturation(ComponentType saturation)
    {
        _saturation = saturation;
        rebuild();
    }

    void setBrightness(ComponentType brightness)
    {
        _brightness = brightness;
        rebuild();
    }

    void setHueOffset(ComponentType hueOffset)
    {
        _hueOffset = hueOffset;
        rebuild();
    }

    void update(uint32_t hueStep = 1) override
    {
        _hueOffset = static_cast<ComponentType>(_hueOffset + static_cast<ComponentType>(hueStep));
        rebuild();
    }

    StopsView stops() const override { return StopsView(_stops.data(), _stops.size()); }

    void updateSettings(SettingsView settings) override
    {
        for (const auto& setting : settings)
        {
            if (detail::palettegen::keyEquals(setting.first, "stop_count"))
            {
                size_t stopCount = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, stopCount))
                {
                    setStopCount(stopCount);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "saturation"))
            {
                ComponentType saturation = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, saturation))
                {
                    setSaturation(saturation);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "brightness"))
            {
                ComponentType brightness = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, brightness))
                {
                    setBrightness(brightness);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "hue_offset"))
            {
                ComponentType hueOffset = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, hueOffset))
                {
                    setHueOffset(hueOffset);
                }
            }
        }
    }

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
            const uint64_t span = static_cast<uint64_t>(TColor::MaxComponent) + 1ull;
            const ComponentType hue = static_cast<ComponentType>(
                _hueOffset + static_cast<ComponentType>((static_cast<uint64_t>(i) * span) / stopCount));
            _stops[i].color = lw::colors::hsbToRgb<TColor>(hue, _saturation, _brightness);
        }
    }

    std::vector<PaletteStop<TColor>> _stops{};
    ComponentType _saturation{TColor::MaxComponent};
    ComponentType _brightness{TColor::MaxComponent};
    ComponentType _hueOffset{0};
};

template <typename TColor, RequireColorChannelsInRange<TColor, 3, 5> = 0>
class TemporalRainbowPaletteGenerator : public IPalette<TColor>
{
  public:
    using SettingsView = typename IPalette<TColor>::SettingsView;
    using ComponentType = typename TColor::ComponentType;
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::TemporalRainbowPaletteGenerator;
    inline static constexpr auto AllowedSettings = detail::palettegen::TemporalRainbowAllowedSettings;

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

    void updateSettings(SettingsView settings) override
    {
        bool requiresRebuild = false;

        for (const auto& setting : settings)
        {
            if (detail::palettegen::keyEquals(setting.first, "stop_count"))
            {
                size_t stopCount = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, stopCount))
                {
                    _rainbowGenerator.setStopCount(stopCount);
                    requiresRebuild = true;
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "saturation"))
            {
                ComponentType saturation = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, saturation))
                {
                    _rainbowGenerator.setSaturation(saturation);
                    requiresRebuild = true;
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "brightness"))
            {
                ComponentType brightness = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, brightness))
                {
                    _rainbowGenerator.setBrightness(brightness);
                    requiresRebuild = true;
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "hue_offset"))
            {
                ComponentType hueOffset = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, hueOffset))
                {
                    _rainbowGenerator.setHueOffset(hueOffset);
                    requiresRebuild = true;
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "step_index"))
            {
                size_t stepIndex = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, stepIndex))
                {
                    _stepIndex = stepIndex;
                    requiresRebuild = true;
                }
            }
        }

        if (requiresRebuild)
        {
            rebuild();
        }
    }

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
    using SettingsView = typename IPalette<TColor>::SettingsView;
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::RandomSmoothPaletteGenerator;
    static constexpr size_t DefaultStopCount = 8u;
    static constexpr uint32_t DefaultSeed = 0xC0FFEE11u;
    static constexpr uint8_t DefaultProgressStep = 12u;
    inline static constexpr auto AllowedSettings = detail::palettegen::RandomSmoothAllowedSettings;

    RandomSmoothPaletteGenerator()
        : IPalette<TColor>(TypeCode), _stops(detail::palettegen::normalizeStopCount(DefaultStopCount)),
          _sourceColors(detail::palettegen::normalizeStopCount(DefaultStopCount)),
          _targetColors(detail::palettegen::normalizeStopCount(DefaultStopCount)), _seed(DefaultSeed),
          _rngState(DefaultSeed), _progressStep(DefaultProgressStep)
    {
        resetGeneratedColors();
    }

    void setStopCount(size_t stopCount)
    {
        const size_t normalizedStopCount = detail::palettegen::normalizeStopCount(stopCount);
        _stops.resize(normalizedStopCount);
        _sourceColors.resize(normalizedStopCount);
        _targetColors.resize(normalizedStopCount);
        resetGeneratedColors();
    }

    void setSeed(uint32_t seed)
    {
        _seed = seed;
        resetGeneratedColors();
    }

    void setProgressStep(uint8_t progressStep) { _progressStep = progressStep; }

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

    void updateSettings(SettingsView settings) override
    {
        for (const auto& setting : settings)
        {
            if (detail::palettegen::keyEquals(setting.first, "stop_count"))
            {
                size_t stopCount = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, stopCount))
                {
                    setStopCount(stopCount);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "seed"))
            {
                uint32_t seed = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, seed))
                {
                    setSeed(seed);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "progress_step"))
            {
                uint8_t progressStep = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, progressStep))
                {
                    setProgressStep(progressStep);
                }
            }
        }
    }

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
        target->_seed = _seed;
        target->_rngState = _rngState;
        target->_progress = _progress;
        target->_progressStep = _progressStep;
    }

  private:
    void resetGeneratedColors()
    {
        _progress = 0u;
        detail::palettegen::assignDistributedStopIndexes(_stops);

        uint32_t rngState = _seed;
        for (size_t i = 0; i < _stops.size(); ++i)
        {
            _sourceColors[i] = detail::palettegen::randomColor<TColor>(rngState);
            _targetColors[i] = detail::palettegen::randomColor<TColor>(rngState);
        }

        _rngState = rngState;
        rebuild();
    }

    void rebuild()
    {
        for (size_t i = 0; i < _stops.size(); ++i)
        {
            _stops[i].color = lw::colors::linearBlendProgress8(_sourceColors[i], _targetColors[i], _progress);
        }
    }

    std::vector<PaletteStop<TColor>> _stops{};
    std::vector<TColor> _sourceColors{};
    std::vector<TColor> _targetColors{};
    uint32_t _seed{DefaultSeed};
    uint32_t _rngState{0xC0FFEE11u};
    uint8_t _progress{0};
    uint8_t _progressStep{12};
};

template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>>
class RandomCyclePaletteGenerator : public IPalette<TColor>
{
  public:
    using SettingsView = typename IPalette<TColor>::SettingsView;
    using StopsView = span<const PaletteStop<TColor>>;
    static constexpr uint32_t TypeCode = detail::PaletteTypeCodes::RandomCyclePaletteGenerator;
    static constexpr size_t DefaultStopCount = 8u;
    static constexpr uint32_t DefaultSeed = 0x13579BDFu;
    static constexpr uint8_t DefaultCycleStep = 8u;
    inline static constexpr auto AllowedSettings = detail::palettegen::RandomCycleAllowedSettings;

    RandomCyclePaletteGenerator()
        : IPalette<TColor>(TypeCode), _stops(detail::palettegen::normalizeStopCount(DefaultStopCount)),
          _colors(detail::palettegen::normalizeStopCount(DefaultStopCount)), _seed(DefaultSeed), _rngState(DefaultSeed),
          _cycleStep(DefaultCycleStep)
    {
        resetGeneratedColors();
    }

    void setStopCount(size_t stopCount)
    {
        const size_t normalizedStopCount = detail::palettegen::normalizeStopCount(stopCount);
        _stops.resize(normalizedStopCount);
        _colors.resize(normalizedStopCount);
        resetGeneratedColors();
    }

    void setSeed(uint32_t seed)
    {
        _seed = seed;
        resetGeneratedColors();
    }

    void setCycleStep(uint8_t cycleStep) { _cycleStep = cycleStep; }

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

    void updateSettings(SettingsView settings) override
    {
        for (const auto& setting : settings)
        {
            if (detail::palettegen::keyEquals(setting.first, "stop_count"))
            {
                size_t stopCount = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, stopCount))
                {
                    setStopCount(stopCount);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "seed"))
            {
                uint32_t seed = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, seed))
                {
                    setSeed(seed);
                }
                continue;
            }

            if (detail::palettegen::keyEquals(setting.first, "cycle_step"))
            {
                uint8_t cycleStep = 0;
                if (detail::palettegen::tryParseUnsigned(setting.second, cycleStep))
                {
                    setCycleStep(cycleStep);
                }
            }
        }
    }

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
        target->_seed = _seed;
        target->_rngState = _rngState;
        target->_phase = _phase;
        target->_cycleStep = _cycleStep;
    }

  private:
    void resetGeneratedColors()
    {
        _phase = 0u;
        detail::palettegen::assignDistributedStopIndexes(_stops);

        uint32_t rngState = _seed;
        for (size_t i = 0; i < _stops.size(); ++i)
        {
            _colors[i] = detail::palettegen::randomColor<TColor>(rngState);
        }

        _rngState = rngState;
        rebuild();
    }

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
            _stops[i].color = lw::colors::linearBlendProgress8(_colors[i], _colors[next], _phase);
        }
    }

    std::vector<PaletteStop<TColor>> _stops{};
    std::vector<TColor> _colors{};
    uint32_t _seed{DefaultSeed};
    uint32_t _rngState{0x13579BDFu};
    uint8_t _phase{0};
    uint8_t _cycleStep{8};
};

} // namespace lw::colors::palettes
