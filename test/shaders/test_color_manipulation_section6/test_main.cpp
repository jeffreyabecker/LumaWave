#include <unity.h>

#include <cstdint>

#include "colors/Color.h"
#include "colors/ColorMath.h"

namespace
{
void test_6_1_1_darken_saturating_subtract_rgb8(void)
{
    lw::Rgb8Color color(5, 10, 250);
    lw::darken(color, static_cast<uint8_t>(10));

    TEST_ASSERT_EQUAL_UINT8(0, color['R']);
    TEST_ASSERT_EQUAL_UINT8(0, color['G']);
    TEST_ASSERT_EQUAL_UINT8(240, color['B']);
}

void test_6_1_2_lighten_saturating_add_rgb16(void)
{
    lw::Rgb16Color color(65500, 10, 40000);
    lw::lighten(color, static_cast<uint16_t>(100));

    TEST_ASSERT_EQUAL_UINT16(65535, color['R']);
    TEST_ASSERT_EQUAL_UINT16(110, color['G']);
    TEST_ASSERT_EQUAL_UINT16(40100, color['B']);
}

void test_6_1_3_channel_agnostic_works_for_five_channels(void)
{
    lw::Rgbcw8Color color(1, 2, 3, 4, 5);

    lw::lighten(color, static_cast<uint8_t>(10));
    TEST_ASSERT_EQUAL_UINT8(11, color['R']);
    TEST_ASSERT_EQUAL_UINT8(12, color['G']);
    TEST_ASSERT_EQUAL_UINT8(13, color['B']);
    TEST_ASSERT_EQUAL_UINT8(14, color['W']);
    TEST_ASSERT_EQUAL_UINT8(15, color['C']);

    lw::darken(color, static_cast<uint8_t>(20));
    TEST_ASSERT_EQUAL_UINT8(0, color['R']);
    TEST_ASSERT_EQUAL_UINT8(0, color['G']);
    TEST_ASSERT_EQUAL_UINT8(0, color['B']);
    TEST_ASSERT_EQUAL_UINT8(0, color['W']);
    TEST_ASSERT_EQUAL_UINT8(0, color['C']);
}

void test_6_2_1_linear_blend_float_endpoints_and_midpoint(void)
{
    const lw::Rgb8Color left(10, 20, 30);
    const lw::Rgb8Color right(110, 220, 130);

    const auto at0 = lw::linearBlendUnitFloat(left, right, 0.0f);
    const auto at1 = lw::linearBlendUnitFloat(left, right, 1.0f);
    const auto atHalf = lw::linearBlendUnitFloat(left, right, 0.5f);

    TEST_ASSERT_EQUAL_UINT8(10, at0['R']);
    TEST_ASSERT_EQUAL_UINT8(20, at0['G']);
    TEST_ASSERT_EQUAL_UINT8(30, at0['B']);

    TEST_ASSERT_EQUAL_UINT8(110, at1['R']);
    TEST_ASSERT_EQUAL_UINT8(220, at1['G']);
    TEST_ASSERT_EQUAL_UINT8(130, at1['B']);

    TEST_ASSERT_EQUAL_UINT8(60, atHalf['R']);
    TEST_ASSERT_EQUAL_UINT8(120, atHalf['G']);
    TEST_ASSERT_EQUAL_UINT8(80, atHalf['B']);
}

void test_6_2_2_linear_blend_uint8_rounding_rgb8(void)
{
    const lw::Rgb8Color left(0, 10, 255);
    const lw::Rgb8Color right(255, 110, 0);

    const auto at0 = lw::linearBlendProgress8(left, right, static_cast<uint8_t>(0));
    const auto at255 = lw::linearBlendProgress8(left, right, static_cast<uint8_t>(255));
    const auto at128 = lw::linearBlendProgress8(left, right, static_cast<uint8_t>(128));

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

    const auto at128 = lw::linearBlendProgress8(left, right, static_cast<uint8_t>(128));

    TEST_ASSERT_EQUAL_UINT16(32767, at128['R']);
    TEST_ASSERT_EQUAL_UINT16(2000, at128['G']);
    TEST_ASSERT_EQUAL_UINT16(32767, at128['B']);
}

void test_6_2_4_linear_blend_uint16_rounding_rgb8(void)
{
    const lw::Rgb8Color left(0, 10, 255);
    const lw::Rgb8Color right(255, 110, 0);

    const auto at0 = lw::linearBlendProgress16(left, right, static_cast<uint16_t>(0));
    const auto at65535 = lw::linearBlendProgress16(left, right, static_cast<uint16_t>(65535));
    const auto at32768 = lw::linearBlendProgress16(left, right, static_cast<uint16_t>(32768));

    TEST_ASSERT_EQUAL_UINT8(0, at0['R']);
    TEST_ASSERT_EQUAL_UINT8(10, at0['G']);
    TEST_ASSERT_EQUAL_UINT8(255, at0['B']);

    TEST_ASSERT_EQUAL_UINT8(254, at65535['R']);
    TEST_ASSERT_EQUAL_UINT8(109, at65535['G']);
    TEST_ASSERT_EQUAL_UINT8(0, at65535['B']);

    TEST_ASSERT_EQUAL_UINT8(127, at32768['R']);
    TEST_ASSERT_EQUAL_UINT8(60, at32768['G']);
    TEST_ASSERT_EQUAL_UINT8(127, at32768['B']);
}

void test_6_2_5_bilinear_blend_weighted_interpolation(void)
{
    const lw::Rgb8Color c00(0, 0, 0);
    const lw::Rgb8Color c01(100, 100, 100);
    const lw::Rgb8Color c10(200, 200, 200);
    const lw::Rgb8Color c11(255, 255, 255);

    const auto blended = lw::bilinearBlend(c00, c01, c10, c11, 0.5f, 0.5f);

    TEST_ASSERT_EQUAL_UINT8(138, blended['R']);
    TEST_ASSERT_EQUAL_UINT8(138, blended['G']);
    TEST_ASSERT_EQUAL_UINT8(138, blended['B']);
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

    static constexpr lw::Rgbw8Color linearBlendUnitFloat(const lw::Rgbw8Color&, const lw::Rgbw8Color&, float)
    {
        return lw::Rgbw8Color(7, 7, 7, 7);
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

namespace lw
{
template <> struct ColorMathBackendSelector<Rgbw8Color>
{
    using Type = OverrideBackend;
};
} // namespace lw

namespace
{
void test_6_3_1_backend_selector_override_hook(void)
{
    const lw::Rgbw8Color left(1, 2, 3, 4);
    const lw::Rgbw8Color right(9, 8, 7, 6);

    const auto byFloat = lw::linearBlendUnitFloat(left, right, 0.25f);
    const auto byUint8 = lw::linearBlendProgress8(left, right, static_cast<uint8_t>(64));
    const auto byUint16 = lw::linearBlendProgress16(left, right, static_cast<uint16_t>(16384));

    TEST_ASSERT_EQUAL_UINT8(7, byFloat['R']);
    TEST_ASSERT_EQUAL_UINT8(7, byFloat['G']);
    TEST_ASSERT_EQUAL_UINT8(7, byFloat['B']);
    TEST_ASSERT_EQUAL_UINT8(7, byFloat['W']);

    TEST_ASSERT_EQUAL_UINT8(9, byUint8['R']);
    TEST_ASSERT_EQUAL_UINT8(9, byUint8['G']);
    TEST_ASSERT_EQUAL_UINT8(9, byUint8['B']);
    TEST_ASSERT_EQUAL_UINT8(9, byUint8['W']);

    TEST_ASSERT_EQUAL_UINT8(11, byUint16['R']);
    TEST_ASSERT_EQUAL_UINT8(11, byUint16['G']);
    TEST_ASSERT_EQUAL_UINT8(11, byUint16['B']);
    TEST_ASSERT_EQUAL_UINT8(11, byUint16['W']);
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
    RUN_TEST(test_6_1_1_darken_saturating_subtract_rgb8);
    RUN_TEST(test_6_1_2_lighten_saturating_add_rgb16);
    RUN_TEST(test_6_1_3_channel_agnostic_works_for_five_channels);
    RUN_TEST(test_6_2_1_linear_blend_float_endpoints_and_midpoint);
    RUN_TEST(test_6_2_2_linear_blend_uint8_rounding_rgb8);
    RUN_TEST(test_6_2_3_linear_blend_uint8_rounding_rgb16);
    RUN_TEST(test_6_2_4_linear_blend_uint16_rounding_rgb8);
    RUN_TEST(test_6_2_5_bilinear_blend_weighted_interpolation);
    RUN_TEST(test_6_3_1_backend_selector_override_hook);
    return UNITY_END();
}
