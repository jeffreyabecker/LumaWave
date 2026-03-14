#include <unity.h>

#include <array>

#include "core/IndexIterator.h"
#include "colors/palette/Palette.h"

namespace
{
using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;
using Palette = lw::colors::palettes::Palette<lw::Rgb8Color>;

template <typename TPalette> void assert_same_stops(const TPalette& a, const TPalette& b)
{
    const auto sa = a.stops();
    const auto sb = b.stops();

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa.size()), static_cast<uint32_t>(sb.size()));
    for (size_t i = 0; i < sa.size(); ++i)
    {
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa[i].index), static_cast<uint32_t>(sb[i].index));
        TEST_ASSERT_TRUE(sa[i].color == sb[i].color);
    }
}

static_assert(std::is_convertible<decltype(std::declval<const Palette&>().stops()), lw::span<const Stop>>::value,
              "Palette stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<Palette>::value, "Palette should satisfy IsPaletteLike");
static_assert(std::is_convertible<
                  decltype(std::declval<const lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>&>().stops()),
                  lw::span<const Stop>>::value,
              "RainbowPaletteGenerator stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>>::value,
              "RainbowPaletteGenerator must satisfy IsPaletteLike");
static_assert(
    std::is_convertible<
        decltype(std::declval<const lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>&>().stops()),
        lw::span<const Stop>>::value,
    "TemporalRainbowPaletteGenerator stops() must return a stop span");
static_assert(
    lw::colors::palettes::IsPaletteLike<lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>>::value,
    "TemporalRainbowPaletteGenerator must satisfy IsPaletteLike");
static_assert(
    std::is_convertible<
        decltype(std::declval<const lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>&>().stops()),
        lw::span<const Stop>>::value,
    "RandomSmoothPaletteGenerator stops() must return a stop span");
static_assert(
    lw::colors::palettes::IsPaletteLike<lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>>::value,
    "RandomSmoothPaletteGenerator must satisfy IsPaletteLike");
static_assert(
    std::is_convertible<
        decltype(std::declval<const lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>&>().stops()),
        lw::span<const Stop>>::value,
    "RandomCyclePaletteGenerator stops() must return a stop span");
static_assert(
    lw::colors::palettes::IsPaletteLike<lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>>::value,
    "RandomCyclePaletteGenerator must satisfy IsPaletteLike");

void test_rainbow_generator_stop_shape_and_update(void)
{
    lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow(8);
    const auto before = rainbow.stops();

    TEST_ASSERT_EQUAL_UINT32(8, static_cast<uint32_t>(before.size()));
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(before[0].index));
    TEST_ASSERT_EQUAL_UINT32(7, static_cast<uint32_t>(before[7].index));

    const lw::Rgb8Color firstBefore = before[0].color;
    rainbow.update(16);
    const auto after = rainbow.stops();

    TEST_ASSERT_TRUE(!(after[0].color == firstBefore));
}

void test_temporal_rainbow_generator_is_two_stop_uniform_palette(void)
{
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> generator;
    const auto stops = generator.stops();

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(stops.size()));
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(stops[0].index));
    TEST_ASSERT_EQUAL_UINT32(255, static_cast<uint32_t>(stops[1].index));
    TEST_ASSERT_TRUE(stops[0].color == stops[1].color);
}

void test_temporal_rainbow_generator_changes_uniform_color_over_time(void)
{
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> generator;
    const auto before = generator.stops();
    const lw::Rgb8Color firstBefore = before[0].color;

    generator.update();

    const auto after = generator.stops();
    TEST_ASSERT_TRUE(after[0].color == after[1].color);
    TEST_ASSERT_TRUE(!(after[0].color == firstBefore));
}

void test_temporal_rainbow_generator_defaults_update_step_to_one(void)
{
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> implicitStep;
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> explicitStep;

    implicitStep.update();
    explicitStep.update(1);

    const auto implicitStops = implicitStep.stops();
    const auto explicitStops = explicitStep.stops();

    TEST_ASSERT_TRUE(implicitStops[0].color == explicitStops[0].color);
    TEST_ASSERT_TRUE(implicitStops[1].color == explicitStops[1].color);
}

