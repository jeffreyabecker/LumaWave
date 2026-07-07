#include <unity.h>

#include <array>
#include <memory>
#include <vector>

#include "buses/AggregateBus.h"
#include "colors/Color.h"

namespace
{


class StubBus : public lw::IPixelBus
{
  public:
        using BrightnessType = typename lw::IPixelBus::BrightnessType;

    explicit StubBus(lw::span<lw::Color> pixels, bool ready = true)
        : _chunks{pixels}, _pixels(lw::span<lw::span<lw::Color>>{_chunks.data(), _chunks.size()}), _ready(ready)
    {
    }

    void begin() override { ++beginCalls; }

    void show() override { ++showCalls; }

    bool isReadyToUpdate() const override { return _ready; }

    lw::PixelView& pixels() override
    {
        ++dirtyCalls;
        return _pixels;
    }

    const lw::PixelView& pixels() const override { return _pixels; }

    void setBrightness(BrightnessType brightness) override { _brightness = brightness; }

    BrightnessType brightness() const override { return _brightness; }

    size_t beginCalls{0};
    size_t showCalls{0};
    size_t dirtyCalls{0};

  private:
    std::array<lw::span<lw::Color>, 1> _chunks;
    lw::PixelView _pixels;
        BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
    bool _ready{true};
};

void test_aggregate_bus_pixels_concatenate_child_views(void)
{
    std::array<lw::Color, 2> left{};
    std::array<lw::Color, 1> right{};

    auto leftBus = std::make_unique<StubBus>(lw::span<lw::Color>{left.data(), left.size()});
    auto rightBus = std::make_unique<StubBus>(lw::span<lw::Color>{right.data(), right.size()});
    auto* leftBusPtr = leftBus.get();
    auto* rightBusPtr = rightBus.get();

    std::vector<std::unique_ptr<lw::IPixelBus>> buses{};
    buses.emplace_back(std::move(leftBus));
    buses.emplace_back(std::move(rightBus));

    lw::busses::AggregateBus aggregate(std::move(buses));

    auto& pixels = aggregate.pixels();
    TEST_ASSERT_EQUAL_UINT32(3U, pixels.size());

    pixels[0] = lw::Color{1, 2, 3};
    pixels[1] = lw::Color{4, 5, 6};
    pixels[2] = lw::Color{7, 8, 9};

    TEST_ASSERT_EQUAL_UINT8(1, left[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(4, left[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(7, right[0]['R']);

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(leftBusPtr->dirtyCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(rightBusPtr->dirtyCalls));
}

void test_aggregate_bus_forwards_lifecycle_and_ready_state(void)
{
    std::array<lw::Color, 1> first{};
    std::array<lw::Color, 1> second{};

    auto firstBus = std::make_unique<StubBus>(lw::span<lw::Color>{first.data(), first.size()}, true);
    auto secondBus = std::make_unique<StubBus>(lw::span<lw::Color>{second.data(), second.size()}, false);
    auto* firstBusPtr = firstBus.get();
    auto* secondBusPtr = secondBus.get();

    std::vector<std::unique_ptr<lw::IPixelBus>> buses{};
    buses.emplace_back(std::move(firstBus));
    buses.emplace_back(std::move(secondBus));

    lw::busses::AggregateBus aggregate(std::move(buses));

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
    std::array<lw::Color, 2> left{};
    std::array<lw::Color, 1> right{};

    StubBus leftBus(lw::span<lw::Color>{left.data(), left.size()});
    StubBus rightBus(lw::span<lw::Color>{right.data(), right.size()});

    std::array<lw::IPixelBus*, 2> buses{&leftBus, &rightBus};
    lw::busses::ReferenceAggregateBus aggregate(
        lw::span<lw::IPixelBus*>{buses.data(), buses.size()});

    auto& pixels = aggregate.pixels();
    TEST_ASSERT_EQUAL_UINT32(3U, pixels.size());

    pixels[0] = lw::Color{1, 2, 3};
    pixels[1] = lw::Color{4, 5, 6};
    pixels[2] = lw::Color{7, 8, 9};

    TEST_ASSERT_EQUAL_UINT8(1, left[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(4, left[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(7, right[0]['R']);

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(leftBus.dirtyCalls));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(rightBus.dirtyCalls));
}

void test_reference_aggregate_bus_forwards_lifecycle_and_ready_state(void)
{
    std::array<lw::Color, 1> first{};
    std::array<lw::Color, 1> second{};

    StubBus firstBus(lw::span<lw::Color>{first.data(), first.size()}, true);
    StubBus secondBus(lw::span<lw::Color>{second.data(), second.size()}, false);

    std::array<lw::IPixelBus*, 2> buses{&firstBus, &secondBus};
    lw::busses::ReferenceAggregateBus aggregate(
        lw::span<lw::IPixelBus*>{buses.data(), buses.size()});

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
