#include <unity.h>

#include <array>
#include <vector>

#include "colors/palette/Palette.h"

namespace
{
using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;

struct PaletteLikeRgb8 : lw::colors::palettes::IPalette<lw::Rgb8Color>
{
    explicit PaletteLikeRgb8(lw::span<const Stop> value) : _stops(value) {}

    lw::span<const Stop> stops() const override { return _stops; }

    void update(uint32_t = 0) override {}

  private:
    lw::span<const Stop> _stops;
};

constexpr std::array<Stop, 2> kGradientA = {Stop{0, lw::Rgb8Color(0, 0, 0)}, Stop{255, lw::Rgb8Color(255, 255, 255)}};

constexpr std::array<Stop, 2> kGradientB = {Stop{0, lw::Rgb8Color(255, 0, 0)}, Stop{255, lw::Rgb8Color(255, 0, 0)}};

lw::span<const Stop> gradientASpan()
{
    return lw::span<const Stop>(kGradientA.data(), kGradientA.size());
}

lw::span<const Stop> gradientBSpan()
{
    return lw::span<const Stop>(kGradientB.data(), kGradientB.size());
}

void test_helper_map_transition_progress_to_blend(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, lw::colors::palettes::mapTransitionProgressToBlend8(0, 100));
    TEST_ASSERT_EQUAL_UINT8(127, lw::colors::palettes::mapTransitionProgressToBlend8(50, 100));
    TEST_ASSERT_EQUAL_UINT8(255, lw::colors::palettes::mapTransitionProgressToBlend8(100, 100));
    TEST_ASSERT_EQUAL_UINT8(255, lw::colors::palettes::mapTransitionProgressToBlend8(120, 100));
    TEST_ASSERT_EQUAL_UINT8(255, lw::colors::palettes::mapTransitionProgressToBlend8(1, 0));
}

