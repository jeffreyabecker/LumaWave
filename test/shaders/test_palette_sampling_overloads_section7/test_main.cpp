#include <unity.h>

#include <array>

#include "core/IndexIterator.h"
#include "colors/palette/Sampling.h"

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

constexpr std::array<Stop, 2> kStops = {Stop{0, lw::Rgb8Color(0, 0, 0)}, Stop{255, lw::Rgb8Color(255, 255, 255)}};

lw::span<const Stop> makeStopsSpan()
{
    return lw::span<const Stop>(kStops.data(), kStops.size());
}

lw::colors::palettes::Palette<lw::Rgb8Color> makePalette()
{
    return lw::colors::palettes::Palette<lw::Rgb8Color>(makeStopsSpan());
}

void test_overload_stops_index_iter_output_span(void)
{
    std::array<lw::Rgb8Color, 3> out{};
    lw::IndexRange paletteIndexes(0, 128, out.size());

    const size_t written = lw::colors::palettes::samplePalette(makePalette(), paletteIndexes,
                                                               lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(255, out[2]['R']);
}

void test_overload_stops_index_iter_output_ptr_range(void)
{
    std::array<lw::Rgb8Color, 2> out{};
    lw::IndexRange paletteIndexes(10, 200, out.size());

    const size_t written = lw::colors::palettes::samplePalette(makePalette(), paletteIndexes,
                                                               lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(9, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(209, out[1]['R']);
}

void test_overload_stops_index_span_output_span(void)
{
    const std::array<uint8_t, 3> indices = {0, 64, 255};
    std::array<lw::Rgb8Color, 3> out{};

    const size_t written =
        lw::colors::palettes::samplePalette(makePalette(), lw::span<const uint8_t>(indices.data(), indices.size()),
                                            lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(63, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(254, out[2]['R']);
}

void test_overload_stops_first_step_output_span(void)
{
    std::array<lw::Rgb8Color, 3> out{};
    lw::IndexRange paletteIndexes(0, 64, out.size());

    const size_t written = lw::colors::palettes::samplePalette(makePalette(), paletteIndexes,
                                                               lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(3, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(63, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, out[2]['R']);
}

void test_overload_stops_first_contiguous_output_span(void)
{
    std::array<lw::Rgb8Color, 2> out{};
    lw::IndexRange paletteIndexes(5, 1, out.size());

    const size_t written = lw::colors::palettes::samplePalette(makePalette(), paletteIndexes,
                                                               lw::span<lw::Rgb8Color>(out.data(), out.size()));

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(4, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(5, out[1]['R']);
}

void test_overload_scalar_sample(void)
{
    std::array<lw::Rgb8Color, 1> sampled{};
    lw::IndexRange paletteIndexes(128, 1, 1);
    lw::colors::palettes::samplePalette(makePalette(), paletteIndexes, sampled);
    TEST_ASSERT_EQUAL_UINT8(127, sampled[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, sampled[0]['G']);
    TEST_ASSERT_EQUAL_UINT8(127, sampled[0]['B']);
}

void test_overload_palette_like_and_options(void)
{
    const PaletteLikeRgb8 paletteLike(makeStopsSpan());
    std::array<lw::Rgb8Color, 2> out{};
    lw::IndexRange paletteIndexes(100, 50, out.size());

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options;
    options.brightnessScale = 128;

    const size_t written = lw::colors::palettes::samplePalette(
        paletteLike, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()), options);

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(49, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(74, out[1]['R']);
}

void test_overload_explicit_blend_mode_option(void)
{
    const std::array<uint8_t, 2> indices = {127, 128};
    std::array<lw::Rgb8Color, 2> out{};
    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options;
    options.blendMode = lw::colors::palettes::BlendMode::Nearest;

    const size_t written =
        lw::colors::palettes::samplePalette(makePalette(), lw::span<const uint8_t>(indices.data(), indices.size()),
                                            lw::span<lw::Rgb8Color>(out.data(), out.size()), options);

    TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(written));
    TEST_ASSERT_EQUAL_UINT8(0, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(255, out[1]['R']);
}

void test_overload_palette_like_scalar_sample(void)
{
    const PaletteLikeRgb8 paletteLike(makeStopsSpan());

    lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options;
    options.brightnessScale = 128;

    std::array<lw::Rgb8Color, 1> sampled{};
    lw::IndexRange paletteIndexes(128, 1, 1);

    lw::colors::palettes::samplePalette(paletteLike, paletteIndexes, sampled, options);

    TEST_ASSERT_EQUAL_UINT8(63, sampled[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(63, sampled[0]['G']);
    TEST_ASSERT_EQUAL_UINT8(63, sampled[0]['B']);
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
    RUN_TEST(test_overload_stops_index_iter_output_span);
    RUN_TEST(test_overload_stops_index_iter_output_ptr_range);
    RUN_TEST(test_overload_stops_index_span_output_span);
    RUN_TEST(test_overload_stops_first_step_output_span);
    RUN_TEST(test_overload_stops_first_contiguous_output_span);
    RUN_TEST(test_overload_scalar_sample);
    RUN_TEST(test_overload_palette_like_and_options);
    RUN_TEST(test_overload_explicit_blend_mode_option);
    RUN_TEST(test_overload_palette_like_scalar_sample);
    return UNITY_END();
}
