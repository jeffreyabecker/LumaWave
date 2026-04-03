#include <unity.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

#include "colors/Color.h"
#include "colors/ChannelMap.h"
#include "colors/ChannelSource.h"

namespace
{
constexpr char channel_tag_for_index(size_t idx)
{
  switch (idx)
  {
    case 0:
      return 'R';
    case 1:
      return 'G';
    case 2:
      return 'B';
    case 3:
      return 'W';
    case 4:
      return 'C';
    default:
      return 'R';
  }
}

template <typename TColor> typename TColor::ComponentType read_channel(const TColor& color, size_t idx)
{
  return color[channel_tag_for_index(idx)];
}

template <typename TColor> void write_channel(TColor& color, size_t idx, typename TColor::ComponentType value)
{
  color[channel_tag_for_index(idx)] = value;
}

template <typename TColor> void assert_all_channels_zero(const TColor& color)
{
  for (size_t idx = 0; idx < TColor::ChannelCount; ++idx)
  {
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(read_channel(color, idx)));
  }
}

template <typename TColor> void assert_prefix_tail(const TColor& color, const typename TColor::ComponentType* prefix, size_t prefixSize)
{
  for (size_t idx = 0; idx < prefixSize; ++idx)
  {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(prefix[idx]), static_cast<uint32_t>(read_channel(color, idx)));
  }

  for (size_t idx = prefixSize; idx < TColor::ChannelCount; ++idx)
  {
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(read_channel(color, idx)));
  }
}

template <typename TColor> bool try_write_channel(TColor& color, size_t idx, typename TColor::ComponentType value)
{
  if (idx >= TColor::ChannelCount)
  {
    return false;
  }

  write_channel(color, idx, value);
  return true;
}

template <typename TColor> bool try_read_channel(const TColor& color, size_t idx, typename TColor::ComponentType& value)
{
  if (idx >= TColor::ChannelCount)
  {
    return false;
  }

  value = read_channel(color, idx);
  return true;
}

void test_1_1_1_default_construction_zero_initialization(void)
{
  assert_all_channels_zero(lw::Rgb8Color{});
  assert_all_channels_zero(lw::Rgbw8Color{});
  assert_all_channels_zero(lw::Rgbcw8Color{});
  assert_all_channels_zero(lw::Rgb16Color{});
  assert_all_channels_zero(lw::Rgbw16Color{});
  assert_all_channels_zero(lw::Rgbcw16Color{});
}

void test_1_1_2_variadic_constructor_prefix_assignment(void)
{
  {
    const lw::Rgb8Color color(7, 9);
    const uint8_t expected[] = {7, 9};
    assert_prefix_tail(color, expected, 2);
  }

  {
    const lw::Rgbw8Color color(1, 2, 3);
    const uint8_t expected[] = {1, 2, 3};
    assert_prefix_tail(color, expected, 3);
  }

  {
    const lw::Rgbcw8Color color(5, 6, 7, 8);
    const uint8_t expected[] = {5, 6, 7, 8};
    assert_prefix_tail(color, expected, 4);
  }

  {
    const lw::Rgb16Color color(111, 222);
    const uint16_t expected[] = {111, 222};
    assert_prefix_tail(color, expected, 2);
  }

  {
    const lw::Rgbw16Color color(1000, 2000, 3000);
    const uint16_t expected[] = {1000, 2000, 3000};
    assert_prefix_tail(color, expected, 3);
  }

  {
    const lw::Rgbcw16Color color(10, 20, 30, 40, 50);
    const uint16_t expected[] = {10, 20, 30, 40, 50};
    assert_prefix_tail(color, expected, 5);
  }
}

