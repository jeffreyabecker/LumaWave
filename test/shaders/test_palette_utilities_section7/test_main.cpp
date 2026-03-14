#include <unity.h>

#include "colors/palette/Palette.h"

namespace
{
using TestColor = lw::Rgb8Color;

lw::span<const lw::colors::palettes::PaletteStop<TestColor>> sampleStops()
{
    static const lw::colors::palettes::PaletteStop<TestColor> stops[] = {
        {0, TestColor{1, 0, 0}},
        {127, TestColor{0, 1, 0}},
        {255, TestColor{0, 0, 1}},
    };

    return lw::span<const lw::colors::palettes::PaletteStop<TestColor>>(stops, 3);
}

void test_7_1_1_map_position_clamp_behaviour(void)
{
    lw::colors::palettes::PaletteSampleOptions<TestColor> options;
    options.wrapMode = lw::colors::palettes::WrapMode::Clamp;
    const TestColor expectedFirst{1, 0, 0};
    const TestColor expectedMiddle{0, 1, 0};
    const TestColor expectedLast{0, 0, 1};

    const TestColor first = lw::colors::palettes::samplePaletteAt(sampleStops(), 0, options);
    const TestColor middle = lw::colors::palettes::samplePaletteAt(sampleStops(), 127, options);
    const TestColor last = lw::colors::palettes::samplePaletteAt(sampleStops(), 255, options);
    const TestColor clamped = lw::colors::palettes::samplePaletteAt(sampleStops(), 999, options);

    TEST_ASSERT_TRUE(first == expectedFirst);
    TEST_ASSERT_TRUE(middle == expectedMiddle);
    TEST_ASSERT_TRUE(last == expectedLast);
    TEST_ASSERT_TRUE(clamped == expectedLast);
}

void test_7_1_2_map_position_wrap_behaviour(void)
{
    lw::colors::palettes::PaletteSampleOptions<TestColor> options;
    options.wrapMode = lw::colors::palettes::WrapMode::Circular;
    const TestColor expectedStart{1, 0, 0};
    const TestColor expectedMiddle{0, 1, 0};
    const TestColor expectedEnd{0, 0, 1};

    const TestColor start = lw::colors::palettes::samplePaletteAt(sampleStops(), 0, options);
    const TestColor wrappedToStart = lw::colors::palettes::samplePaletteAt(sampleStops(), 256, options);
    const TestColor wrappedToMiddle = lw::colors::palettes::samplePaletteAt(sampleStops(), 127 + 256, options);
    const TestColor wrappedNearEnd = lw::colors::palettes::samplePaletteAt(sampleStops(), 255 + 256, options);

    TEST_ASSERT_TRUE(start == expectedStart);
    TEST_ASSERT_TRUE(wrappedToStart == expectedStart);
    TEST_ASSERT_TRUE(wrappedToMiddle == expectedMiddle);
    TEST_ASSERT_TRUE(wrappedNearEnd == expectedEnd);
}
} // namespace

void setUp(void)
{
}

void tearDown(void)
{
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    UNITY_BEGIN();
    RUN_TEST(test_7_1_1_map_position_clamp_behaviour);
    RUN_TEST(test_7_1_2_map_position_wrap_behaviour);
    return UNITY_END();
}
