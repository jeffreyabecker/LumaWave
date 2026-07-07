#include <unity.h>

#include <array>
#include <memory>

#include "buses/ReferenceLightBus.h"
#include "colors/Color.h"
#include "transports/ILightDriver.h"

namespace
{
using TestColor = lw::Rgbw8Color;

class CaptureLightDriver : public lw::transports::ILightDriver<TestColor>
{
public:
  ~CaptureLightDriver() override { ++destructorCount; }

  void begin() override { began = true; }

  bool isReadyToUpdate() const override { return ready; }

  void write(const TestColor& color) override
  {
    ++writeCount;
    lastColor = color;
  }

  static int destructorCount;
  bool began{false};
  bool ready{true};
  size_t writeCount{0};
  TestColor lastColor{};
};

int CaptureLightDriver::destructorCount = 0;

struct OwnedColor
{
  static int destructorCount;

  OwnedColor() = default;

  ~OwnedColor() { ++destructorCount; }
};

int OwnedColor::destructorCount = 0;

class OwnedLightDriver : public lw::transports::ILightDriver<OwnedColor>
{
public:
  ~OwnedLightDriver() override { ++destructorCount; }

  void begin() override {}

  bool isReadyToUpdate() const override { return true; }

  void write(const OwnedColor&) override {}

  static int destructorCount;
};

int OwnedLightDriver::destructorCount = 0;

void resetDestructorCounters()
{
  CaptureLightDriver::destructorCount = 0;
  OwnedLightDriver::destructorCount = 0;
  OwnedColor::destructorCount = 0;
}

void test_reference_light_bus_writes_root_pixel(void)
{
  resetDestructorCounters();

  auto driver = std::make_unique<CaptureLightDriver>();
  auto* driverPtr = driver.get();

  lw::busses::ReferenceLightBus<TestColor> bus(std::move(driver));
  bus.pixels()[0] = TestColor{3, 4, 5};

  bus.begin();
  bus.show();

  TEST_ASSERT_TRUE(driverPtr->began);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(driverPtr->writeCount));
  TEST_ASSERT_EQUAL_UINT8(3, driverPtr->lastColor['R']);
  TEST_ASSERT_EQUAL_UINT8(3, bus.rootBuffer()->operator[]('R'));
}

void test_reference_light_bus_owns_all_resources(void)
{
  resetDestructorCounters();

  {
    auto driver = std::make_unique<OwnedLightDriver>();

    lw::busses::ReferenceLightBus<OwnedColor> bus(std::move(driver));
  }

  TEST_ASSERT_EQUAL_INT(1, OwnedLightDriver::destructorCount);
  TEST_ASSERT_EQUAL_INT(2, OwnedColor::destructorCount);
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
  RUN_TEST(test_reference_light_bus_writes_root_pixel);
  RUN_TEST(test_reference_light_bus_owns_all_resources);
  return UNITY_END();
}