void test_1_1_3_component_type_and_channel_metadata(void)
{
  static_assert(lw::Rgb8Color::ChannelCount == 3);
  static_assert(lw::Rgbw8Color::ChannelCount == 4);
  static_assert(lw::Rgbcw8Color::ChannelCount == 5);
  static_assert(lw::Rgb16Color::ChannelCount == 3);
  static_assert(lw::Rgbw16Color::ChannelCount == 4);
  static_assert(lw::Rgbcw16Color::ChannelCount == 5);

  static_assert(std::is_same_v<lw::Rgb8Color::ComponentType, uint8_t>);
  static_assert(std::is_same_v<lw::Rgbw8Color::ComponentType, uint8_t>);
  static_assert(std::is_same_v<lw::Rgbcw8Color::ComponentType, uint8_t>);
  static_assert(std::is_same_v<lw::Rgb16Color::ComponentType, uint16_t>);
  static_assert(std::is_same_v<lw::Rgbw16Color::ComponentType, uint16_t>);
  static_assert(std::is_same_v<lw::Rgbcw16Color::ComponentType, uint16_t>);

  static_assert(std::is_same_v<lw::LargerColorTypeT<lw::Rgb8Color, lw::Rgbw8Color>, lw::Rgbw8Color>);
  static_assert(std::is_same_v<lw::LargerColorTypeT<lw::Rgbw8Color, lw::Rgb16Color>, lw::Rgb16Color>);
  static_assert(std::is_same_v<lw::LargerColorTypeT<lw::Rgbcw16Color, lw::Rgbw16Color>, lw::Rgbcw16Color>);
  static_assert(lw::ColorAtLeastAsLarge<lw::Rgbcw16Color, lw::Rgbw16Color>);
  static_assert(!lw::ColorAtLeastAsLarge<lw::Rgb8Color, lw::Rgb16Color>);

  using ExpectedRgb8InternalComponent = std::conditional_t<(lw::ColorMinimumComponentSizeBits == 16), uint16_t, uint8_t>;

  static_assert(std::is_same_v<lw::Rgb8Color::InternalComponentType, ExpectedRgb8InternalComponent>);
  static_assert(std::is_same_v<lw::Rgbw8Color::InternalComponentType, ExpectedRgb8InternalComponent>);
  static_assert(std::is_same_v<lw::Rgbcw8Color::InternalComponentType, ExpectedRgb8InternalComponent>);

  TEST_ASSERT_EQUAL_UINT8(std::numeric_limits<uint8_t>::max(), lw::Rgb8Color::MaxComponent);
  TEST_ASSERT_EQUAL_UINT8(std::numeric_limits<uint8_t>::max(), lw::Rgbw8Color::MaxComponent);
  TEST_ASSERT_EQUAL_UINT8(std::numeric_limits<uint8_t>::max(), lw::Rgbcw8Color::MaxComponent);
  TEST_ASSERT_EQUAL_UINT16(std::numeric_limits<uint16_t>::max(), lw::Rgb16Color::MaxComponent);
  TEST_ASSERT_EQUAL_UINT16(std::numeric_limits<uint16_t>::max(), lw::Rgbw16Color::MaxComponent);
  TEST_ASSERT_EQUAL_UINT16(std::numeric_limits<uint16_t>::max(), lw::Rgbcw16Color::MaxComponent);
}

void test_1_2_1_channel_tag_read_write_round_trip(void)
{
  lw::Rgbcw8Color color{};

  color['R'] = 11;
  color['G'] = 22;
  color['B'] = 33;
  color['W'] = 44;
  color['C'] = 55;

  TEST_ASSERT_EQUAL_UINT8(11, color['R']);
  TEST_ASSERT_EQUAL_UINT8(22, color['G']);
  TEST_ASSERT_EQUAL_UINT8(33, color['B']);
  TEST_ASSERT_EQUAL_UINT8(44, color['W']);
  TEST_ASSERT_EQUAL_UINT8(55, color['C']);

  const lw::Rgbcw8Color& constView = color;
  TEST_ASSERT_EQUAL_UINT8(11, constView['R']);
  TEST_ASSERT_EQUAL_UINT8(22, constView['G']);
  TEST_ASSERT_EQUAL_UINT8(33, constView['B']);
  TEST_ASSERT_EQUAL_UINT8(44, constView['W']);
  TEST_ASSERT_EQUAL_UINT8(55, constView['C']);
}

void test_1_2_2_character_index_mapping_upper_lower_case(void)
{
  lw::Rgbcw8Color color{1, 2, 3, 4, 5};

  TEST_ASSERT_EQUAL_UINT8(1, color['R']);
  TEST_ASSERT_EQUAL_UINT8(2, color['G']);
  TEST_ASSERT_EQUAL_UINT8(3, color['B']);
  TEST_ASSERT_EQUAL_UINT8(4, color['W']);
  TEST_ASSERT_EQUAL_UINT8(5, color['C']);

  TEST_ASSERT_EQUAL_UINT8(1, color['r']);
  TEST_ASSERT_EQUAL_UINT8(2, color['g']);
  TEST_ASSERT_EQUAL_UINT8(3, color['b']);
  TEST_ASSERT_EQUAL_UINT8(4, color['w']);
  TEST_ASSERT_EQUAL_UINT8(5, color['c']);
}

