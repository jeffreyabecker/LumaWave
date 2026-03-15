#include <unity.h>

#include <cstdint>

#include "colors/Color.h"
#include "colors/ColorMath.h"

namespace
{
void test_6_1_1_darken_saturating_subtract_rgb8(void)
{
    lw::Rgb8Color color(5, 10, 250);
    lw::colors::darken(color, static_cast<uint8_t>(10));

    TEST_ASSERT_EQUAL_UINT8(0, color['R']);
    TEST_ASSERT_EQUAL_UINT8(0, color['G']);
    TEST_ASSERT_EQUAL_UINT8(240, color['B']);
}

void test_6_1_2_lighten_saturating_add_rgb16(void)
{
    lw::Rgb16Color color(65500, 10, 40000);
    lw::colors::lighten(color, static_cast<uint16_t>(100));

    TEST_ASSERT_EQUAL_UINT16(65535, color['R']);
    TEST_ASSERT_EQUAL_UINT16(110, color['G']);
    TEST_ASSERT_EQUAL_UINT16(40100, color['B']);
}

void test_6_1_3_channel_agnostic_works_for_five_channels(void)
{
    lw::Rgbcw8Color color(1, 2, 3, 4, 5);

    lw::colors::lighten(color, static_cast<uint8_t>(10));
    TEST_ASSERT_EQUAL_UINT8(11, color['R']);
    TEST_ASSERT_EQUAL_UINT8(12, color['G']);
    TEST_ASSERT_EQUAL_UINT8(13, color['B']);
    TEST_ASSERT_EQUAL_UINT8(14, color['W']);
    TEST_ASSERT_EQUAL_UINT8(15, color['C']);

    lw::colors::darken(color, static_cast<uint8_t>(20));
    TEST_ASSERT_EQUAL_UINT8(0, color['R']);
    TEST_ASSERT_EQUAL_UINT8(0, color['G']);
    TEST_ASSERT_EQUAL_UINT8(0, color['B']);
    TEST_ASSERT_EQUAL_UINT8(0, color['W']);
    TEST_ASSERT_EQUAL_UINT8(0, color['C']);
}

void test_6_1_4_scale_component_maps_between_component_domains(void)
{
    TEST_ASSERT_EQUAL_UINT16(0u, lw::colors::scaleComponent<uint16_t>(static_cast<uint8_t>(0u)));
    TEST_ASSERT_EQUAL_UINT16(32896u, lw::colors::scaleComponent<uint16_t>(static_cast<uint8_t>(128u)));
    TEST_ASSERT_EQUAL_UINT16(65535u, lw::colors::scaleComponent<uint16_t>(static_cast<uint8_t>(255u)));

    TEST_ASSERT_EQUAL_UINT8(0u, lw::colors::scaleComponent<uint8_t>(static_cast<uint16_t>(0u)));
    TEST_ASSERT_EQUAL_UINT8(128u, lw::colors::scaleComponent<uint8_t>(static_cast<uint16_t>(32768u)));
    TEST_ASSERT_EQUAL_UINT8(255u, lw::colors::scaleComponent<uint8_t>(static_cast<uint16_t>(65535u)));
}

void test_6_1_5_interpolate_component_blends_scalar_domains(void)
{
    TEST_ASSERT_EQUAL_UINT8(10u, lw::colors::interpolateComponent<uint8_t>(static_cast<uint8_t>(10u),
                                                                           static_cast<uint8_t>(110u), 0u, 255u));
    TEST_ASSERT_EQUAL_UINT8(60u, lw::colors::interpolateComponent<uint8_t>(static_cast<uint8_t>(10u),
                                                                           static_cast<uint8_t>(110u), 128u, 255u));
    TEST_ASSERT_EQUAL_UINT8(110u, lw::colors::interpolateComponent<uint8_t>(static_cast<uint8_t>(10u),
                                                                            static_cast<uint8_t>(110u), 255u, 255u));

    TEST_ASSERT_EQUAL_UINT16(
        0u, lw::colors::interpolateComponent<uint16_t>(static_cast<uint8_t>(0u), static_cast<uint8_t>(255u), 0u, 255u));
    TEST_ASSERT_EQUAL_UINT16(32896u, lw::colors::interpolateComponent<uint16_t>(
                                         static_cast<uint8_t>(0u), static_cast<uint8_t>(255u), 128u, 255u));
    TEST_ASSERT_EQUAL_UINT16(65535u, lw::colors::interpolateComponent<uint16_t>(
                                         static_cast<uint8_t>(0u), static_cast<uint8_t>(255u), 255u, 255u));
}

void test_6_2_2_linear_blend_uint8_rounding_rgb8(void)
{
    const lw::Rgb8Color left(0, 10, 255);
    const lw::Rgb8Color right(255, 110, 0);

    const auto at0 = lw::colors::linearBlendProgress8(left, right, static_cast<uint8_t>(0));
    const auto at255 = lw::colors::linearBlendProgress8(left, right, static_cast<uint8_t>(255));
    const auto at128 = lw::colors::linearBlendProgress8(left, right, static_cast<uint8_t>(128));

    TEST_ASSERT_EQUAL_UINT8(0, at0['R']);
    TEST_ASSERT_EQUAL_UINT8(10, at0['G']);
    TEST_ASSERT_EQUAL_UINT8(255, at0['B']);

    TEST_ASSERT_EQUAL_UINT8(254, at255['R']);
    TEST_ASSERT_EQUAL_UINT8(109, at255['G']);
    TEST_ASSERT_EQUAL_UINT8(1, at255['B']);

    TEST_ASSERT_EQUAL_UINT8(127, at128['R']);
    TEST_ASSERT_EQUAL_UINT8(60, at128['G']);
    TEST_ASSERT_EQUAL_UINT8(127, at128['B']);
}

void test_6_2_3_linear_blend_uint8_rounding_rgb16(void)
{
    const lw::Rgb16Color left(0, 1000, 65535);
    const lw::Rgb16Color right(65535, 3000, 0);

    const auto at128 = lw::colors::linearBlendProgress8(left, right, static_cast<uint8_t>(128));

    TEST_ASSERT_EQUAL_UINT16(32767, at128['R']);
    TEST_ASSERT_EQUAL_UINT16(2000, at128['G']);
    TEST_ASSERT_EQUAL_UINT16(32767, at128['B']);
}

void test_6_2_4_linear_blend_uint16_rounding_rgb8(void)
{
    const lw::Rgb8Color left(0, 10, 255);
    const lw::Rgb8Color right(255, 110, 0);

    const auto at0 = lw::colors::linearBlendProgress16(left, right, static_cast<uint16_t>(0));
    const auto at65535 = lw::colors::linearBlendProgress16(left, right, static_cast<uint16_t>(65535));
    const auto at32768 = lw::colors::linearBlendProgress16(left, right, static_cast<uint16_t>(32768));

    TEST_ASSERT_EQUAL_UINT8(0, at0['R']);
    TEST_ASSERT_EQUAL_UINT8(10, at0['G']);
    TEST_ASSERT_EQUAL_UINT8(255, at0['B']);

    TEST_ASSERT_EQUAL_UINT8(255, at65535['R']);
    TEST_ASSERT_EQUAL_UINT8(110, at65535['G']);
    TEST_ASSERT_EQUAL_UINT8(0, at65535['B']);

    TEST_ASSERT_EQUAL_UINT8(128, at32768['R']);
    TEST_ASSERT_EQUAL_UINT8(60, at32768['G']);
    TEST_ASSERT_EQUAL_UINT8(127, at32768['B']);
}

struct OverrideBackend
{
    static constexpr void darken(lw::Rgbw8Color& color, uint8_t delta)
    {
        for (auto channel : lw::Rgbw8Color::channelIndexes())
        {
            auto& component = color[channel];
            component = static_cast<uint8_t>(component > delta ? component - delta : 0);
        }
    }

