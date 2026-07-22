#include <unity.h>

#include "core/Compat.h"
#include "core/Pixel.h"
#include "transports/Transport.h"
#include "transports/NilTransport.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_core_headers_compile(void)
{
  // Verify core types compile and default-construct on native
  lw::Pixel p{};
  TEST_ASSERT_EQUAL_UINT8(0, lw::pixelR(p));

  lw::transports::NilTransport t;
  t.begin();
  TEST_ASSERT_TRUE(t.isReadyToUpdate());
}

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_core_headers_compile);
  return UNITY_END();
}