void test_1_2_3_unknown_channel_fallback_behavior(void)
{
  lw::Rgbcw8Color color{77, 2, 3, 4, 5};

  TEST_ASSERT_EQUAL_UINT8(77, color['X']);
  TEST_ASSERT_EQUAL_UINT8(77, color['?']);
}

void test_1_2_4_wc_fallback_on_lower_channel_colors(void)
{
  lw::Rgb8Color rgb{10, 20, 30};
  lw::Rgbw8Color rgbw{40, 50, 60, 70};

  TEST_ASSERT_EQUAL_UINT8(10, rgb['W']);
  TEST_ASSERT_EQUAL_UINT8(10, rgb['C']);

  TEST_ASSERT_EQUAL_UINT8(70, rgbw['W']);
  TEST_ASSERT_EQUAL_UINT8(40, rgbw['C']);
}

void test_1_3_1_equality_operator_correctness(void)
{
  const lw::Rgbcw8Color lhs{1, 2, 3, 4, 5};
  const lw::Rgbcw8Color equal{1, 2, 3, 4, 5};
  const lw::Rgbcw8Color different{1, 2, 3, 9, 5};

  TEST_ASSERT_TRUE(lhs == equal);
  TEST_ASSERT_FALSE(lhs == different);
  TEST_ASSERT_FALSE(lhs != equal);
  TEST_ASSERT_TRUE(lhs != different);
}

void test_1_3_2_ordering_operators_compare_canonical_channel_order(void)
{
  const lw::Rgb8Color lower{1, 2, 3};
  const lw::Rgb8Color higherBlue{1, 2, 4};
  const lw::Rgb8Color same{1, 2, 3};
  const lw::Rgbcw16Color cPreferredLower{100, 200, 300, 600, 400};
  const lw::Rgbcw16Color cPreferredHigher{100, 200, 300, 0, 500};

  TEST_ASSERT_TRUE(lower < higherBlue);
  TEST_ASSERT_TRUE(lower <= higherBlue);
  TEST_ASSERT_TRUE(higherBlue > lower);
  TEST_ASSERT_TRUE(higherBlue >= lower);

  TEST_ASSERT_TRUE(lower <= same);
  TEST_ASSERT_TRUE(lower >= same);
  TEST_ASSERT_FALSE(lower < same);
  TEST_ASSERT_FALSE(lower > same);

  TEST_ASSERT_TRUE(cPreferredLower < cPreferredHigher);
  TEST_ASSERT_TRUE(cPreferredLower <= cPreferredHigher);
  TEST_ASSERT_TRUE(cPreferredHigher > cPreferredLower);
  TEST_ASSERT_TRUE(cPreferredHigher >= cPreferredLower);
}

void test_1_3_3_channel_order_string_length_consistency(void)
{
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::RGB::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::RGB::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::GRB::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::GRB::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::BGR::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::BGR::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::RGBW::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::RGBW::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::GRBW::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::GRBW::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::BGRW::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::BGRW::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::RGBCW::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::RGBCW::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::GRBCW::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::GRBCW::value)));
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(lw::ChannelOrder::BGRCW::length), static_cast<uint32_t>(std::char_traits<char>::length(lw::ChannelOrder::BGRCW::value)));
}

void test_1_4_1_widen_conversion_formula(void)
{
  const lw::Rgbcw8Color src{0x00, 0x01, 0x7F, 0x80, 0xFF};
  const auto widened = lw::widen(src);

  TEST_ASSERT_EQUAL_UINT16(0x0000, widened['R']);
  TEST_ASSERT_EQUAL_UINT16(0x0101, widened['G']);
  TEST_ASSERT_EQUAL_UINT16(0x7F7F, widened['B']);
  TEST_ASSERT_EQUAL_UINT16(0x8080, widened['W']);
  TEST_ASSERT_EQUAL_UINT16(0xFFFF, widened['C']);
}

void test_1_4_2_narrow_conversion_formula(void)
{
  const lw::Rgbcw16Color src{0x0000, 0x01FF, 0x7F00, 0x80AA, 0xFFFF};
  const auto narrowed = lw::narrow(src);

  TEST_ASSERT_EQUAL_UINT8(0x00, narrowed['R']);
  TEST_ASSERT_EQUAL_UINT8(0x01, narrowed['G']);
  TEST_ASSERT_EQUAL_UINT8(0x7F, narrowed['B']);
  TEST_ASSERT_EQUAL_UINT8(0x80, narrowed['W']);
  TEST_ASSERT_EQUAL_UINT8(0xFF, narrowed['C']);
}

