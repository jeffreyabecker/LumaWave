#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include <unity.h>

namespace lw::test
{
inline void assertByteSpansEqual(std::span<const uint8_t> expected, std::span<const uint8_t> actual)
{
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(expected.size()), static_cast<uint32_t>(actual.size()));

  for (size_t index = 0; index < expected.size(); ++index)
  {
    TEST_ASSERT_EQUAL_UINT8(expected[index], actual[index]);
  }
}
} // namespace lw::test
