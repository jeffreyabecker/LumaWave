#include <unity.h>

#include <array>
#include <limits>

#include "buses/Bus.h"
#include "buses/ProtocolTransportPipeline.h"
#include "core/OutputPipeline.h"
#include "core/Pixel.h"

namespace
{

class MockPipeline : public lw::OutputPipeline
{
public:
  void begin() override { began = true; }
  bool isReadyToUpdate() const override { return ready; }
  bool alwaysUpdate() const override { return always; }

  void write(lw::span<const lw::Color> colors) override
  {
    ++writeCount;
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
  lw::Color lastColor{};
  size_t lastSpanSize{0};
};

void test_bus_single_run_writes_on_show(void)
{
  lw::Color pixel{};
  MockPipeline mock;

  lw::buses::PipelineRun runs[] = {{&mock, 1}};
  lw::buses::Bus bus(lw::span<lw::Color>{&pixel, 1}, lw::span<const lw::buses::PipelineRun>{runs});

  bus.begin();
  bus.pixels()[0] = lw::colorFromRGB(10, 20, 30);
  bus.show();

  TEST_ASSERT_TRUE(mock.began);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mock.writeCount));
  TEST_ASSERT_EQUAL_UINT8(10, lw::colorR(mock.lastColor));
  TEST_ASSERT_EQUAL_UINT8(20, lw::colorG(mock.lastColor));
  TEST_ASSERT_EQUAL_UINT8(30, lw::colorB(mock.lastColor));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mock.lastSpanSize));
}

void test_bus_dirty_guard_prevents_double_write(void)
{
  lw::Color pixel{};
  MockPipeline mock;

  lw::buses::PipelineRun runs[] = {{&mock, 1}};
  lw::buses::Bus bus(lw::span<lw::Color>{&pixel, 1}, lw::span<const lw::buses::PipelineRun>{runs});

  bus.pixels()[0] = lw::colorFromRGB(1, 2, 3);
  bus.show();
  bus.show(); // second show should be no-op
  bus.show();

  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mock.writeCount));
}

void test_bus_respects_pipeline_readiness(void)
{
  lw::Color pixel{};
  MockPipeline mock;
  mock.ready = false;

  lw::buses::PipelineRun runs[] = {{&mock, 1}};
  lw::buses::Bus bus(lw::span<lw::Color>{&pixel, 1}, lw::span<const lw::buses::PipelineRun>{runs});

  TEST_ASSERT_FALSE(bus.isReadyToUpdate());

  bus.pixels()[0] = lw::colorFromRGB(5, 6, 7);
  bus.show();

  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(mock.writeCount));

  mock.ready = true;
  TEST_ASSERT_TRUE(bus.isReadyToUpdate());
  bus.show();
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mock.writeCount));
}

void test_bus_multi_run_zero_copy_subspans(void)
{
  std::array<lw::Color, 5> buf{};
  MockPipeline mock1;
  MockPipeline mock2;

  lw::buses::PipelineRun runs[] = {{&mock1, 2}, {&mock2, 3}};
  lw::buses::Bus bus(lw::span<lw::Color>{buf}, lw::span<const lw::buses::PipelineRun>{runs});

  buf[0] = lw::colorFromRGB(1, 0, 0);
  buf[1] = lw::colorFromRGB(2, 0, 0);
  buf[2] = lw::colorFromRGB(3, 0, 0);
  buf[3] = lw::colorFromRGB(4, 0, 0);
  buf[4] = lw::colorFromRGB(5, 0, 0);

  bus.show();

  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mock1.writeCount));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(mock2.writeCount));
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(mock1.lastSpanSize));
  TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(mock2.lastSpanSize));
  TEST_ASSERT_EQUAL_UINT8(1, lw::colorR(mock1.lastColor));
  TEST_ASSERT_EQUAL_UINT8(3, lw::colorR(mock2.lastColor));
}

void test_bus_writes_go_to_caller_buffer(void)
{
  std::array<lw::Color, 3> buf{};
  MockPipeline mock;

  lw::buses::PipelineRun runs[] = {{&mock, 3}};
  lw::buses::Bus bus(lw::span<lw::Color>{buf}, lw::span<const lw::buses::PipelineRun>{runs});

  bus.pixels()[1] = lw::colorFromRGB(42, 0, 0);

  TEST_ASSERT_EQUAL_UINT8(42, lw::colorR(buf[1]));
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
  RUN_TEST(test_bus_multi_run_zero_copy_subspans);
  RUN_TEST(test_bus_writes_go_to_caller_buffer);
  return UNITY_END();
}