void test_1_4_3_expand_ordering_and_zero_fill(void)
{
  const lw::Rgb8Color src{9, 8, 7};
  const auto expanded = lw::expand<5>(src);

  TEST_ASSERT_EQUAL_UINT8(9, expanded['R']);
  TEST_ASSERT_EQUAL_UINT8(8, expanded['G']);
  TEST_ASSERT_EQUAL_UINT8(7, expanded['B']);
  TEST_ASSERT_EQUAL_UINT8(0, expanded['W']);
  TEST_ASSERT_EQUAL_UINT8(0, expanded['C']);
}

void test_1_4_4_compress_ordering(void)
{
  const lw::Rgbcw16Color src{11, 22, 33, 44, 55};
  const auto compressed = lw::compress<3>(src);

  TEST_ASSERT_EQUAL_UINT16(11, compressed['R']);
  TEST_ASSERT_EQUAL_UINT16(22, compressed['G']);
  TEST_ASSERT_EQUAL_UINT16(33, compressed['B']);
}

void test_1_4_5_implicit_rgbw_packed_conversion(void)
{
  {
    const lw::Rgb8Color rgb{0x12, 0x34, 0x56};
    const uint32_t packed = rgb;
    TEST_ASSERT_EQUAL_HEX32(0x00123456u, packed);
  }

  {
    const lw::Rgbw8Color rgbw{0x12, 0x34, 0x56, 0x78};
    const uint32_t packed = rgbw;
    TEST_ASSERT_EQUAL_HEX32(0x78123456u, packed);
  }

  {
    const lw::Rgb16Color rgb{0x1111, 0x2222, 0x3333};
    const uint64_t packed = rgb;
    TEST_ASSERT_EQUAL_HEX64(0x0000111122223333ull, packed);
  }

  {
    const lw::Rgbw16Color rgbw{0x1111, 0x2222, 0x3333, 0x4444};
    const uint64_t packed = rgbw;
    TEST_ASSERT_EQUAL_HEX64(0x4444111122223333ull, packed);
  }

  {
    const lw::Rgb8Color rgb{0x12, 0x34, 0x56};
    const int32_t packed = rgb;
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(0x00123456u), packed);
  }

  {
    const lw::Rgbw8Color rgbw{0x12, 0x34, 0x56, 0x78};
    const int32_t packed = rgbw;
    TEST_ASSERT_EQUAL_INT32(static_cast<int32_t>(0x78123456u), packed);
  }

  {
    const lw::Rgb16Color rgb{0x1111, 0x2222, 0x3333};
    const int64_t packed = rgb;
    TEST_ASSERT_EQUAL_INT64(static_cast<int64_t>(0x0000111122223333ull), packed);
  }

  {
    const lw::Rgbw16Color rgbw{0x1111, 0x2222, 0x3333, 0x4444};
    const int64_t packed = rgbw;
    TEST_ASSERT_EQUAL_INT64(static_cast<int64_t>(0x4444111122223333ull), packed);
  }
}

void test_1_4_6_packed_integer_assignment(void)
{
  {
    lw::Rgb8Color rgb{};
    rgb = static_cast<uint32_t>(0xAA112233u);
    TEST_ASSERT_EQUAL_UINT8(0x11, rgb['R']);
    TEST_ASSERT_EQUAL_UINT8(0x22, rgb['G']);
    TEST_ASSERT_EQUAL_UINT8(0x33, rgb['B']);
  }

  {
    lw::Rgbw8Color rgbw{};
    rgbw = static_cast<uint32_t>(0xAA112233u);
    TEST_ASSERT_EQUAL_UINT8(0xAA, rgbw['W']);
    TEST_ASSERT_EQUAL_UINT8(0x11, rgbw['R']);
    TEST_ASSERT_EQUAL_UINT8(0x22, rgbw['G']);
    TEST_ASSERT_EQUAL_UINT8(0x33, rgbw['B']);
  }

  {
    lw::Rgbw8Color rgbw{};
    rgbw = static_cast<int32_t>(0xFF010203u);
    TEST_ASSERT_EQUAL_UINT8(0xFF, rgbw['W']);
    TEST_ASSERT_EQUAL_UINT8(0x01, rgbw['R']);
    TEST_ASSERT_EQUAL_UINT8(0x02, rgbw['G']);
    TEST_ASSERT_EQUAL_UINT8(0x03, rgbw['B']);
  }

  {
    lw::Rgb16Color rgb{};
    rgb = static_cast<uint64_t>(0xAAAA111122223333ull);
    TEST_ASSERT_EQUAL_UINT16(0x1111, rgb['R']);
    TEST_ASSERT_EQUAL_UINT16(0x2222, rgb['G']);
    TEST_ASSERT_EQUAL_UINT16(0x3333, rgb['B']);
  }

  {
    lw::Rgbw16Color rgbw{};
    rgbw = static_cast<int64_t>(0xFFFF000100020003ull);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, rgbw['W']);
    TEST_ASSERT_EQUAL_UINT16(0x0001, rgbw['R']);
    TEST_ASSERT_EQUAL_UINT16(0x0002, rgbw['G']);
    TEST_ASSERT_EQUAL_UINT16(0x0003, rgbw['B']);
  }
}

