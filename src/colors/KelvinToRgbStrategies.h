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

namespace detail
{
struct KelvinToRgbIntegerMathQ11
{
    static constexpr int32_t QBits = 11;
    static constexpr int32_t QOne = int32_t{1} << QBits;

    static constexpr std::array<int32_t, 5> RedSegment0 = {22615, -110215, 201488, -212120, 527325};
    static constexpr std::array<int32_t, 4> RedSegment1 = {-62015, 152924, -169707, 427650};
    static constexpr std::array<int32_t, 4> RedSegment2 = {-43354, 108498, -125744, 348655};

    static constexpr std::array<int32_t, 4> GreenSegment0 = {14679, -58665, 167435, 176346};
    static constexpr std::array<int32_t, 4> GreenSegment1 = {8050, -36327, 128479, 299884};
    static constexpr std::array<int32_t, 4> GreenSegment2 = {3591, -18931, 90221, 400126};
    static constexpr std::array<int32_t, 4> GreenSegment3 = {-744, -5140, 54163, 475025};
    static constexpr std::array<int32_t, 5> GreenSegment4 = {50921, -148111, 178405, -139609, 514908};
    static constexpr std::array<int32_t, 4> GreenSegment5 = {-35883, 89998, -103715, 455530};
    static constexpr std::array<int32_t, 4> GreenSegment6 = {-27985, 70246, -83924, 405704};

    static constexpr std::array<int32_t, 4> BlueSegment0 = {28922, -109305, 276496, -745};
    static constexpr std::array<int32_t, 4> BlueSegment1 = {17480, -72078, 217663, 195501};
    static constexpr std::array<int32_t, 4> BlueSegment2 = {3645, -19766, 106456, 358491};
    static constexpr std::array<int32_t, 4> BlueSegment3 = {1117, -10091, 77259, 448849};

    static constexpr int32_t clampByte(int32_t value) { return std::clamp(value, int32_t{0}, int32_t{255}); }

    static constexpr int32_t normalizeKelvin(uint16_t kelvin, uint16_t startKelvin, uint16_t endKelvin)
    {
        const int32_t range = static_cast<int32_t>(endKelvin) - static_cast<int32_t>(startKelvin);
        if (range <= 0)
        {
            return 0;
        }

        const int32_t offset = static_cast<int32_t>(kelvin) - static_cast<int32_t>(startKelvin);
        return ((offset * QOne) + (range / 2)) / range;
    }

    template <size_t N> static constexpr int32_t evaluatePolynomial(const std::array<int32_t, N>& coeffs, int32_t xQ)
    {
        int32_t accumulator = coeffs[0];

        for (size_t index = 1; index < N; ++index)
        {
            accumulator = ((accumulator * xQ) + (QOne / 2)) / QOne;
            accumulator += coeffs[index];
        }

        return (accumulator + (QOne / 2)) / QOne;
    }

