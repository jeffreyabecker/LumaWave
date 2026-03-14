#include <unity.h>

#include <array>

#include "core/IndexIterator.h"
#include "colors/palette/Palette.h"

namespace
{
using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;

constexpr std::array<Stop, 2> kWideStops = {Stop{0, lw::Rgb8Color(0, 0, 0)}, Stop{255, lw::Rgb8Color(255, 255, 255)}};

constexpr std::array<Stop, 5> kRangeStops = {Stop{0, lw::Rgb8Color(10, 0, 0)}, Stop{50, lw::Rgb8Color(10, 0, 0)},
                                             Stop{120, lw::Rgb8Color(100, 0, 0)}, Stop{200, lw::Rgb8Color(200, 0, 0)},
                                             Stop{255, lw::Rgb8Color(200, 0, 0)}};

constexpr std::array<Stop, 3> kTieStops = {Stop{0, lw::Rgb8Color(255, 0, 0)}, Stop{2, lw::Rgb8Color(0, 0, 255)},
                                           Stop{255, lw::Rgb8Color(0, 0, 255)}};

constexpr std::array<Stop, 4> kDuplicateIndexStops = {
    Stop{0, lw::Rgb8Color(0, 0, 0)}, Stop{128, lw::Rgb8Color(255, 0, 0)}, Stop{128, lw::Rgb8Color(0, 0, 255)},
    Stop{255, lw::Rgb8Color(255, 255, 255)}};

lw::span<const Stop> wideStops()
{
    return lw::span<const Stop>(kWideStops.data(), kWideStops.size());
}

lw::colors::palettes::Palette<lw::Rgb8Color> widePalette()
{
    return lw::colors::palettes::Palette<lw::Rgb8Color>(wideStops());
}

lw::span<const Stop> rangeStops()
{
    return lw::span<const Stop>(kRangeStops.data(), kRangeStops.size());
}

lw::colors::palettes::Palette<lw::Rgb8Color> rangePalette()
{
    return lw::colors::palettes::Palette<lw::Rgb8Color>(rangeStops());
}

lw::span<const Stop> tieStops()
{
    return lw::span<const Stop>(kTieStops.data(), kTieStops.size());
}

lw::colors::palettes::Palette<lw::Rgb8Color> tiePalette()
{
    return lw::colors::palettes::Palette<lw::Rgb8Color>(tieStops());
}

lw::span<const Stop> duplicateIndexStops()
{
    return lw::span<const Stop>(kDuplicateIndexStops.data(), kDuplicateIndexStops.size());
}

lw::colors::palettes::Palette<lw::Rgb8Color> duplicateIndexPalette()
{
    return lw::colors::palettes::Palette<lw::Rgb8Color>(duplicateIndexStops());
}

template <typename TPaletteLike>
lw::Rgb8Color sampleScalar(const TPaletteLike& palette, size_t paletteIndex,
                           lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options = {})
{
    std::array<lw::Rgb8Color, 1> sampled{};
    lw::IndexRange paletteIndexes(paletteIndex, 1, 1);
    lw::colors::palettes::samplePalette(palette, paletteIndexes, sampled, options);
    return sampled[0];
}

void test_blend_step_and_hold_midpoint(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> stepOptions;
    stepOptions.blendMode = lw::colors::palettes::BlendMode::Step;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> midpointOptions;
    midpointOptions.blendMode = lw::colors::palettes::BlendMode::HoldMidpoint;

    const lw::Rgb8Color stepped = sampleScalar(widePalette(), 120, stepOptions);
    const lw::Rgb8Color heldLow = sampleScalar(widePalette(), 127, midpointOptions);
    const lw::Rgb8Color heldHigh = sampleScalar(widePalette(), 128, midpointOptions);

    TEST_ASSERT_EQUAL_UINT8(0, stepped['R']);
    TEST_ASSERT_EQUAL_UINT8(0, heldLow['R']);
    TEST_ASSERT_EQUAL_UINT8(255, heldHigh['R']);
}

void test_blend_smooth_cubic_cosine_family(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> smoothOptions;
    smoothOptions.blendMode = lw::colors::palettes::BlendMode::Smoothstep;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> cubicOptions;
    cubicOptions.blendMode = lw::colors::palettes::BlendMode::Cubic;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> cosineOptions;
    cosineOptions.blendMode = lw::colors::palettes::BlendMode::Cosine;

    const lw::Rgb8Color linear = sampleScalar(widePalette(), 64);
    const lw::Rgb8Color smooth = sampleScalar(widePalette(), 64, smoothOptions);
    const lw::Rgb8Color cubic = sampleScalar(widePalette(), 64, cubicOptions);
    const lw::Rgb8Color cosine = sampleScalar(widePalette(), 64, cosineOptions);

    TEST_ASSERT_EQUAL_UINT8(63, linear['R']);
    TEST_ASSERT_TRUE(smooth['R'] < linear['R']);
    TEST_ASSERT_TRUE(cubic['R'] < smooth['R']);
    TEST_ASSERT_TRUE(cosine['R'] <= smooth['R']);
}

void test_blend_gamma_quantized_dithered(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> gammaOptions;
    gammaOptions.blendMode = lw::colors::palettes::BlendMode::GammaLinear;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> quantizedOptions;
    quantizedOptions.blendMode = lw::colors::palettes::BlendMode::Quantized;
    quantizedOptions.quantizedLevels = 4;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> ditherOptions;
    ditherOptions.blendMode = lw::colors::palettes::BlendMode::DitheredLinear;

    const lw::Rgb8Color gamma = sampleScalar(widePalette(), 128, gammaOptions);
    const lw::Rgb8Color quant4 = sampleScalar(widePalette(), 64, quantizedOptions);
    const lw::Rgb8Color dither = sampleScalar(widePalette(), 63, ditherOptions);

    TEST_ASSERT_TRUE(gamma['R'] > 127);
    TEST_ASSERT_EQUAL_UINT8(85, quant4['R']);
    TEST_ASSERT_TRUE(dither['R'] >= 62);
}

void test_nearest_tie_break_modes(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> stableOptions;
    stableOptions.blendMode = lw::colors::palettes::BlendMode::Nearest;
    stableOptions.tieBreakPolicy = lw::colors::palettes::TieBreakPolicy::Stable;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> leftOptions;
    leftOptions.blendMode = lw::colors::palettes::BlendMode::Nearest;
    leftOptions.tieBreakPolicy = lw::colors::palettes::TieBreakPolicy::Left;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> rightOptions;
    rightOptions.blendMode = lw::colors::palettes::BlendMode::Nearest;
    rightOptions.tieBreakPolicy = lw::colors::palettes::TieBreakPolicy::Right;

    const lw::Rgb8Color stable = sampleScalar(tiePalette(), 1, stableOptions);
    const lw::Rgb8Color left = sampleScalar(tiePalette(), 1, leftOptions);
    const lw::Rgb8Color right = sampleScalar(tiePalette(), 1, rightOptions);

    TEST_ASSERT_EQUAL_UINT8(255, stable['R']);
    TEST_ASSERT_EQUAL_UINT8(255, left['R']);
    TEST_ASSERT_EQUAL_UINT8(255, right['B']);
}

void test_ordered_duplicate_indexes_form_hard_transition(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options;
    options.blendMode = lw::colors::palettes::BlendMode::HoldMidpoint;

    const lw::Rgb8Color atBoundary = sampleScalar(duplicateIndexPalette(), 128, options);
    const lw::Rgb8Color afterBoundary = sampleScalar(duplicateIndexPalette(), 129, options);

    TEST_ASSERT_EQUAL_UINT8(255, atBoundary['R']);
    TEST_ASSERT_EQUAL_UINT8(0, atBoundary['B']);
    TEST_ASSERT_EQUAL_UINT8(0, afterBoundary['R']);
    TEST_ASSERT_EQUAL_UINT8(255, afterBoundary['B']);
}

void test_wrap_mode_index_mapping(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> clampOptions;
    clampOptions.scaledSampleCount = 10;

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> mirrorOptions;
    mirrorOptions.wrapMode = lw::colors::palettes::WrapMode::Mirror;
    mirrorOptions.scaledSampleCount = 5;

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> holdFirstOptions;
    holdFirstOptions.wrapMode = lw::colors::palettes::WrapMode::HoldFirst;
    holdFirstOptions.scaledSampleCount = 5;

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> holdLastOptions;
    holdLastOptions.wrapMode = lw::colors::palettes::WrapMode::HoldLast;
    holdLastOptions.scaledSampleCount = 5;

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> circularOptions;
    circularOptions.wrapMode = lw::colors::palettes::WrapMode::Circular;
    circularOptions.scaledSampleCount = 4;

    const auto clamped = lw::colors::palettes::detail::normalizeForDomain(clampOptions, 100u);
    const auto mirrored = lw::colors::palettes::detail::normalizeForDomain(mirrorOptions, 7u);
    const auto heldFirst = lw::colors::palettes::detail::normalizeForDomain(holdFirstOptions, 7u);
    const auto heldLast = lw::colors::palettes::detail::normalizeForDomain(holdLastOptions, 7u);
    const auto circular = lw::colors::palettes::detail::normalizeForDomain(circularOptions, 1u);

    TEST_ASSERT_FALSE(clamped.outOfRange);
    TEST_ASSERT_FALSE(mirrored.outOfRange);
    TEST_ASSERT_FALSE(heldFirst.outOfRange);
    TEST_ASSERT_FALSE(heldLast.outOfRange);
    TEST_ASSERT_FALSE(circular.outOfRange);

    TEST_ASSERT_EQUAL_UINT8(9, clamped.value);
    TEST_ASSERT_EQUAL_UINT8(1, mirrored.value);
    TEST_ASSERT_EQUAL_UINT8(0, heldFirst.value);
    TEST_ASSERT_EQUAL_UINT8(4, heldLast.value);
    TEST_ASSERT_EQUAL_UINT8(1, circular.value);
}

void test_wrap_blackout_out_of_range_sampling(void)
{
    std::array<lw::Rgb8Color, 3> out{};
    const std::array<size_t, 3> paletteIndexes = {50u, 255u, 452u};
    const lw::colors::palettes::Palette<lw::Rgb8Color> palette(rangeStops());
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options;
    options.wrapMode = lw::colors::palettes::WrapMode::Blackout;
    const size_t written = lw::colors::palettes::samplePalette(
        palette, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()), options);

    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(10, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(200, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(0, out[2]['R']);
}

void test_wrap_hold_first_last_with_linear_sampling(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> holdFirstOptions;
    holdFirstOptions.wrapMode = lw::colors::palettes::WrapMode::HoldFirst;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> holdLastOptions;
    holdLastOptions.wrapMode = lw::colors::palettes::WrapMode::HoldLast;

    const lw::Rgb8Color firstHeld = sampleScalar(rangePalette(), 300, holdFirstOptions);
    const lw::Rgb8Color lastHeld = sampleScalar(rangePalette(), 300, holdLastOptions);

    TEST_ASSERT_EQUAL_UINT8(10, firstHeld['R']);
    TEST_ASSERT_EQUAL_UINT8(200, lastHeld['R']);
}

void test_wrap_hold_first_last_with_nearest_sampling(void)
{
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> holdFirstOptions;
    holdFirstOptions.wrapMode = lw::colors::palettes::WrapMode::HoldFirst;
    holdFirstOptions.blendMode = lw::colors::palettes::BlendMode::Nearest;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> holdLastOptions;
    holdLastOptions.wrapMode = lw::colors::palettes::WrapMode::HoldLast;
    holdLastOptions.blendMode = lw::colors::palettes::BlendMode::Nearest;

    const lw::Rgb8Color firstHeld = sampleScalar(rangePalette(), 300, holdFirstOptions);
    const lw::Rgb8Color lastHeld = sampleScalar(rangePalette(), 300, holdLastOptions);

    TEST_ASSERT_EQUAL_UINT8(10, firstHeld['R']);
    TEST_ASSERT_EQUAL_UINT8(200, lastHeld['R']);
}

void test_blend_mode_cost_smoke(void)
{
    std::array<lw::Rgb8Color, 64> out{};
    lw::IndexRange paletteIndexes(0, 4, out.size());

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> smoothOptions;
    smoothOptions.blendMode = lw::colors::palettes::BlendMode::Smoothstep;
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> gammaOptions;
    gammaOptions.blendMode = lw::colors::palettes::BlendMode::GammaLinear;

    const size_t linearCount = lw::colors::palettes::samplePalette(widePalette(), paletteIndexes,
                                                                   lw::span<lw::Rgb8Color>(out.data(), out.size()));
    const size_t smoothCount = lw::colors::palettes::samplePalette(
        widePalette(), paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()), smoothOptions);
    const size_t gammaCount = lw::colors::palettes::samplePalette(
        widePalette(), paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()), gammaOptions);

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(out.size()), static_cast<uint32_t>(linearCount));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(out.size()), static_cast<uint32_t>(smoothCount));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(out.size()), static_cast<uint32_t>(gammaCount));
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
    RUN_TEST(test_blend_step_and_hold_midpoint);
    RUN_TEST(test_blend_smooth_cubic_cosine_family);
    RUN_TEST(test_blend_gamma_quantized_dithered);
    RUN_TEST(test_nearest_tie_break_modes);
    RUN_TEST(test_ordered_duplicate_indexes_form_hard_transition);
    RUN_TEST(test_wrap_mode_index_mapping);
    RUN_TEST(test_wrap_blackout_out_of_range_sampling);
    RUN_TEST(test_wrap_hold_first_last_with_linear_sampling);
    RUN_TEST(test_wrap_hold_first_last_with_nearest_sampling);
    RUN_TEST(test_blend_mode_cost_smoke);
    return UNITY_END();
}
