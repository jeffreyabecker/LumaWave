#include <unity.h>

#include <array>
#include <memory>
#include <vector>

#include "buses/AggregateBus.h"
#include "colors/Color.h"

namespace
{
using TestColor = lw::Rgb8Color;

class StubBus : public lw::IPixelBus<TestColor>
{
  public:
        using BrightnessType = typename lw::IPixelBus<TestColor>::BrightnessType;

    explicit StubBus(lw::span<TestColor> pixels, bool ready = true)
        : _chunks{pixels}, _pixels(lw::span<lw::span<TestColor>>{_chunks.data(), _chunks.size()}), _ready(ready)
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
    std::array<lw::span<TestColor>, 1> _chunks;
    lw::PixelView<TestColor> _pixels;
        BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
    bool _ready{true};
};

void test_aggregate_bus_pixels_concatenate_child_views(void)
{
    std::array<TestColor, 2> left{};
    std::array<TestColor, 1> right{};

    auto leftBus = std::make_unique<StubBus>(lw::span<TestColor>{left.data(), left.size()});
    auto rightBus = std::make_unique<StubBus>(lw::span<TestColor>{right.data(), right.size()});
    auto* leftBusPtr = leftBus.get();
    auto* rightBusPtr = rightBus.get();

    std::vector<std::unique_ptr<lw::IPixelBus<TestColor>>> buses{};
    buses.emplace_back(std::move(leftBus));
    buses.emplace_back(std::move(rightBus));

    lw::busses::AggregateBus<TestColor> aggregate(std::move(buses));

    auto& pixels = aggregate.pixels();
    TEST_ASSERT_EQUAL_UINT32(3U, pixels.size());

    pixels[0] = TestColor{1, 2, 3};
    pixels[1] = TestColor{4, 5, 6};
    pixels[2] = TestColor{7, 8, 9};

    TEST_ASSERT_EQUAL_UINT8(1, left[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(4, left[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(7, right[0]['R']);

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(leftBusPtr->dirtyCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(rightBusPtr->dirtyCalls));
}

void test_aggregate_bus_forwards_lifecycle_and_ready_state(void)
{
    std::array<TestColor, 1> first{};
    std::array<TestColor, 1> second{};

    auto firstBus = std::make_unique<StubBus>(lw::span<TestColor>{first.data(), first.size()}, true);
    auto secondBus = std::make_unique<StubBus>(lw::span<TestColor>{second.data(), second.size()}, false);
    auto* firstBusPtr = firstBus.get();
    auto* secondBusPtr = secondBus.get();

    std::vector<std::unique_ptr<lw::IPixelBus<TestColor>>> buses{};
    buses.emplace_back(std::move(firstBus));
    buses.emplace_back(std::move(secondBus));

    lw::busses::AggregateBus<TestColor> aggregate(std::move(buses));

    aggregate.begin();
    aggregate.show();

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(firstBusPtr->beginCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(secondBusPtr->beginCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(firstBusPtr->showCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(secondBusPtr->showCalls));
    TEST_ASSERT_FALSE(aggregate.isReadyToUpdate());
}

void test_reference_aggregate_bus_pixels_concatenate_child_views(void)
{
    std::array<TestColor, 2> left{};
    std::array<TestColor, 1> right{};

    StubBus leftBus(lw::span<TestColor>{left.data(), left.size()});
    StubBus rightBus(lw::span<TestColor>{right.data(), right.size()});

    std::array<lw::IPixelBus<TestColor>*, 2> buses{&leftBus, &rightBus};
    lw::busses::ReferenceAggregateBus<TestColor> aggregate(
        lw::span<lw::IPixelBus<TestColor>*>{buses.data(), buses.size()});

    auto& pixels = aggregate.pixels();
    TEST_ASSERT_EQUAL_UINT32(3U, pixels.size());

    pixels[0] = TestColor{1, 2, 3};
    pixels[1] = TestColor{4, 5, 6};
    pixels[2] = TestColor{7, 8, 9};

    TEST_ASSERT_EQUAL_UINT8(1, left[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(4, left[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(7, right[0]['R']);

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(leftBus.dirtyCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(rightBus.dirtyCalls));
}

void test_reference_aggregate_bus_forwards_lifecycle_and_ready_state(void)
{
    std::array<TestColor, 1> first{};
    std::array<TestColor, 1> second{};

    StubBus firstBus(lw::span<TestColor>{first.data(), first.size()}, true);
    StubBus secondBus(lw::span<TestColor>{second.data(), second.size()}, false);

    std::array<lw::IPixelBus<TestColor>*, 2> buses{&firstBus, &secondBus};
    lw::busses::ReferenceAggregateBus<TestColor> aggregate(
        lw::span<lw::IPixelBus<TestColor>*>{buses.data(), buses.size()});

    aggregate.begin();
    aggregate.show();

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(firstBus.beginCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(secondBus.beginCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(firstBus.showCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(secondBus.showCalls));
    TEST_ASSERT_FALSE(aggregate.isReadyToUpdate());
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
    RUN_TEST(test_aggregate_bus_pixels_concatenate_child_views);
    RUN_TEST(test_aggregate_bus_forwards_lifecycle_and_ready_state);
    RUN_TEST(test_reference_aggregate_bus_pixels_concatenate_child_views);
    RUN_TEST(test_reference_aggregate_bus_forwards_lifecycle_and_ready_state);
    return UNITY_END();
}
