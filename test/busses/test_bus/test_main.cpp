#include <unity.h>

#include <array>
#include <memory>
#include <limits>

#include "buses/Bus.h"
#include "buses/ProtocolTransportPipeline.h"
#include "core/IOutputPipeline.h"
#include "colors/Color.h"

namespace
{

class MockPipeline : public lw::busses::IOutputPipeline
{
public:
  void begin() override { began = true; }
  bool isReadyToUpdate() const override { return ready; }
  bool alwaysUpdate() const override { return always; }

  void write(lw::span<const lw::colors::Color> colors, lw::busses::IOutputPipeline::BrightnessType brightness) override
  {
    ++writeCount;
    lastBrightness = brightness;
    if (!colors.empty())
    {
      lastColor = colors[0];
    }
    lastSpanSize = colors.size();
  }

  bool began{false};
  bool ready{true};
  bool always{false};
  size_t writeCount{0};
  lw::colors::Color lastColor{};
  lw::busses::IOutputPipeline::BrightnessType lastBrightness{std::numeric_limits<lw::busses::IOutputPipeline::BrightnessType>::max()};
  size_t lastSpanSize{0};
};

void test_bus_single_run_writes_on_show(void)
{
  lw::Color pixel{};
  auto mock = std::make_unique<MockPipeline>();
  auto* mockPtr = mock.get();

  lw::busses::Bus bus(lw::span<lw::Color>{&pixel, 1}, {{std::move(mock), 1}});

  bus.pixels()[0] = lw::Color{10, 20, 30};
  bus.show();

  TEST_ASSERT_TRUE(mockPtr->began);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mockPtr->writeCount));
  TEST_ASSERT_EQUAL_UINT8(10, mockPtr->lastColor['R']);
  TEST_ASSERT_EQUAL_UINT8(20, mockPtr->lastColor['G']);
  TEST_ASSERT_EQUAL_UINT8(30, mockPtr->lastColor['B']);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mockPtr->lastSpanSize));
}

void test_bus_dirty_guard_prevents_double_write(void)
{
  lw::Color pixel{};
  auto mock = std::make_unique<MockPipeline>();
  auto* mockPtr = mock.get();

  lw::busses::Bus bus(lw::span<lw::Color>{&pixel, 1}, {{std::move(mock), 1}});

  bus.pixels()[0] = lw::Color{1, 2, 3};
  bus.show();
  bus.show(); // second show should be no-op
  bus.show();

  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mockPtr->writeCount));
}

void test_bus_respects_pipeline_readiness(void)
{
  lw::Color pixel{};
  auto mock = std::make_unique<MockPipeline>();
  auto* mockPtr = mock.get();
  mockPtr->ready = false;

  lw::busses::Bus bus(lw::span<lw::Color>{&pixel, 1}, {{std::move(mock), 1}});

  TEST_ASSERT_FALSE(bus.isReadyToUpdate());

  bus.pixels()[0] = lw::Color{5, 6, 7};
  bus.show();

  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(mockPtr->writeCount));

  mockPtr->ready = true;
  TEST_ASSERT_TRUE(bus.isReadyToUpdate());
  bus.show();
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mockPtr->writeCount));
}

void test_bus_passes_brightness_to_pipeline(void)
{
  lw::Color pixel{};
  auto mock = std::make_unique<MockPipeline>();
  auto* mockPtr = mock.get();

  lw::busses::Bus bus(lw::span<lw::Color>{&pixel, 1}, {{std::move(mock), 1}});

  bus.setBrightness(64);
  bus.pixels()[0] = lw::Color{100, 100, 100};
  bus.show();

  TEST_ASSERT_EQUAL_UINT8(64, mockPtr->lastBrightness);
}

void test_bus_multi_run_zero_copy_subspans(void)
{
  std::array<lw::Color, 5> buf{};
  auto mock1 = std::make_unique<MockPipeline>();
  auto mock2 = std::make_unique<MockPipeline>();
  auto* m1 = mock1.get();
  auto* m2 = mock2.get();

  lw::busses::Bus bus(lw::span<lw::Color>{buf}, {{std::move(mock1), 2}, {std::move(mock2), 3}});

  buf[0] = lw::Color{1, 0, 0};
  buf[1] = lw::Color{2, 0, 0};
  buf[2] = lw::Color{3, 0, 0};
  buf[3] = lw::Color{4, 0, 0};
  buf[4] = lw::Color{5, 0, 0};

  bus.show();

  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(m1->writeCount));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(m2->writeCount));
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(m1->lastSpanSize));
  TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(m2->lastSpanSize));
  TEST_ASSERT_EQUAL_UINT8(1, m1->lastColor['R']);
  TEST_ASSERT_EQUAL_UINT8(3, m2->lastColor['R']);
}

void test_bus_writes_go_to_caller_buffer(void)
{
  std::array<lw::Color, 3> buf{};
  auto mock = std::make_unique<MockPipeline>();

  lw::busses::Bus bus(lw::span<lw::Color>{buf}, {{std::move(mock), 3}});

  bus.pixels()[1] = lw::Color{42, 0, 0};

  TEST_ASSERT_EQUAL_UINT8(42, buf[1]['R']);
}

} // namespace

void setUp(void)
{
}
void tearDown(void)
{
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_bus_single_run_writes_on_show);
  RUN_TEST(test_bus_dirty_guard_prevents_double_write);
  RUN_TEST(test_bus_respects_pipeline_readiness);
  RUN_TEST(test_bus_passes_brightness_to_pipeline);
  RUN_TEST(test_bus_multi_run_zero_copy_subspans);
  RUN_TEST(test_bus_writes_go_to_caller_buffer);
  return UNITY_END();
}