void test_1_5_1_p0_out_of_range_channel_access_use_guarded(void)
{
  lw::Rgb8Color color{10, 20, 30};
  uint8_t readValue = 0;

  TEST_ASSERT_TRUE(try_write_channel(color, 1, 99));
  TEST_ASSERT_TRUE(try_read_channel(color, 1, readValue));
  TEST_ASSERT_EQUAL_UINT8(99, readValue);

  TEST_ASSERT_FALSE(try_write_channel(color, 3, 77));
  TEST_ASSERT_FALSE(try_read_channel(color, 3, readValue));

  TEST_ASSERT_EQUAL_UINT8(10, color['R']);
  TEST_ASSERT_EQUAL_UINT8(99, color['G']);
  TEST_ASSERT_EQUAL_UINT8(30, color['B']);
}

void test_1_5_2_boundary_stress_for_conversion_helpers(void)
{
  {
    const lw::Rgb8Color srcMin{0x00, 0x00, 0x00};
    const auto widenedMin = lw::widen(srcMin);
    TEST_ASSERT_EQUAL_UINT16(0x0000, widenedMin['R']);
    TEST_ASSERT_EQUAL_UINT16(0x0000, widenedMin['G']);
    TEST_ASSERT_EQUAL_UINT16(0x0000, widenedMin['B']);
  }

  {
    const lw::Rgb8Color srcMax{0xFF, 0xFF, 0xFF};
    const auto widenedMax = lw::widen(srcMax);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, widenedMax['R']);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, widenedMax['G']);
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, widenedMax['B']);
  }

  {
    const lw::Rgb16Color srcMin{0x0000, 0x0000, 0x0000};
    const auto narrowedMin = lw::narrow(srcMin);
    TEST_ASSERT_EQUAL_UINT8(0x00, narrowedMin['R']);
    TEST_ASSERT_EQUAL_UINT8(0x00, narrowedMin['G']);
    TEST_ASSERT_EQUAL_UINT8(0x00, narrowedMin['B']);
  }

  {
    const lw::Rgb16Color srcMax{0xFFFF, 0xFFFF, 0xFFFF};
    const auto narrowedMax = lw::narrow(srcMax);
    TEST_ASSERT_EQUAL_UINT8(0xFF, narrowedMax['R']);
    TEST_ASSERT_EQUAL_UINT8(0xFF, narrowedMax['G']);
    TEST_ASSERT_EQUAL_UINT8(0xFF, narrowedMax['B']);
  }

  {
    const lw::Rgb8Color src{0xAA, 0x00, 0xFF};
    const auto expanded = lw::expand<5>(src);
    TEST_ASSERT_EQUAL_UINT8(0xAA, expanded['R']);
    TEST_ASSERT_EQUAL_UINT8(0x00, expanded['G']);
    TEST_ASSERT_EQUAL_UINT8(0xFF, expanded['B']);
    TEST_ASSERT_EQUAL_UINT8(0x00, expanded['W']);
    TEST_ASSERT_EQUAL_UINT8(0x00, expanded['C']);
  }

  {
    const lw::Rgbcw8Color src{0x12, 0x34, 0x56, 0x78, 0x9A};
    const auto compressed = lw::compress<3>(src);
    TEST_ASSERT_EQUAL_UINT8(0x12, compressed['R']);
    TEST_ASSERT_EQUAL_UINT8(0x34, compressed['G']);
    TEST_ASSERT_EQUAL_UINT8(0x56, compressed['B']);
  }
}

