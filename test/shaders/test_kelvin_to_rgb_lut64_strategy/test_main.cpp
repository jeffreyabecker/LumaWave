#include <unity.h>

#include <array>
#include <cstdint>

#include "colors/KelvinToRgbStrategies.h"

namespace
{
template <typename TComponent, template <typename> class TStrategy> std::array<TComponent, 3> convert(uint16_t kelvin)
{
    return TStrategy<TComponent>::convert(kelvin);
}

void test_uint8_lut64_strategy_clamps_input_to_table_domain(void)
{
    const auto color1000 = convert<uint8_t, lw::KelvinToRgbLut64Strategy>(1000);
    const auto color1200 = convert<uint8_t, lw::KelvinToRgbLut64Strategy>(1200);
    const auto color2000 = convert<uint8_t, lw::KelvinToRgbLut64Strategy>(2000);

    TEST_ASSERT_EQUAL_UINT8(255u, color1000[0]);
    TEST_ASSERT_EQUAL_UINT8(137u, color1000[1]);
    TEST_ASSERT_EQUAL_UINT8(14u, color1000[2]);

    TEST_ASSERT_EQUAL_UINT8(255u, color1200[0]);
    TEST_ASSERT_EQUAL_UINT8(137u, color1200[1]);
    TEST_ASSERT_EQUAL_UINT8(14u, color1200[2]);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(color2000.data(), color1200.data(), 3u);
}

void test_uint16_lut64_strategy_clamps_low_end_to_first_lut_entry(void)
{
    const auto color1000 = convert<uint16_t, lw::KelvinToRgbLut64Strategy>(1000);
    const auto color2000 = convert<uint16_t, lw::KelvinToRgbLut64Strategy>(2000);

    TEST_ASSERT_EQUAL_UINT16(lw::colors::scaleComponent<uint16_t>(uint8_t{255}), color1000[0]);
    TEST_ASSERT_EQUAL_UINT16(lw::colors::scaleComponent<uint16_t>(uint8_t{137}), color1000[1]);
    TEST_ASSERT_EQUAL_UINT16(lw::colors::scaleComponent<uint16_t>(uint8_t{14}), color1000[2]);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(color2000.data(), color1000.data(), 3u);
}

void test_lut64_strategy_clamps_to_last_lut_entry_above_table(void)
{
    const auto last8 = convert<uint8_t, lw::KelvinToRgbLut64Strategy>(6800);
    const auto lut8 = convert<uint8_t, lw::KelvinToRgbLut64Strategy>(8000);
    const auto last16 = convert<uint16_t, lw::KelvinToRgbLut64Strategy>(6800);
    const auto lut16 = convert<uint16_t, lw::KelvinToRgbLut64Strategy>(8000);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(last8.data(), lut8.data(), 3u);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(last16.data(), lut16.data(), 3u);
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
    RUN_TEST(test_uint8_lut64_strategy_clamps_input_to_table_domain);
    RUN_TEST(test_uint16_lut64_strategy_clamps_low_end_to_first_lut_entry);
    RUN_TEST(test_lut64_strategy_clamps_to_last_lut_entry_above_table);
    return UNITY_END();
}