#define LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES 1

#include <unity.h>

#include <array>
#include <memory>
#include <vector>

#include "LumaWave.h"

namespace
{
struct StubBus : lw::IPixelBus<lw::Rgb8Color>
{
    using BrightnessType = typename lw::IPixelBus<lw::Rgb8Color>::BrightnessType;

    void begin() override {}

    void show() override {}

    bool isReadyToUpdate() const override { return true; }

    lw::PixelView<lw::Rgb8Color>& pixels() override { return _pixels; }

    const lw::PixelView<lw::Rgb8Color>& pixels() const override { return _pixels; }

    void setBrightness(BrightnessType brightness) override { _brightness = brightness; }

    BrightnessType brightness() const override { return _brightness; }

  private:
    std::array<lw::Rgb8Color, 1> _storage{};
    std::array<lw::span<lw::Rgb8Color>, 1> _chunks{lw::span<lw::Rgb8Color>(_storage.data(), _storage.size())};
    lw::PixelView<lw::Rgb8Color> _pixels{lw::span<lw::span<lw::Rgb8Color>>(_chunks.data(), _chunks.size())};
    BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
};

struct NoOpShader : lw::shaders::IShader<lw::Rgb8Color>
{
    void apply(lw::span<lw::Rgb8Color>) override {}
};

void test_disable_template_combinatorial_types_compile(void)
{
    static_assert(LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES == 1,
                  "LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES must be enabled in this contract");
    static_assert(LW_HAS_TEMPLATE_COMBINATORIAL_TYPES == 0,
                  "LW_HAS_TEMPLATE_COMBINATORIAL_TYPES must reflect the disabled combinatorial surface");

    lw::AggregateShaderSettings<lw::Rgb8Color> shaderSettings{};
    shaderSettings.shaders.push_back(std::make_unique<NoOpShader>());
    lw::AggregateShader<lw::Rgb8Color> aggregateShader(std::move(shaderSettings));

    std::vector<std::unique_ptr<lw::IPixelBus<lw::Rgb8Color>>> buses{};
    buses.push_back(std::make_unique<StubBus>());
    buses.push_back(std::make_unique<StubBus>());
    lw::busses::AggregateBus<lw::Rgb8Color> aggregateBus(std::move(buses));

    StubBus leftBus;
    StubBus rightBus;
    std::array<lw::IPixelBus<lw::Rgb8Color>*, 2> referencedBuses{&leftBus, &rightBus};
    ReferenceAggregateStrip<lw::Rgb8Color> referenceAggregateBus(
        lw::span<lw::IPixelBus<lw::Rgb8Color>*>{referencedBuses.data(), referencedBuses.size()});

    TEST_ASSERT_TRUE(aggregateBus.isReadyToUpdate());
    TEST_ASSERT_TRUE(referenceAggregateBus.isReadyToUpdate());

    std::array<lw::Rgb8Color, 1> sampled{};
    lw::colors::palettes::Palette<lw::Rgb8Color> palette({
        lw::colors::palettes::PaletteStop<lw::Rgb8Color>{0, lw::Rgb8Color(0, 0, 0)},
        lw::colors::palettes::PaletteStop<lw::Rgb8Color>{255, lw::Rgb8Color(255, 255, 255)},
    });
    lw::IndexRange indexes(0, 1, sampled.size());
    const size_t written =
        lw::colors::palettes::samplePalette(palette, indexes, lw::span<lw::Rgb8Color>(sampled.data(), sampled.size()));

    TEST_ASSERT_EQUAL_UINT32(1, static_cast<uint32_t>(written));
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
    RUN_TEST(test_disable_template_combinatorial_types_compile);
    return UNITY_END();
}