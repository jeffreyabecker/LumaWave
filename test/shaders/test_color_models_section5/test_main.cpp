#include <unity.h>

#include <cstdint>

#include "colors/Color.h"
#include "colors/ColorMath.h"

namespace
{
void assert_rgb8_exact(const lw::Rgb8Color& actual, uint8_t r, uint8_t g, uint8_t b)
{
    TEST_ASSERT_EQUAL_UINT8(r, actual['R']);
    TEST_ASSERT_EQUAL_UINT8(g, actual['G']);
    TEST_ASSERT_EQUAL_UINT8(b, actual['B']);
}

void assert_rgb8_near(const lw::Rgb8Color& actual, uint8_t r, uint8_t g, uint8_t b, uint8_t tolerance)
{
    TEST_ASSERT_UINT8_WITHIN(tolerance, r, actual['R']);
    TEST_ASSERT_UINT8_WITHIN(tolerance, g, actual['G']);
    TEST_ASSERT_UINT8_WITHIN(tolerance, b, actual['B']);
}

void assert_rgb16_near(const lw::Rgb16Color& actual, uint16_t r, uint16_t g, uint16_t b, uint16_t tolerance)
{
    TEST_ASSERT_UINT16_WITHIN(tolerance, r, actual['R']);
    TEST_ASSERT_UINT16_WITHIN(tolerance, g, actual['G']);
    TEST_ASSERT_UINT16_WITHIN(tolerance, b, actual['B']);
}

void assert_rgbcw8_exact(const lw::Rgbcw8Color& actual, uint8_t r, uint8_t g, uint8_t b, uint8_t c, uint8_t w)
{
    TEST_ASSERT_EQUAL_UINT8(r, actual['R']);
    TEST_ASSERT_EQUAL_UINT8(g, actual['G']);
    TEST_ASSERT_EQUAL_UINT8(b, actual['B']);
    TEST_ASSERT_EQUAL_UINT8(c, actual['C']);
    TEST_ASSERT_EQUAL_UINT8(w, actual['W']);
}

void test_5_1_1_hsl_to_rgb8_canonical_vectors(void)
{
    assert_rgb8_near(lw::colors::hslToRgb<lw::Rgb8Color>(0u, 255u, 128u), 255, 0, 0, 8);
    assert_rgb8_near(lw::colors::hslToRgb<lw::Rgb8Color>(85u, 255u, 128u), 0, 255, 0, 8);
    assert_rgb8_near(lw::colors::hslToRgb<lw::Rgb8Color>(171u, 255u, 128u), 0, 0, 255, 8);
    assert_rgb8_exact(lw::colors::hslToRgb<lw::Rgb8Color>(64u, 0u, 128u), 128, 128, 128);
}

void test_5_1_2_hsb_to_rgb8_canonical_vectors(void)
{
    assert_rgb8_exact(lw::colors::hsbToRgb<lw::Rgb8Color>(0u, 255u, 255u), 255, 0, 0);
    assert_rgb8_near(lw::colors::hsbToRgb<lw::Rgb8Color>(85u, 255u, 255u), 0, 255, 0, 4);
    assert_rgb8_near(lw::colors::hsbToRgb<lw::Rgb8Color>(171u, 255u, 255u), 0, 0, 255, 4);
    assert_rgb8_exact(lw::colors::hsbToRgb<lw::Rgb8Color>(153u, 0u, 128u), 128, 128, 128);
}

void test_5_1_3_hsl_to_rgb16_component_domain_scaling(void)
{
    const auto red = lw::colors::hslToRgb<lw::Rgb16Color>(0u, 65535u, 32768u);
    const auto green = lw::colors::hslToRgb<lw::Rgb16Color>(21845u, 65535u, 32768u);
    const auto gray = lw::colors::hslToRgb<lw::Rgb16Color>(16384u, 0u, 32768u);

    assert_rgb16_near(red, 65535u, 0u, 0u, 2048u);
    assert_rgb16_near(green, 0u, 65535u, 0u, 2048u);
    assert_rgb16_near(gray, 32896u, 32896u, 32896u, 1024u);
}

void test_5_1_4_hsb_to_rgb16_component_domain_scaling(void)
{
    const auto red = lw::colors::hsbToRgb<lw::Rgb16Color>(0u, 65535u, 65535u);
    const auto blue = lw::colors::hsbToRgb<lw::Rgb16Color>(43947u, 65535u, 65535u);
    const auto gray = lw::colors::hsbToRgb<lw::Rgb16Color>(39321u, 0u, 32768u);

    assert_rgb16_near(red, 65535u, 0u, 0u, 1024u);
    assert_rgb16_near(blue, 0u, 0u, 65535u, 2048u);
    assert_rgb16_near(gray, 32896u, 32896u, 32896u, 1024u);
}

void test_5_2_1_hsl_to_rgb_zeroes_extra_white_channels(void)
{
    const auto hslRgbw = lw::colors::hslToRgb<lw::Rgbw8Color>(0u, 255u, 128u);
    TEST_ASSERT_UINT8_WITHIN(8, 255u, hslRgbw['R']);
    TEST_ASSERT_UINT8_WITHIN(8, 0u, hslRgbw['G']);
    TEST_ASSERT_UINT8_WITHIN(8, 0u, hslRgbw['B']);
    TEST_ASSERT_EQUAL_UINT8(0u, hslRgbw['W']);

    const auto hslRgbcw = lw::colors::hslToRgb<lw::Rgbcw8Color>(171u, 255u, 128u);
    TEST_ASSERT_UINT8_WITHIN(8, 0u, hslRgbcw['R']);
    TEST_ASSERT_UINT8_WITHIN(8, 0u, hslRgbcw['G']);
    TEST_ASSERT_UINT8_WITHIN(8, 255u, hslRgbcw['B']);
    TEST_ASSERT_EQUAL_UINT8(0u, hslRgbcw['C']);
    TEST_ASSERT_EQUAL_UINT8(0u, hslRgbcw['W']);
}

void test_5_2_2_hsb_to_rgb_zeroes_extra_white_channels(void)
{
    const auto hsbRgbw = lw::colors::hsbToRgb<lw::Rgbw8Color>(85u, 255u, 255u);
    TEST_ASSERT_UINT8_WITHIN(4, 0u, hsbRgbw['R']);
    TEST_ASSERT_UINT8_WITHIN(4, 255u, hsbRgbw['G']);
    TEST_ASSERT_UINT8_WITHIN(4, 0u, hsbRgbw['B']);
    TEST_ASSERT_EQUAL_UINT8(0u, hsbRgbw['W']);

    assert_rgbcw8_exact(lw::colors::hsbToRgb<lw::Rgbcw8Color>(0u, 0u, 128u), 128, 128, 128, 0, 0);
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
    RUN_TEST(test_5_1_1_hsl_to_rgb8_canonical_vectors);
    RUN_TEST(test_5_1_2_hsb_to_rgb8_canonical_vectors);
    RUN_TEST(test_5_1_3_hsl_to_rgb16_component_domain_scaling);
    RUN_TEST(test_5_1_4_hsb_to_rgb16_component_domain_scaling);
    RUN_TEST(test_5_2_1_hsl_to_rgb_zeroes_extra_white_channels);
    RUN_TEST(test_5_2_2_hsb_to_rgb_zeroes_extra_white_channels);
    return UNITY_END();
}