void test_random_smooth_generator_is_deterministic(void)
{
    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> a(6, 12345u, 20);
    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> b(6, 12345u, 20);

    for (int i = 0; i < 12; ++i)
    {
        a.update();
        b.update();
    }

    const auto sa = a.stops();
    const auto sb = b.stops();

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa.size()), static_cast<uint32_t>(sb.size()));
    for (size_t i = 0; i < sa.size(); ++i)
    {
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa[i].index), static_cast<uint32_t>(sb[i].index));
        TEST_ASSERT_TRUE(sa[i].color == sb[i].color);
    }
}

void test_random_smooth_generator_changes_over_time(void)
{
    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> generator(6, 999u, 17);
    const auto before = generator.stops();
    const lw::Rgb8Color firstBefore = before[0].color;

    generator.update();
    generator.update();

    const auto after = generator.stops();
    TEST_ASSERT_TRUE(!(after[0].color == firstBefore));
}

void test_random_cycle_generator_is_deterministic(void)
{
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> a(5, 42u, 32);
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> b(5, 42u, 32);

    for (int i = 0; i < 16; ++i)
    {
        a.update();
        b.update();
    }

    const auto sa = a.stops();
    const auto sb = b.stops();

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa.size()), static_cast<uint32_t>(sb.size()));
    for (size_t i = 0; i < sa.size(); ++i)
    {
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa[i].index), static_cast<uint32_t>(sb[i].index));
        TEST_ASSERT_TRUE(sa[i].color == sb[i].color);
    }
}

void test_random_cycle_generator_rotates_and_samples(void)
{
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> generator(5, 77u, 64);
    const auto beforeStopsView = generator.stops();
    std::array<Stop, 5> beforeStops{};
    for (size_t i = 0; i < beforeStops.size(); ++i)
    {
        beforeStops[i] = beforeStopsView[i];
    }
    for (int i = 0; i < 8; ++i)
    {
        generator.update();
    }

    const auto afterStops = generator.stops();

    bool anyStopChanged = false;
    for (size_t i = 0; i < afterStops.size(); ++i)
    {
        if (!(afterStops[i].color == beforeStops[i].color))
        {
            anyStopChanged = true;
            break;
        }
    }

    TEST_ASSERT_TRUE(anyStopChanged);
}

void test_generators_satisfy_palette_like_usage(void)
{
    const std::array<lw::colors::palettes::PaletteStop<lw::Rgb8Color>, 2> solidStops = {
        lw::colors::palettes::PaletteStop<lw::Rgb8Color>{0, lw::Rgb8Color(7, 8, 9)},
        lw::colors::palettes::PaletteStop<lw::Rgb8Color>{255, lw::Rgb8Color(7, 8, 9)},
    };
    const Palette solid(solidStops);
    lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow(6);
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalRainbow;
    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smooth(6, 1u, 25);
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle(6, 2u, 25);

    std::array<lw::Rgb8Color, 4> out{};
    lw::IndexRange paletteIndexes(0, 32, out.size());
    const size_t solidWritten =
        lw::colors::palettes::samplePalette(solid, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
    const size_t rainbowWritten =
        lw::colors::palettes::samplePalette(rainbow, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
    const size_t temporalRainbowWritten = lw::colors::palettes::samplePalette(
        temporalRainbow, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
    const size_t smoothWritten =
        lw::colors::palettes::samplePalette(smooth, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
    const size_t cycleWritten =
        lw::colors::palettes::samplePalette(cycle, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(solidWritten));
    TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(rainbowWritten));
    TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(temporalRainbowWritten));
    TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(smoothWritten));
    TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(cycleWritten));
}

void test_palette_type_codes_are_initialized_and_unique_per_implementation(void)
{
    const Palette solid;
    lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow;
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalRainbow;
    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smooth;
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle;

    TEST_ASSERT_NOT_EQUAL(0u, solid.typeCode());
    TEST_ASSERT_NOT_EQUAL(0u, rainbow.typeCode());
    TEST_ASSERT_NOT_EQUAL(0u, temporalRainbow.typeCode());
    TEST_ASSERT_NOT_EQUAL(0u, smooth.typeCode());
    TEST_ASSERT_NOT_EQUAL(0u, cycle.typeCode());

    TEST_ASSERT_EQUAL_UINT32(Palette::TypeCode, solid.typeCode());
    TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>::TypeCode,
                             rainbow.typeCode());
    TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>::TypeCode,
                             temporalRainbow.typeCode());
    TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>::TypeCode,
                             smooth.typeCode());
    TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>::TypeCode,
                             cycle.typeCode());

    TEST_ASSERT_NOT_EQUAL(solid.typeCode(), rainbow.typeCode());
    TEST_ASSERT_NOT_EQUAL(solid.typeCode(), temporalRainbow.typeCode());
    TEST_ASSERT_NOT_EQUAL(solid.typeCode(), smooth.typeCode());
    TEST_ASSERT_NOT_EQUAL(solid.typeCode(), cycle.typeCode());
    TEST_ASSERT_NOT_EQUAL(rainbow.typeCode(), temporalRainbow.typeCode());
    TEST_ASSERT_NOT_EQUAL(rainbow.typeCode(), smooth.typeCode());
    TEST_ASSERT_NOT_EQUAL(rainbow.typeCode(), cycle.typeCode());
    TEST_ASSERT_NOT_EQUAL(temporalRainbow.typeCode(), smooth.typeCode());
    TEST_ASSERT_NOT_EQUAL(temporalRainbow.typeCode(), cycle.typeCode());
    TEST_ASSERT_NOT_EQUAL(smooth.typeCode(), cycle.typeCode());
}