    static constexpr uint8_t evaluateRed(uint16_t kelvin)
    {
        if (kelvin <= 6600)
        {
            return 255;
        }

        if (kelvin <= 9000)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(RedSegment0, normalizeKelvin(kelvin, 6601, 9000))));
        }

        if (kelvin <= 20000)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(RedSegment1, normalizeKelvin(kelvin, 9001, 20000))));
        }

        return static_cast<uint8_t>(clampByte(evaluatePolynomial(RedSegment2, normalizeKelvin(kelvin, 20001, 65000))));
    }

    static constexpr uint8_t evaluateGreen(uint16_t kelvin)
    {
        if (kelvin <= 2200)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(GreenSegment0, normalizeKelvin(kelvin, 1200, 2200))));
        }

        if (kelvin <= 3600)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(GreenSegment1, normalizeKelvin(kelvin, 2201, 3600))));
        }

        if (kelvin <= 5200)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(GreenSegment2, normalizeKelvin(kelvin, 3601, 5200))));
        }

        if (kelvin <= 6600)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(GreenSegment3, normalizeKelvin(kelvin, 5201, 6600))));
        }

        if (kelvin <= 9000)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(GreenSegment4, normalizeKelvin(kelvin, 6601, 9000))));
        }

        if (kelvin <= 20000)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(GreenSegment5, normalizeKelvin(kelvin, 9001, 20000))));
        }

        return static_cast<uint8_t>(
            clampByte(evaluatePolynomial(GreenSegment6, normalizeKelvin(kelvin, 20001, 65000))));
    }

    static constexpr uint8_t evaluateBlue(uint16_t kelvin)
    {
        if (kelvin <= 1900)
        {
            return 0;
        }

        if (kelvin <= 2800)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(BlueSegment0, normalizeKelvin(kelvin, 1901, 2800))));
        }

        if (kelvin <= 4200)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(BlueSegment1, normalizeKelvin(kelvin, 2801, 4200))));
        }

        if (kelvin <= 5400)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(BlueSegment2, normalizeKelvin(kelvin, 4201, 5400))));
        }

        if (kelvin <= 6600)
        {
            return static_cast<uint8_t>(
                clampByte(evaluatePolynomial(BlueSegment3, normalizeKelvin(kelvin, 5401, 6600))));
        }

        return 255;
    }
};
} // namespace detail

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

template <typename TComponent> struct KelvinToRgbIntegerStrategy
{
    static constexpr uint16_t MinKelvin = KelvinToRgbExactStrategy<TComponent>::MinKelvin;
    static constexpr uint16_t MaxKelvin = KelvinToRgbExactStrategy<TComponent>::MaxKelvin;

    static constexpr uint16_t clampKelvin(uint16_t kelvin)
    {
        return KelvinToRgbExactStrategy<TComponent>::clampKelvin(kelvin);
    }

    static std::array<TComponent, 3> convert(uint16_t kelvin)
    {
        using Math = detail::KelvinToRgbIntegerMathQ11;

        const uint16_t clampedKelvin = clampKelvin(kelvin);
        const uint8_t red = Math::evaluateRed(clampedKelvin);
        const uint8_t green = Math::evaluateGreen(clampedKelvin);
        const uint8_t blue = Math::evaluateBlue(clampedKelvin);

        return {scaleComponent<TComponent>(red), scaleComponent<TComponent>(green), scaleComponent<TComponent>(blue)};
    }
};

// Approximate Kelvin to RGB conversion using a 64-entry stepped lookup table.
// This strategy is designed for use in shaders where performance is critical and some approximation is acceptable.
// Input kelvin values are clamped to the LUT domain before lookup.
template <typename TComponent> struct KelvinToRgbLut64Strategy
{
    static constexpr uint16_t TableMinKelvin = 2000;
    static constexpr uint16_t TableMaxKelvin = 6800;
    static constexpr uint16_t MinKelvin = TableMinKelvin;
    static constexpr uint16_t MaxKelvin = TableMaxKelvin;

    static constexpr uint16_t TableRangeKelvin = TableMaxKelvin - TableMinKelvin;
    static constexpr size_t TablePointCount = 64;
    static constexpr size_t TableSegmentCount = TablePointCount - 1;

    static constexpr uint16_t clampKelvin(uint16_t kelvin) { return std::clamp(kelvin, MinKelvin, MaxKelvin); }

    static std::array<TComponent, 3> convert(uint16_t kelvin)
    {
        const uint16_t clampedKelvin = clampKelvin(kelvin);

        const uint32_t offsetKelvin = static_cast<uint32_t>(clampedKelvin - TableMinKelvin);
        const uint32_t scaledOffset = offsetKelvin * static_cast<uint32_t>(TableSegmentCount);
        const size_t segmentIndex = static_cast<size_t>(scaledOffset / TableRangeKelvin);

        return scaleEntry(Table[segmentIndex]);
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

template <typename TComponent> using KelvinToRgbIntegerStrategy = colors::KelvinToRgbIntegerStrategy<TComponent>;

template <typename TComponent> using KelvinToRgbLut64Strategy = colors::KelvinToRgbLut64Strategy<TComponent>;

} // namespace lw