void test_sample_palette_single_range_overload(void)
{
    std::array<lw::Rgb8Color, 3> out{};
    lw::IndexRange indexes(static_cast<size_t>(0), static_cast<size_t>(128), out.size());
    const PaletteLikeRgb8 paletteLike(gradientASpan());

    const size_t written =
        lw::colors::palettes::samplePalette(paletteLike, indexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(255, out[2]['R']);
}

void test_sample_palette_step_default_palette_index(void)
{
    std::array<lw::Rgb8Color, 2> out{};
    const PaletteLikeRgb8 paletteLike(gradientASpan());
    const size_t written = lw::colors::palettes::samplePalette(paletteLike, static_cast<size_t>(254),
                                                               lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(253, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(254, out[1]['R']);
}

void test_sample_palette_transition_blend_progress8_overload(void)
{
    const PaletteLikeRgb8 from(gradientASpan());
    const PaletteLikeRgb8 to(gradientBSpan());

    std::array<lw::Rgb8Color, 1> out{};
    lw::IndexRange indexes(static_cast<size_t>(128), static_cast<size_t>(1), out.size());

    const size_t written = lw::colors::palettes::samplePalette(
        from, to, indexes, lw::span<lw::Rgb8Color>(out.data(), out.size()), static_cast<uint8_t>(128));

    TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(191, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(63, out[0]['G']);
    TEST_ASSERT_EQUAL_UINT8(63, out[0]['B']);
}

void test_sample_palette_transition_duration_overload(void)
{
    const PaletteLikeRgb8 from(gradientASpan());
    const PaletteLikeRgb8 to(gradientBSpan());

    std::array<lw::Rgb8Color, 1> out{};
    lw::IndexRange indexes(static_cast<size_t>(128), static_cast<size_t>(1), out.size());

    const size_t written =
        lw::colors::palettes::samplePalette(from, to, indexes, lw::span<lw::Rgb8Color>(out.data(), out.size()),
                                            static_cast<uint8_t>(50), static_cast<uint8_t>(100));

    TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(190, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(64, out[0]['G']);
    TEST_ASSERT_EQUAL_UINT8(64, out[0]['B']);
}

void test_sample_palette_transition_step_default_convenience(void)
{
    const PaletteLikeRgb8 from(gradientASpan());
    const PaletteLikeRgb8 to(gradientBSpan());
    std::array<lw::Rgb8Color, 2> out{};

    const size_t written = lw::colors::palettes::samplePalette(
        from, to, static_cast<size_t>(127), lw::span<lw::Rgb8Color>(out.data(), out.size()), static_cast<uint8_t>(128));

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(190, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(63, out[0]['G']);
    TEST_ASSERT_EQUAL_UINT8(63, out[0]['B']);
    TEST_ASSERT_EQUAL_UINT8(191, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(63, out[1]['G']);
    TEST_ASSERT_EQUAL_UINT8(63, out[1]['B']);
}

void test_owned_palette_copies_vector_input(void)
{
    std::vector<Stop> stops = {
        Stop{0, lw::Rgb8Color(0, 10, 20)},
        Stop{255, lw::Rgb8Color(255, 100, 50)},
    };
    lw::colors::palettes::Palette<lw::Rgb8Color> palette(stops);

    stops[0].color = lw::Rgb8Color(99, 99, 99);

    const auto ownedStops = palette.stops();
    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(ownedStops.size()));
    TEST_ASSERT_EQUAL_UINT8(0, ownedStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT8(10, ownedStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT8(20, ownedStops[0].color['B']);
}

void test_owned_palette_mutable_storage_updates_sampling(void)
{
    lw::colors::palettes::Palette<lw::Rgb8Color> palette({
        Stop{0, lw::Rgb8Color(0, 0, 0)},
        Stop{255, lw::Rgb8Color(255, 255, 255)},
    });
    palette.storage()[1].color = lw::Rgb8Color(255, 0, 0);

    std::array<lw::Rgb8Color, 1> out{};
    lw::IndexRange indexes(static_cast<size_t>(255), static_cast<size_t>(1), out.size());

    const size_t written =
        lw::colors::palettes::samplePalette(palette, indexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(254, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['G']);
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['B']);
}

void test_owned_palette_parse_reads_stops(void)
{
    const auto palette = lw::colors::palettes::Palette<lw::Rgb8Color>::parse("0,FF0000|128,00FF00|255,0000FF");

    const auto stops = palette.stops();
    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(stops.size()));
    TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(stops[0].index));
    TEST_ASSERT_EQUAL_UINT32(128, static_cast<uint32_t>(stops[1].index));
    TEST_ASSERT_EQUAL_UINT32(255, static_cast<uint32_t>(stops[2].index));
    TEST_ASSERT_EQUAL_UINT8(255, stops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT8(255, stops[1].color['G']);
    TEST_ASSERT_EQUAL_UINT8(255, stops[2].color['B']);
}

void test_owned_palette_parse_allows_whitespace(void)
{
    const auto palette = lw::colors::palettes::Palette<lw::Rgb8Color>::parse(" 0,112233 | 64,AABBCC | 255,00FF10 ");

    const auto stops = palette.stops();
    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(stops.size()));
    TEST_ASSERT_EQUAL_UINT8(0x11, stops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT8(0xBB, stops[1].color['G']);
    TEST_ASSERT_EQUAL_UINT8(0x10, stops[2].color['B']);
}

void test_owned_palette_parse_supports_rgbw_and_rgbcw_8bit(void)
{
    const auto rgbwPalette = lw::colors::palettes::Palette<lw::Rgbw8Color>::parse("0,11223344|255,AABBCCDD");
    const auto rgbcwPalette = lw::colors::palettes::Palette<lw::Rgbcw8Color>::parse("0,0102030405|255,A0B0C0D0E0");

    const auto rgbwStops = rgbwPalette.stops();
    const auto rgbcwStops = rgbcwPalette.stops();

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbwStops.size()));
    TEST_ASSERT_EQUAL_UINT8(0x11, rgbwStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT8(0x22, rgbwStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT8(0x33, rgbwStops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT8(0x44, rgbwStops[0].color['W']);
    TEST_ASSERT_EQUAL_UINT8(0xDD, rgbwStops[1].color['W']);

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbcwStops.size()));
    TEST_ASSERT_EQUAL_UINT8(0x01, rgbcwStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT8(0x02, rgbcwStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT8(0x03, rgbcwStops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT8(0x04, rgbcwStops[0].color['C']);
    TEST_ASSERT_EQUAL_UINT8(0x05, rgbcwStops[0].color['W']);
    TEST_ASSERT_EQUAL_UINT8(0xE0, rgbcwStops[1].color['W']);
}

void test_owned_palette_parse_upscales_channel_count_at_same_bit_depth(void)
{
    const auto rgbwPalette = lw::colors::palettes::Palette<lw::Rgbw8Color>::parse("0,112233|255,AABBCC");
    const auto rgbcwPalette =
        lw::colors::palettes::Palette<lw::Rgbcw16Color>::parse("0,0001000200030004|255,A001B002C003D004");

    const auto rgbwStops = rgbwPalette.stops();
    const auto rgbcwStops = rgbcwPalette.stops();

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbwStops.size()));
    TEST_ASSERT_EQUAL_UINT8(0x11, rgbwStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT8(0x22, rgbwStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT8(0x33, rgbwStops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT8(0x00, rgbwStops[0].color['W']);

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbcwStops.size()));
    TEST_ASSERT_EQUAL_UINT16(0x0001, rgbcwStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT16(0x0002, rgbcwStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT16(0x0003, rgbcwStops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT16(0x0004, rgbcwStops[0].color['W']);
    TEST_ASSERT_EQUAL_UINT16(0x0000, rgbcwStops[0].color['C']);
}

void test_owned_palette_parse_supports_rgbw_and_rgbcw_16bit(void)
{
    const auto rgbwPalette =
        lw::colors::palettes::Palette<lw::Rgbw16Color>::parse("0,1111222233334444|255,AAAABBBBCCCCDDDD");
    const auto rgbcwPalette =
        lw::colors::palettes::Palette<lw::Rgbcw16Color>::parse("0,00010002000300040005|255,A001B002C003D004E005");

    const auto rgbwStops = rgbwPalette.stops();
    const auto rgbcwStops = rgbcwPalette.stops();

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbwStops.size()));
    TEST_ASSERT_EQUAL_UINT16(0x1111, rgbwStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT16(0x2222, rgbwStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT16(0x3333, rgbwStops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT16(0x4444, rgbwStops[0].color['W']);
    TEST_ASSERT_EQUAL_UINT16(0xDDDD, rgbwStops[1].color['W']);

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbcwStops.size()));
    TEST_ASSERT_EQUAL_UINT16(0x0001, rgbcwStops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT16(0x0002, rgbcwStops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT16(0x0003, rgbcwStops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT16(0x0004, rgbcwStops[0].color['C']);
    TEST_ASSERT_EQUAL_UINT16(0x0005, rgbcwStops[0].color['W']);
    TEST_ASSERT_EQUAL_UINT16(0xE005, rgbcwStops[1].color['W']);
}

void test_owned_palette_parse_upscales_bit_depth_and_channel_count_together(void)
{
    const auto palette = lw::colors::palettes::Palette<lw::Rgbw16Color>::parse("0,112233|255,AABBCC");
    const auto rgbcwPalette = lw::colors::palettes::Palette<lw::Rgbcw16Color>::parse("0,11223344|255,AABBCCDD");

    const auto stops = palette.stops();
    const auto rgbcwStops = rgbcwPalette.stops();

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(stops.size()));
    TEST_ASSERT_EQUAL_UINT16(0x1111, stops[0].color['R']);
    TEST_ASSERT_EQUAL_UINT16(0x2222, stops[0].color['G']);
    TEST_ASSERT_EQUAL_UINT16(0x3333, stops[0].color['B']);
    TEST_ASSERT_EQUAL_UINT16(0x0000, stops[0].color['W']);

    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(rgbcwStops.size()));
    TEST_ASSERT_EQUAL_UINT16(0xAAAA, rgbcwStops[1].color['R']);
    TEST_ASSERT_EQUAL_UINT16(0xBBBB, rgbcwStops[1].color['G']);
    TEST_ASSERT_EQUAL_UINT16(0xCCCC, rgbcwStops[1].color['B']);
    TEST_ASSERT_EQUAL_UINT16(0xDDDD, rgbcwStops[1].color['W']);
    TEST_ASSERT_EQUAL_UINT16(0x0000, rgbcwStops[1].color['C']);
}

void test_owned_palette_parse_rejects_invalid_input(void)
{
    const auto empty = lw::colors::palettes::Palette<lw::Rgb8Color>::parse(nullptr);
    const auto malformed = lw::colors::palettes::Palette<lw::Rgb8Color>::parse("0,FF0000|bad");
    const auto badHex = lw::colors::palettes::Palette<lw::Rgb8Color>::parse("0,FF00ZZ");
    const auto downscaleChannels = lw::colors::palettes::Palette<lw::Rgb8Color>::parse("0,11223344");
    const auto downscaleBitDepth =
        lw::colors::palettes::Palette<lw::Rgbw8Color>::parse("0,1111222233334444|255,AAAABBBBCCCCDDDD");
    const auto downscaleBoth = lw::colors::palettes::Palette<lw::Rgb16Color>::parse("0,0102030405");

    TEST_ASSERT_TRUE(empty.stops().empty());
    TEST_ASSERT_TRUE(malformed.stops().empty());
    TEST_ASSERT_TRUE(badHex.stops().empty());
    TEST_ASSERT_TRUE(downscaleChannels.stops().empty());
    TEST_ASSERT_TRUE(downscaleBitDepth.stops().empty());
    TEST_ASSERT_TRUE(downscaleBoth.stops().empty());
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
    RUN_TEST(test_helper_map_transition_progress_to_blend);
    RUN_TEST(test_sample_palette_single_range_overload);
    RUN_TEST(test_sample_palette_step_default_palette_index);
    RUN_TEST(test_sample_palette_transition_blend_progress8_overload);
    RUN_TEST(test_sample_palette_transition_duration_overload);
    RUN_TEST(test_sample_palette_transition_step_default_convenience);
    RUN_TEST(test_owned_palette_copies_vector_input);
    RUN_TEST(test_owned_palette_mutable_storage_updates_sampling);
    RUN_TEST(test_owned_palette_parse_reads_stops);
    RUN_TEST(test_owned_palette_parse_allows_whitespace);
    RUN_TEST(test_owned_palette_parse_supports_rgbw_and_rgbcw_8bit);
    RUN_TEST(test_owned_palette_parse_upscales_channel_count_at_same_bit_depth);
    RUN_TEST(test_owned_palette_parse_supports_rgbw_and_rgbcw_16bit);
    RUN_TEST(test_owned_palette_parse_upscales_bit_depth_and_channel_count_together);
    RUN_TEST(test_owned_palette_parse_rejects_invalid_input);
    return UNITY_END();
}