struct ChannelFoo
{
  int R;
  int G;
  int B;
  int W;
  int C;
};

void test_1_4_1_channel_map_accepts_struct_channel_members(void)
{
  lw::ChannelMap<lw::Rgbcw8Color, int> map{};
  map = ChannelFoo{.R = 123, .G = 45, .B = 67, .W = 89, .C = 10};

  TEST_ASSERT_EQUAL_INT(123, map['R']);
  TEST_ASSERT_EQUAL_INT(45, map['G']);
  TEST_ASSERT_EQUAL_INT(67, map['B']);
  TEST_ASSERT_EQUAL_INT(89, map['W']);
  TEST_ASSERT_EQUAL_INT(10, map['C']);
}

void test_1_4_2_channel_map_uses_relevant_members_for_color_depth(void)
{
  lw::ChannelMap<lw::Rgb8Color, int> map(ChannelFoo{.R = 9, .G = 8, .B = 7, .W = 6, .C = 5});

  TEST_ASSERT_EQUAL_INT(9, map['R']);
  TEST_ASSERT_EQUAL_INT(8, map['G']);
  TEST_ASSERT_EQUAL_INT(7, map['B']);
}

void test_1_4_3_channel_source_exposes_expected_fields_by_color(void)
{
  lw::ChannelSource<lw::Rgb8Color> rgb{.R = 1, .G = 2, .B = 3};
  lw::ChannelSource<lw::Rgbw8Color> rgbw{.R = 4, .G = 5, .B = 6, .W = 7};
  lw::ChannelSource<lw::Rgbcw8Color> rgbcw{.R = 8, .G = 9, .B = 10, .W = 11, .C = 12};

  TEST_ASSERT_EQUAL_UINT8(1, rgb.R);
  TEST_ASSERT_EQUAL_UINT8(7, rgbw.W);
  TEST_ASSERT_EQUAL_UINT8(12, rgbcw.C);
}

void test_1_4_4_channel_source_assigns_into_channel_map(void)
{
  lw::ChannelMap<lw::Rgbcw8Color, int> map{};
  map = lw::ChannelSource<lw::Rgbcw8Color, int>{.R = 21, .G = 22, .B = 23, .W = 24, .C = 25};

  TEST_ASSERT_EQUAL_INT(21, map['R']);
  TEST_ASSERT_EQUAL_INT(22, map['G']);
  TEST_ASSERT_EQUAL_INT(23, map['B']);
  TEST_ASSERT_EQUAL_INT(24, map['W']);
  TEST_ASSERT_EQUAL_INT(25, map['C']);
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
  RUN_TEST(test_1_1_1_default_construction_zero_initialization);
  RUN_TEST(test_1_1_2_variadic_constructor_prefix_assignment);
  RUN_TEST(test_1_1_3_component_type_and_channel_metadata);
  RUN_TEST(test_1_2_1_channel_tag_read_write_round_trip);
  RUN_TEST(test_1_2_2_character_index_mapping_upper_lower_case);
  RUN_TEST(test_1_2_3_unknown_channel_fallback_behavior);
  RUN_TEST(test_1_2_4_wc_fallback_on_lower_channel_colors);
  RUN_TEST(test_1_3_1_equality_operator_correctness);
  RUN_TEST(test_1_3_2_ordering_operators_compare_canonical_channel_order);
  RUN_TEST(test_1_3_3_channel_order_string_length_consistency);
  RUN_TEST(test_1_4_1_channel_map_accepts_struct_channel_members);
  RUN_TEST(test_1_4_2_channel_map_uses_relevant_members_for_color_depth);
  RUN_TEST(test_1_4_3_channel_source_exposes_expected_fields_by_color);
  RUN_TEST(test_1_4_4_channel_source_assigns_into_channel_map);
  RUN_TEST(test_1_4_1_widen_conversion_formula);
  RUN_TEST(test_1_4_2_narrow_conversion_formula);
  RUN_TEST(test_1_4_3_expand_ordering_and_zero_fill);
  RUN_TEST(test_1_4_4_compress_ordering);
  RUN_TEST(test_1_4_5_implicit_rgbw_packed_conversion);
  RUN_TEST(test_1_4_6_packed_integer_assignment);
  RUN_TEST(test_1_5_1_p0_out_of_range_channel_access_use_guarded);
  RUN_TEST(test_1_5_2_boundary_stress_for_conversion_helpers);
  return UNITY_END();
}
