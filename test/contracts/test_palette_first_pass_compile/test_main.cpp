#include <unity.h>

#include <array>
#include <type_traits>
#include <vector>

#include "core/IndexIterator.h"
#include "colors/palette/Palette.h"

namespace
{
void test_palette_first_pass_compile(void)
{
    static_assert(lw::ColorType<lw::Rgb8Color>, "Rgb8Color must satisfy ColorType");
    static_assert(lw::colors::palettes::IsPaletteLike<lw::colors::palettes::Palette<lw::Rgb8Color>>::value,
                  "Palette<TColor> must satisfy IsPaletteLike");
    static_assert(lw::colors::palettes::Palette<lw::Rgb8Color>::AllowedSettings.size() == 0u,
                  "Palette<TColor> must expose an empty AllowedSettings descriptor");
    static_assert(lw::colors::palettes::blend::Linear == lw::colors::palettes::BlendMode::Linear,
                  "blend::Linear must map to BlendMode::Linear");
    static_assert(lw::colors::palettes::blend::Nearest == lw::colors::palettes::BlendMode::Nearest,
                  "blend::Nearest must map to BlendMode::Nearest");
    static_assert(lw::colors::palettes::blend::Midpoint == lw::colors::palettes::BlendMode::HoldMidpoint,
                  "blend::Midpoint must map to BlendMode::HoldMidpoint");
    static_assert(std::is_enum<lw::colors::palettes::WrapMode>::value, "WrapMode must be an enum");
    static_assert(lw::colors::palettes::WrapMode::Clamp != lw::colors::palettes::WrapMode::Circular,
                  "WrapMode values must remain distinct");

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options;

    lw::colors::palettes::PaletteStop<lw::Rgb8Color> invalidStop{};
    invalidStop.index = 0;
    invalidStop.color = lw::Rgb8Color(1, 2, 3);

    lw::colors::palettes::Palette<lw::Rgb8Color> invalidPalette(
        lw::span<const lw::colors::palettes::PaletteStop<lw::Rgb8Color>>(&invalidStop, 1));
    TEST_ASSERT_TRUE(invalidPalette.stops().empty());

    std::array<lw::colors::palettes::PaletteStop<lw::Rgb8Color>, 2> sampleStops = {
        lw::colors::palettes::PaletteStop<lw::Rgb8Color>{0, lw::Rgb8Color(0, 0, 0)},
        lw::colors::palettes::PaletteStop<lw::Rgb8Color>{255, lw::Rgb8Color(255, 255, 255)}};
    lw::colors::palettes::Palette<lw::Rgb8Color> samplePaletteLike(
        lw::span<const lw::colors::palettes::PaletteStop<lw::Rgb8Color>>(sampleStops.data(), sampleStops.size()));
    lw::colors::palettes::Palette<lw::Rgb8Color> ownedPaletteLike(
        std::vector<lw::colors::palettes::PaletteStop<lw::Rgb8Color>>(sampleStops.begin(), sampleStops.end()));
    std::array<lw::Rgb8Color, 2> sampledOutput{};
    lw::IndexRange sampleIndexes(0, 128, sampledOutput.size());
    const size_t sampledCount = lw::colors::palettes::samplePalette(
        samplePaletteLike, sampleIndexes, lw::span<lw::Rgb8Color>(sampledOutput.data(), sampledOutput.size()), options);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(sampledCount));

    const size_t ownedSampledCount = lw::colors::palettes::samplePalette(
        ownedPaletteLike, sampleIndexes, lw::span<lw::Rgb8Color>(sampledOutput.data(), sampledOutput.size()), options);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(ownedSampledCount));

    lw::IndexRange nearestSampleIndexes(0, 128, sampledOutput.size());
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> nearestOptions = options;
    nearestOptions.blendMode = lw::colors::palettes::BlendMode::Nearest;
    const size_t nearestSampledCount = lw::colors::palettes::samplePalette(
        samplePaletteLike, nearestSampleIndexes, lw::span<lw::Rgb8Color>(sampledOutput.data(), sampledOutput.size()),
        nearestOptions);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(nearestSampledCount));

    lw::IndexRange midpointIndexes(0, 128, sampledOutput.size());
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> midpointOptions = options;
    midpointOptions.blendMode = lw::colors::palettes::BlendMode::HoldMidpoint;
    const size_t midpointSampledCount = lw::colors::palettes::samplePalette(
        samplePaletteLike, midpointIndexes, lw::span<lw::Rgb8Color>(sampledOutput.data(), sampledOutput.size()),
        midpointOptions);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(midpointSampledCount));

    lw::IndexIterator indexIt(10, 5, 3);
    const lw::IndexSentinel indexEnd{};
    TEST_ASSERT_FALSE(indexIt == indexEnd);
    TEST_ASSERT_EQUAL_UINT8(10, *indexIt);
    ++indexIt;
    TEST_ASSERT_EQUAL_UINT8(15, *indexIt);
    ++indexIt;
    TEST_ASSERT_EQUAL_UINT8(20, *indexIt);
    ++indexIt;
    TEST_ASSERT_TRUE(indexIt == indexEnd);
}
} // namespace

void setUp(void)
{
}

void tearDown(void)
{
}

int main(int, char**)
{
    UNITY_BEGIN();
    RUN_TEST(test_palette_first_pass_compile);
    return UNITY_END();
}
