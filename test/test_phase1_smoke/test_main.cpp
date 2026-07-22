#include <ArduinoFake.h>
#include <unity.h>

void setUp(void)
{
}

void tearDown(void)
{
}

void test_arduinofake_header_is_available(void)
{
  TEST_ASSERT_TRUE(true);
}

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_arduinofake_header_is_available);
  return UNITY_END();
}