    static constexpr void lighten(lw::Rgbw8Color& color, uint8_t delta)
    {
        for (auto channel : lw::Rgbw8Color::channelIndexes())
        {
            auto& component = color[channel];
            const uint16_t next = static_cast<uint16_t>(component) + delta;
            component = static_cast<uint8_t>(next > 255 ? 255 : next);
        }
    }

    static constexpr lw::Rgbw8Color linearBlendProgress8(const lw::Rgbw8Color&, const lw::Rgbw8Color&, uint8_t)
    {
        return lw::Rgbw8Color(9, 9, 9, 9);
    }

    static constexpr lw::Rgbw8Color linearBlendProgress16(const lw::Rgbw8Color&, const lw::Rgbw8Color&, uint16_t)
    {
        return lw::Rgbw8Color(11, 11, 11, 11);
    }
};
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
    RUN_TEST(test_6_1_1_darken_saturating_subtract_rgb8);
    RUN_TEST(test_6_1_2_lighten_saturating_add_rgb16);
    RUN_TEST(test_6_1_3_channel_agnostic_works_for_five_channels);
    RUN_TEST(test_6_1_4_scale_component_maps_between_component_domains);
    RUN_TEST(test_6_1_5_interpolate_component_blends_scalar_domains);
    RUN_TEST(test_6_2_2_linear_blend_uint8_rounding_rgb8);
    RUN_TEST(test_6_2_3_linear_blend_uint8_rounding_rgb16);
    RUN_TEST(test_6_2_4_linear_blend_uint16_rounding_rgb8);
    return UNITY_END();
}
