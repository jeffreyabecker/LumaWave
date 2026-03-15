#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "ColorMath.h"

namespace lw::colors
{

template <typename TComponent> struct KelvinToRgbExactStrategy
{
    static constexpr uint16_t MinKelvin = 1200;
    static constexpr uint16_t MaxKelvin = 65000;

    static constexpr uint16_t clampKelvin(uint16_t kelvin) { return std::clamp(kelvin, MinKelvin, MaxKelvin); }

    static std::array<TComponent, 3> convert(uint16_t kelvin)
    {
        const float temp = static_cast<float>(clampKelvin(kelvin)) / 100.0f;

        int32_t red = 0;
        int32_t green = 0;
        int32_t blue = 0;

        if (temp <= 66.0f)
        {
            red = 255;
            green = static_cast<int32_t>(roundf(99.4708025861f * logf(temp) - 161.1195681661f));

            if (temp <= 19.0f)
            {
                blue = 0;
            }
            else
            {
                blue = static_cast<int32_t>(roundf(138.5177312231f * logf(temp - 10.0f) - 305.0447927307f));
            }
        }
        else
        {
            red = static_cast<int32_t>(roundf(329.698727446f * powf(temp - 60.0f, -0.1332047592f)));
            green = static_cast<int32_t>(roundf(288.1221695283f * powf(temp - 60.0f, -0.0755148492f)));
            blue = 255;
        }

        return {scaleComponent<TComponent>(static_cast<uint8_t>(std::clamp(red, int32_t{0}, int32_t{255}))),
                scaleComponent<TComponent>(static_cast<uint8_t>(std::clamp(green, int32_t{0}, int32_t{255}))),
                scaleComponent<TComponent>(static_cast<uint8_t>(std::clamp(blue, int32_t{0}, int32_t{255})))};
    }
};

// Approximate Kelvin to RGB conversion using a 64-entry lookup table with linear interpolation.
// This strategy is designed for use in shaders where performance is critical and some approximation is acceptable.
// The math is valid for a range of approximately 2000K to 6800K @ < 0.75% error, which covers most common white light sources. Values outside this range will be
// clamped and converted using the exact strategy.
template <typename TComponent> struct KelvinToRgbLut64Strategy
{
    static constexpr uint16_t TableMinKelvin = 2000;
    static constexpr uint16_t TableMaxKelvin = 6800;
    static constexpr uint16_t TableRangeKelvin = TableMaxKelvin - TableMinKelvin;
    static constexpr size_t TablePointCount = 64;
    static constexpr size_t TableSegmentCount = TablePointCount - 1;

    static std::array<TComponent, 3> convert(uint16_t kelvin)
    {
        const uint16_t clampedKelvin = KelvinToRgbExactStrategy<TComponent>::clampKelvin(kelvin);

        if (clampedKelvin < TableMinKelvin || clampedKelvin > TableMaxKelvin)
        {
            return KelvinToRgbExactStrategy<TComponent>::convert(clampedKelvin);
        }

        const uint32_t offsetKelvin = static_cast<uint32_t>(clampedKelvin - TableMinKelvin);
        const uint32_t scaledOffset = offsetKelvin * static_cast<uint32_t>(TableSegmentCount);
        const size_t segmentIndex = static_cast<size_t>(scaledOffset / TableRangeKelvin);
        const uint32_t segmentFraction = scaledOffset % TableRangeKelvin;

        if (segmentIndex >= TableSegmentCount)
        {
            return scaleEntry(Table[TablePointCount - 1]);
        }

        const auto& left = Table[segmentIndex];
        const auto& right = Table[segmentIndex + 1];
        return {interpolateComponent<TComponent>(left[0], right[0], segmentFraction, TableRangeKelvin),
                interpolateComponent<TComponent>(left[1], right[1], segmentFraction, TableRangeKelvin),
                interpolateComponent<TComponent>(left[2], right[2], segmentFraction, TableRangeKelvin)};
    }

  private:
    using Rgb8 = std::array<uint8_t, 3>;

    static std::array<TComponent, 3> scaleEntry(const Rgb8& entry)
    {
        return {scaleComponent<TComponent>(entry[0]), scaleComponent<TComponent>(entry[1]),
                scaleComponent<TComponent>(entry[2])};
    }

    static constexpr std::array<Rgb8, TablePointCount> makeTable()
    {
        return {{{255, 137, 14},  {255, 141, 24},  {255, 144, 34},  {255, 148, 42},  {255, 151, 51},  {255, 154, 59},
                 {255, 157, 66},  {255, 160, 73},  {255, 163, 80},  {255, 166, 86},  {255, 169, 92},  {255, 172, 98},
                 {255, 174, 104}, {255, 177, 109}, {255, 179, 114}, {255, 182, 119}, {255, 184, 124}, {255, 187, 129},
                 {255, 189, 134}, {255, 191, 138}, {255, 193, 142}, {255, 195, 146}, {255, 197, 150}, {255, 199, 154},
                 {255, 201, 158}, {255, 203, 162}, {255, 205, 165}, {255, 207, 169}, {255, 209, 172}, {255, 211, 175},
                 {255, 213, 179}, {255, 214, 182}, {255, 216, 185}, {255, 218, 188}, {255, 220, 191}, {255, 221, 194},
                 {255, 223, 197}, {255, 224, 200}, {255, 226, 202}, {255, 227, 205}, {255, 229, 208}, {255, 230, 210},
                 {255, 232, 213}, {255, 233, 215}, {255, 235, 218}, {255, 236, 220}, {255, 238, 222}, {255, 239, 225},
                 {255, 240, 227}, {255, 242, 229}, {255, 243, 231}, {255, 244, 234}, {255, 246, 236}, {255, 247, 238},
                 {255, 248, 240}, {255, 249, 242}, {255, 250, 244}, {255, 252, 246}, {255, 253, 248}, {255, 254, 250},
                 {255, 255, 252}, {255, 250, 255}, {253, 248, 255}, {250, 246, 255}}};
    }

    static constexpr std::array<Rgb8, TablePointCount> Table = makeTable();
};

} // namespace lw::colors

namespace lw
{

template <typename TComponent> using KelvinToRgbExactStrategy = colors::KelvinToRgbExactStrategy<TComponent>;

template <typename TComponent> using KelvinToRgbLut64Strategy = colors::KelvinToRgbLut64Strategy<TComponent>;

} // namespace lw
