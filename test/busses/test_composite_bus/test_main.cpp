#include <unity.h>

#include <array>
#include <vector>

#include "buses/CompositeBus.h"
#include "colors/Color.h"

namespace
{
using TestColor = lw::Rgb8Color;

class StubBus : public lw::IPixelBus<TestColor>
{
  public:
    using ColorType = TestColor;
    using BrightnessType = typename lw::IPixelBus<TestColor>::BrightnessType;

    explicit StubBus(size_t pixelCount, bool ready = true)
        : _pixelsStorage(pixelCount), _chunks{lw::span<TestColor>{_pixelsStorage.data(), _pixelsStorage.size()}},
          _pixels(lw::span<lw::span<TestColor>>{_chunks.data(), _chunks.size()}), _ready(ready)
    {
    }

    void begin() override { ++beginCalls; }

    void show() override { ++showCalls; }

    bool isReadyToUpdate() const override { return _ready; }

    lw::PixelView<TestColor>& pixels() override
    {
        ++dirtyCalls;
        return _pixels;
    }

    const lw::PixelView<TestColor>& pixels() const override { return _pixels; }

    void setBrightness(BrightnessType brightness) override { _brightness = brightness; }

    BrightnessType brightness() const override { return _brightness; }

    size_t beginCalls{0};
    size_t showCalls{0};
    size_t dirtyCalls{0};

  private:
    std::vector<TestColor> _pixelsStorage;
    std::array<lw::span<TestColor>, 1> _chunks;
    lw::PixelView<TestColor> _pixels;
    BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
    bool _ready{true};
};

void test_composite_bus_pixels_concatenate_child_views(void)
{
    StubBus leftBus(2);
    StubBus rightBus(1);

    lw::busses::CompositeBus<StubBus, StubBus> composite(std::move(leftBus), std::move(rightBus));

    auto& pixels = composite.pixels();
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(pixels.size()));

    pixels[0] = TestColor{1, 2, 3};
    pixels[1] = TestColor{4, 5, 6};
    pixels[2] = TestColor{7, 8, 9};

    auto& buses = composite.buses();
    auto& left = std::get<0>(buses).pixels();
    auto& right = std::get<1>(buses).pixels();

    TEST_ASSERT_EQUAL_UINT8(1, left[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(4, left[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(7, right[0]['R']);
}

void test_composite_bus_forwards_lifecycle_and_ready_state(void)
{
    StubBus firstBus(1, true);
    StubBus secondBus(1, false);

    lw::busses::CompositeBus<StubBus, StubBus> composite(std::move(firstBus), std::move(secondBus));

    composite.begin();
    composite.show();

    auto& buses = composite.buses();
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(std::get<0>(buses).beginCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(std::get<1>(buses).beginCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(std::get<0>(buses).showCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(std::get<1>(buses).showCalls));
    TEST_ASSERT_FALSE(composite.isReadyToUpdate());
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
    RUN_TEST(test_composite_bus_pixels_concatenate_child_views);
    RUN_TEST(test_composite_bus_forwards_lifecycle_and_ready_state);
    return UNITY_END();
}
