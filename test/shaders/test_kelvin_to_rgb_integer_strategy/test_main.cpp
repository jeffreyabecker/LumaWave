#include <unity.h>

#include <algorithm>
#include <array>
#include <cstdint>

#include "colors/KelvinToRgbStrategies.h"

namespace
{
template <typename TComponent, template <typename> class TStrategy> std::array<TComponent, 3> convert(uint16_t kelvin)
{
    return TStrategy<TComponent>::convert(kelvin);
}

template <typename TComponent>
void assert_full_sweep_within_delta(uint16_t expected_max_delta, uint32_t expected_max_mismatches)
{
    uint32_t mismatch_count = 0;
    uint16_t max_delta = 0;

    for (uint32_t kelvin = lw::KelvinToRgbExactStrategy<TComponent>::MinKelvin;
         kelvin <= lw::KelvinToRgbExactStrategy<TComponent>::MaxKelvin; ++kelvin)
    {
        const auto exact = convert<TComponent, lw::KelvinToRgbExactStrategy>(static_cast<uint16_t>(kelvin));
        const auto integer = convert<TComponent, lw::KelvinToRgbIntegerStrategy>(static_cast<uint16_t>(kelvin));

        if (exact != integer)
        {
            ++mismatch_count;
        }

        for (size_t index = 0; index < exact.size(); ++index)
        {
            const auto left = static_cast<uint32_t>(exact[index]);
            const auto right = static_cast<uint32_t>(integer[index]);
            const uint16_t delta = static_cast<uint16_t>((left > right) ? (left - right) : (right - left));
            max_delta = std::max(max_delta, delta);
        }
    }

    TEST_ASSERT_LESS_OR_EQUAL_UINT32(expected_max_mismatches, mismatch_count);
    TEST_ASSERT_LESS_OR_EQUAL_UINT16(expected_max_delta, max_delta);
}

void test_uint8_integer_strategy_tracks_exact_strategy_across_full_kelvin_range(void)
{
    assert_full_sweep_within_delta<uint8_t>(2u, 10000u);
}

void test_uint16_integer_strategy_tracks_exact_strategy_across_full_kelvin_range(void)
{
    assert_full_sweep_within_delta<uint16_t>(514u, 10000u);
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
    RUN_TEST(test_uint8_integer_strategy_tracks_exact_strategy_across_full_kelvin_range);
    RUN_TEST(test_uint16_integer_strategy_tracks_exact_strategy_across_full_kelvin_range);
    return UNITY_END();
}
