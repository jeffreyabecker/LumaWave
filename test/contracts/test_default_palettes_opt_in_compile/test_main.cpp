#include <unity.h>

#include <LumaWave.h>
#include <LumaWave/DefaultPalettes.h>

namespace
{
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

void test_default_palettes_opt_in_compile(void)
{
    using namespace lw::palettes;

    const auto staticPalettes = StaticPalettes<lw::Rgb8Color>();
    TEST_ASSERT_TRUE(staticPalettes.size() >= 68u);

    const auto party = staticPalettes.front().create();
    TEST_ASSERT_FALSE(party.stops().empty());
    TEST_ASSERT_EQUAL_UINT32(16u, static_cast<uint32_t>(party.stops().size()));
    TEST_ASSERT_NOT_EQUAL(0u, party.typeCode());
    TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::StaticPalette<lw::Rgb8Color>::TypeCode, party.typeCode());

    const lw::Rgb8Color primary(10, 20, 30);
    const lw::Rgb8Color secondary(40, 50, 60);
    const lw::Rgb8Color tertiary(70, 80, 90);

    using PaletteType = lw::colors::palettes::Palette<lw::Rgb8Color>;
    using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;

    const std::array<Stop, 2> color1Stops = {
        Stop{0, primary},
        Stop{255, primary},
    };
    const std::array<Stop, 4> splitStops = {
        Stop{0, primary},
        Stop{127, primary},
        Stop{128, secondary},
        Stop{255, secondary},
    };
    const std::array<Stop, 3> gradientStops = {
        Stop{0, tertiary},
        Stop{127, secondary},
        Stop{255, primary},
    };
    const std::array<Stop, 16> triStops = {
        Stop{0, primary},     Stop{16, primary},    Stop{32, primary},   Stop{48, primary},
        Stop{64, primary},    Stop{80, secondary},  Stop{96, secondary}, Stop{112, secondary},
        Stop{128, secondary}, Stop{144, secondary}, Stop{160, tertiary}, Stop{176, tertiary},
        Stop{192, tertiary},  Stop{208, tertiary},  Stop{224, tertiary}, Stop{255, primary},
    };

    const PaletteType color1(color1Stops);
    const PaletteType split(splitStops);
    const PaletteType gradient(gradientStops);
    const PaletteType tri(triStops);

    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<uint32_t>(color1.stops().size()));
    TEST_ASSERT_EQUAL_UINT32(4u, static_cast<uint32_t>(split.stops().size()));
    TEST_ASSERT_EQUAL_UINT32(3u, static_cast<uint32_t>(gradient.stops().size()));
    TEST_ASSERT_EQUAL_UINT32(16u, static_cast<uint32_t>(tri.stops().size()));

    TEST_ASSERT_NOT_EQUAL(0u, color1.typeCode());
    TEST_ASSERT_EQUAL_UINT32(PaletteType::TypeCode, color1.typeCode());
    TEST_ASSERT_EQUAL_UINT32(PaletteType::TypeCode, split.typeCode());
    TEST_ASSERT_EQUAL_UINT32(PaletteType::TypeCode, gradient.typeCode());
    TEST_ASSERT_EQUAL_UINT32(PaletteType::TypeCode, tri.typeCode());
    TEST_ASSERT_NOT_EQUAL(color1.typeCode(), party.typeCode());
}

void test_palette_sync_copies_same_type_and_ignores_mismatched_type(void)
{
    using namespace lw::palettes;

    using PaletteType = lw::colors::palettes::Palette<lw::Rgb8Color>;
    using StaticPaletteType = lw::colors::palettes::StaticPalette<lw::Rgb8Color>;
    using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;

    const std::array<Stop, 2> sourceStops = {
        Stop{0, lw::Rgb8Color(11, 22, 33)},
        Stop{255, lw::Rgb8Color(44, 55, 66)},
    };
    const std::array<Stop, 2> targetStops = {
        Stop{0, lw::Rgb8Color(1, 2, 3)},
        Stop{255, lw::Rgb8Color(4, 5, 6)},
    };

    PaletteType source(sourceStops);
    PaletteType target(targetStops);
    source.syncTo(&target);
    assert_same_stops(source, target);

    const auto staticPalettes = StaticPalettes<lw::Rgb8Color>();
    StaticPaletteType staticTarget = staticPalettes.front().create();
    const auto before = staticTarget.stops();
    std::array<Stop, 16> beforeStops{};
    for (size_t i = 0; i < before.size(); ++i)
    {
        beforeStops[i] = before[i];
    }

    source.syncTo(&staticTarget);

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(beforeStops.size()),
                             static_cast<uint32_t>(staticTarget.stops().size()));
    for (size_t i = 0; i < beforeStops.size(); ++i)
    {
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(beforeStops[i].index),
                                 static_cast<uint32_t>(staticTarget.stops()[i].index));
        TEST_ASSERT_TRUE(beforeStops[i].color == staticTarget.stops()[i].color);
    }
}

void test_static_palette_sync_copies_same_type(void)
{
    using namespace lw::palettes;

    using StaticPaletteType = lw::colors::palettes::StaticPalette<lw::Rgb8Color>;

    const auto staticPalettes = StaticPalettes<lw::Rgb8Color>();
    StaticPaletteType source = staticPalettes[0].create();
    StaticPaletteType target = staticPalettes[1].create();

    source.syncTo(&target);
    assert_same_stops(source, target);
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
    RUN_TEST(test_default_palettes_opt_in_compile);
    RUN_TEST(test_palette_sync_copies_same_type_and_ignores_mismatched_type);
    RUN_TEST(test_static_palette_sync_copies_same_type);
    return UNITY_END();
}
