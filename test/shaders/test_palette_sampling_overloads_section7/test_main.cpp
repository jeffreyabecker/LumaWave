#include <unity.h>

#include <algorithm>
#include <array>

#include "core/IndexIterator.h"
#include "colors/palette/Sampling.h"

namespace
{
using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;

struct PaletteLikeRgb8 : lw::colors::palettes::IPalette<lw::Rgb8Color>
{
    static constexpr uint32_t TypeCode = 0x504C5431u;

    explicit PaletteLikeRgb8(lw::span<const Stop> value) : IPalette(TypeCode), _stops(value) {}

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

void test_scaled_palette_sampler_stretches_indexes_using_integer_mapping(void)
{
    const auto palette = makePalette();
    const lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> options{};
    using BaseSampler = lw::colors::palettes::SinglePaletteSampler<lw::Rgb8Color>;
    const lw::colors::palettes::ScaledPaletteSampler<lw::Rgb8Color, BaseSampler> sampler(BaseSampler(palette, options),
                                                                                         256u, 1024u);

    const lw::Rgb8Color first = sampler(0u);
    const lw::Rgb8Color middleA = sampler(511u);
    const lw::Rgb8Color middleB = sampler(512u);
    const lw::Rgb8Color last = sampler(1023u);

    TEST_ASSERT_TRUE(first == lw::colors::palettes::samplePaletteAt<lw::Rgb8Color>(palette.stops(), 0u, options));
    TEST_ASSERT_TRUE(middleA == lw::colors::palettes::samplePaletteAt<lw::Rgb8Color>(palette.stops(), 127u, options));
    TEST_ASSERT_TRUE(middleB == lw::colors::palettes::samplePaletteAt<lw::Rgb8Color>(palette.stops(), 127u, options));
    TEST_ASSERT_TRUE(last == lw::colors::palettes::samplePaletteAt<lw::Rgb8Color>(palette.stops(), 255u, options));
}

void test_scaled_palette_sampler_reuses_cached_color_for_repeated_mapped_indexes(void)
{
    struct CountingPalette : lw::colors::palettes::IPalette<lw::Rgb8Color>
    {
        explicit CountingPalette(lw::span<const Stop> value) : IPalette(0x504C5432u), _stops(value) {}

        lw::span<const Stop> stops() const override
        {
            ++stopsCallCount;
            return _stops;
        }

        void update(uint32_t = 0) override {}

        lw::span<const Stop> _stops;
        mutable uint32_t stopsCallCount{0};
    };

    const CountingPalette palette(makeStopsSpan());
    using BaseSampler = lw::colors::palettes::SinglePaletteSampler<lw::Rgb8Color>;
    const lw::colors::palettes::ScaledPaletteSampler<lw::Rgb8Color, BaseSampler> sampler(BaseSampler(palette, {}), 2u,
                                                                                         4u);

    const lw::Rgb8Color first = sampler(0u);
    const lw::Rgb8Color second = sampler(1u);
    const lw::Rgb8Color third = sampler(2u);
    const lw::Rgb8Color fourth = sampler(3u);

    TEST_ASSERT_TRUE(first == second);
    TEST_ASSERT_TRUE(second == third);
    TEST_ASSERT_TRUE(!(third == fourth));
    TEST_ASSERT_EQUAL_UINT32(2u, palette.stopsCallCount);
}

void test_scaled_palette_sampler_wraps_transition_sampler(void)
{
    const auto from = lw::colors::palettes::Palette<lw::Rgb8Color>::color1(lw::Rgb8Color(0, 0, 0));
    const auto to = lw::colors::palettes::Palette<lw::Rgb8Color>::color1(lw::Rgb8Color(255, 255, 255));
    using BaseSampler = lw::colors::palettes::TransitionPaletteSampler<lw::Rgb8Color>;
    const lw::colors::palettes::ScaledPaletteSampler<lw::Rgb8Color, BaseSampler> sampler(
        BaseSampler(from, to, 128u, {}), 256u, 1024u);

    const lw::Rgb8Color first = sampler(0u);
    const lw::Rgb8Color middle = sampler(512u);
    const lw::Rgb8Color last = sampler(1023u);

    TEST_ASSERT_EQUAL_UINT8(127u, first['R']);
    TEST_ASSERT_EQUAL_UINT8(127u, middle['R']);
    TEST_ASSERT_EQUAL_UINT8(127u, last['R']);
}

void test_palette_samples_range_supports_std_copy(void)
{
    const auto palette = makePalette();
    std::array<lw::Rgb8Color, 3> out{};
    lw::IndexRange paletteIndexes(0, 128, out.size());

    const auto samples = lw::colors::palettes::paletteSamples(palette, paletteIndexes);
    std::copy(samples.begin(), samples.end(), out.begin());

    TEST_ASSERT_EQUAL_UINT8(0, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(255, out[2]['R']);
}

void test_palette_transition_samples_range_supports_std_copy(void)
{
    const auto from = lw::colors::palettes::Palette<lw::Rgb8Color>::color1(lw::Rgb8Color(0, 0, 0));
    const auto to = lw::colors::palettes::Palette<lw::Rgb8Color>::color1(lw::Rgb8Color(255, 255, 255));
    std::array<lw::Rgb8Color, 3> out{};
    lw::IndexRange paletteIndexes(10, 30, out.size());

    const auto samples = lw::colors::palettes::paletteTransitionSamples(from, to, paletteIndexes, 128);
    std::copy(samples.begin(), samples.end(), out.begin());

    TEST_ASSERT_EQUAL_UINT8(127, out[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, out[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(127, out[2]['R']);
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
    RUN_TEST(test_scaled_palette_sampler_stretches_indexes_using_integer_mapping);
    RUN_TEST(test_scaled_palette_sampler_reuses_cached_color_for_repeated_mapped_indexes);
    RUN_TEST(test_scaled_palette_sampler_wraps_transition_sampler);
    RUN_TEST(test_palette_samples_range_supports_std_copy);
    RUN_TEST(test_palette_transition_samples_range_supports_std_copy);
    return UNITY_END();
}