void test_generator_sync_copies_internal_state_for_same_type(void)
{
    lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbowSource(6, 0.8f, 0.5f, 17);
    lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbowTarget(10, 1.0f, 1.0f, 99);
    rainbowSource.update(13);
    rainbowTarget.update(7);
    rainbowSource.syncTo(&rainbowTarget);
    assert_same_stops(rainbowSource, rainbowTarget);
    rainbowSource.update(5);
    rainbowTarget.update(5);
    assert_same_stops(rainbowSource, rainbowTarget);

    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalSource;
    lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalTarget;
    temporalSource.update(23);
    temporalTarget.update(2);
    temporalSource.syncTo(&temporalTarget);
    assert_same_stops(temporalSource, temporalTarget);
    temporalSource.update(9);
    temporalTarget.update(9);
    assert_same_stops(temporalSource, temporalTarget);

    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smoothSource(6, 111u, 19);
    lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smoothTarget(4, 222u, 5);
    smoothSource.update();
    smoothSource.update();
    smoothTarget.update();
    smoothSource.syncTo(&smoothTarget);
    assert_same_stops(smoothSource, smoothTarget);
    smoothSource.update();
    smoothTarget.update();
    assert_same_stops(smoothSource, smoothTarget);

    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycleSource(5, 333u, 41);
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycleTarget(7, 444u, 9);
    cycleSource.update();
    cycleSource.update();
    cycleTarget.update();
    cycleSource.syncTo(&cycleTarget);
    assert_same_stops(cycleSource, cycleTarget);
    cycleSource.update();
    cycleTarget.update();
    assert_same_stops(cycleSource, cycleTarget);
}

void test_generator_sync_ignores_mismatched_type_and_null_target(void)
{
    lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow(6, 0.9f, 0.7f, 33);
    lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle(6, 555u, 12);

    rainbow.update(4);
    const auto before = cycle.stops();
    std::array<Stop, 6> beforeStops{};
    for (size_t i = 0; i < beforeStops.size(); ++i)
    {
        beforeStops[i] = before[i];
    }

    rainbow.syncTo(nullptr);
    rainbow.syncTo(&cycle);

    const auto after = cycle.stops();
    for (size_t i = 0; i < beforeStops.size(); ++i)
    {
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(beforeStops[i].index), static_cast<uint32_t>(after[i].index));
        TEST_ASSERT_TRUE(beforeStops[i].color == after[i].color);
    }
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
    RUN_TEST(test_rainbow_generator_stop_shape_and_update);
    RUN_TEST(test_temporal_rainbow_generator_is_two_stop_uniform_palette);
    RUN_TEST(test_temporal_rainbow_generator_changes_uniform_color_over_time);
    RUN_TEST(test_temporal_rainbow_generator_defaults_update_step_to_one);
    RUN_TEST(test_random_smooth_generator_is_deterministic);
    RUN_TEST(test_random_smooth_generator_changes_over_time);
    RUN_TEST(test_random_cycle_generator_is_deterministic);
    RUN_TEST(test_random_cycle_generator_rotates_and_samples);
    RUN_TEST(test_generators_satisfy_palette_like_usage);
    RUN_TEST(test_palette_type_codes_are_initialized_and_unique_per_implementation);
    RUN_TEST(test_generator_sync_copies_internal_state_for_same_type);
    RUN_TEST(test_generator_sync_ignores_mismatched_type_and_null_target);
    return UNITY_END();
}
