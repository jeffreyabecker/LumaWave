#include <unity.h>

#include <cstdint>
#include <vector>

#include "core/Pixel.h"
#include "transports/PrintOutputPipeline.h"

namespace
{
class MockWritable
{
public:
  size_t write(const uint8_t* data, size_t length)
  {
    if (data == nullptr || length == 0)
    {
      return 0;
    }

    bytes.insert(bytes.end(), data, data + length);
    return length;
  }

  std::vector<uint8_t> bytes{};
};

using TestSettings = lw::transports::PrintOutputPipelineSettings<MockWritable>;
using TestDriver = lw::transports::PrintOutputPipeline<MockWritable>;

void test_print_light_driver_writes_binary_by_default(void)
{
  MockWritable sink{};
  TestSettings settings{};
  settings.output = &sink;

  TestDriver driver(settings);
  lw::Color color = lw::colorFromRGB(0x11, 0x22, 0x33);
  driver.write(lw::span<const lw::Color>{&color, 1});

  TEST_ASSERT_EQUAL_UINT32(4U, static_cast<uint32_t>(sink.bytes.size()));
  TEST_ASSERT_EQUAL_HEX8(0x11, sink.bytes[0]);
  TEST_ASSERT_EQUAL_HEX8(0x22, sink.bytes[1]);
  TEST_ASSERT_EQUAL_HEX8(0x33, sink.bytes[2]);
  TEST_ASSERT_EQUAL_HEX8(0x00, sink.bytes[3]);
}

void test_print_light_driver_writes_ascii_when_enabled(void)
{
  MockWritable sink{};
  TestSettings settings{};
  settings.output = &sink;
  settings.asciiOutput = true;

  TestDriver driver(settings);
  lw::Color color = lw::colorFromRGB(0x0A, 0x14, 0xFF);
  driver.write(lw::span<const lw::Color>{&color, 1});

  static constexpr char Expected[] = "0A14FF00";
  TEST_ASSERT_EQUAL_UINT32(sizeof(Expected) - 1U, static_cast<uint32_t>(sink.bytes.size()));
  TEST_ASSERT_EQUAL_MEMORY(Expected, sink.bytes.data(), sizeof(Expected) - 1U);
}

void test_print_light_driver_debug_prefix_includes_identifier(void)
{
  MockWritable sink{};
  TestSettings settings{};
  settings.output = &sink;
  settings.debugOutput = true;
  settings.identifier = "Desk";

  TestDriver driver(settings);
  driver.begin();
  lw::Color color = lw::colorFromRGB(0x01, 0x02, 0x03);
  driver.write(lw::span<const lw::Color>{&color, 1});

  static constexpr char ExpectedPrefix[] = "[LIGHT:Desk] begin\r\n[LIGHT:Desk] write\r\n";
  TEST_ASSERT_TRUE(sink.bytes.size() >= (sizeof(ExpectedPrefix) - 1U));
  TEST_ASSERT_EQUAL_MEMORY(ExpectedPrefix, sink.bytes.data(), sizeof(ExpectedPrefix) - 1U);
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
  RUN_TEST(test_print_light_driver_writes_binary_by_default);
  RUN_TEST(test_print_light_driver_writes_ascii_when_enabled);
  RUN_TEST(test_print_light_driver_debug_prefix_includes_identifier);
  return UNITY_END();
}
