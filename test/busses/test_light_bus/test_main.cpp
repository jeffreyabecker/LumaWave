#include <unity.h>

#include <limits>

#include "buses/LightBus.h"
#include "colors/Color.h"
#include "colors/ColorMath.h"
#include "transports/ILightDriver.h"

namespace
{
using TestColor = lw::Rgb8Color;

class MockLightDriver : public lw::transports::ILightDriver<TestColor>
{
public:
  struct LightDriverSettingsType : lw::transports::LightDriverSettingsBase
  {
    bool initiallyReady{true};

    static LightDriverSettingsType normalize(LightDriverSettingsType settings) { return settings; }
  };

  using ColorType = TestColor;
  using BrightnessType = typename lw::transports::ILightDriver<TestColor>::BrightnessType;

  explicit MockLightDriver(LightDriverSettingsType settings) : ready(settings.initiallyReady) {}

  void begin() override { began = true; }

  bool isReadyToUpdate() const override { return ready; }

  void write(const TestColor& color) override { write(color, std::numeric_limits<BrightnessType>::max()); }

  void write(const TestColor& color, BrightnessType brightness) override
  {
    ++writeCount;
    lastColor = color;
    lastBrightness = brightness;
  }

  bool began{false};
  bool ready{true};
  size_t writeCount{0};
  TestColor lastColor{};
  BrightnessType lastBrightness{std::numeric_limits<BrightnessType>::max()};
};

void test_light_bus_writes_root_pixel_on_show(void)
{
  lw::busses::LightBus<TestColor, MockLightDriver> bus(MockLightDriver::LightDriverSettingsType{});

  bus.pixel() = TestColor{3, 4, 5};
  bus.show();

  TEST_ASSERT_TRUE(bus.driver().began);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.driver().writeCount));
  TEST_ASSERT_EQUAL_UINT8(3, bus.driver().lastColor['R']);
}

void test_light_bus_uses_dirty_guard(void)
{
  lw::busses::LightBus<TestColor, MockLightDriver> bus(MockLightDriver::LightDriverSettingsType{});

  bus.pixel() = TestColor{9, 1, 2};
  bus.show();
  bus.show();

  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.driver().writeCount));
  TEST_ASSERT_EQUAL_UINT8(9, bus.driver().lastColor['R']);
}

void test_light_bus_checks_driver_readiness(void)
{
  auto settings = MockLightDriver::LightDriverSettingsType{};
  settings.initiallyReady = false;
  lw::busses::LightBus<TestColor, MockLightDriver> bus(settings);
  bus.pixel() = TestColor{7, 8, 9};

  TEST_ASSERT_FALSE(bus.isReadyToUpdate());
  bus.show();
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(bus.driver().writeCount));

  bus.driver().ready = true;
  TEST_ASSERT_TRUE(bus.isReadyToUpdate());
  bus.show();
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.driver().writeCount));
}

void test_light_bus_passes_brightness_to_driver_write(void)
{
  lw::busses::LightBus<TestColor, MockLightDriver> bus(MockLightDriver::LightDriverSettingsType{});

  bus.setBrightness(static_cast<TestColor::ComponentType>(64));
  bus.pixel() = TestColor{120, 10, 11};
  bus.show();

  TEST_ASSERT_EQUAL_UINT8(120, bus.driver().lastColor['R']);
  TEST_ASSERT_EQUAL_UINT8(64, bus.driver().lastBrightness);
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
  RUN_TEST(test_light_bus_writes_root_pixel_on_show);
  RUN_TEST(test_light_bus_uses_dirty_guard);
  RUN_TEST(test_light_bus_checks_driver_readiness);
  RUN_TEST(test_light_bus_passes_brightness_to_driver_write);
  return UNITY_END();
}
