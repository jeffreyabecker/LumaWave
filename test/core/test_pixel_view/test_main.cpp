#include <unity.h>

#include <array>
#include <functional>
#include <vector>

#include "colors/Color.h"
#include "core/PixelView.h"

namespace
{
using Color = lw::Rgb8Color;

void test_default_constructed_view_is_empty(void)
{
  lw::PixelView<Color> view;

  TEST_ASSERT_EQUAL_UINT32(0U, view.size());
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(view.chunks().size()));
  TEST_ASSERT_TRUE(view.begin() == view.end());
  TEST_ASSERT_TRUE(view.cbegin() == view.cend());
}

void test_slice_returns_expected_subsection(void)
{
  std::array<Color, 6> pixels = {Color{1, 2, 3}, Color{4, 5, 6}, Color{7, 8, 9}, Color{10, 11, 12}, Color{13, 14, 15}, Color{16, 17, 18}};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), 2);
  chunks.emplace_back(pixels.data() + 2, 4);

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};
  auto sliced = view.slice(1, 5);

  TEST_ASSERT_EQUAL_UINT32(4u, sliced.size());
  TEST_ASSERT_EQUAL_UINT8(4u, sliced[0]['R']);
  TEST_ASSERT_EQUAL_UINT8(7u, sliced[1]['R']);
  TEST_ASSERT_EQUAL_UINT8(10u, sliced[2]['R']);
  TEST_ASSERT_EQUAL_UINT8(13u, sliced[3]['R']);
}

void test_slice_write_through_updates_underlying_storage(void)
{
  std::array<Color, 4> pixels = {Color{10, 0, 0}, Color{20, 0, 0}, Color{30, 0, 0}, Color{40, 0, 0}};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), 1);
  chunks.emplace_back(pixels.data() + 1, 3);

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};
  auto sliced = view.slice(1, 3);

  sliced[1]['R'] = 99;

  TEST_ASSERT_EQUAL_UINT8(99u, pixels[2]['R']);
}

void test_slice_clamps_to_bounds_and_handles_reverse_range(void)
{
  std::array<Color, 3> pixels = {Color{1, 0, 0}, Color{2, 0, 0}, Color{3, 0, 0}};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), pixels.size());

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};

  auto clamped = view.slice(2, 100);
  TEST_ASSERT_EQUAL_UINT32(1u, clamped.size());
  TEST_ASSERT_EQUAL_UINT8(3u, clamped[0]['R']);

  auto reversed = view.slice(2, 1);
  TEST_ASSERT_EQUAL_UINT32(0u, reversed.size());
}

void test_fill_pixels_solid_color_updates_all_chunks(void)
{
  std::array<Color, 5> pixels = {Color{1, 0, 0}, Color{2, 0, 0}, Color{3, 0, 0}, Color{4, 0, 0}, Color{5, 0, 0}};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), 2);
  chunks.emplace_back(pixels.data() + 2, 3);

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};
  lw::fillPixels(view, Color{9, 8, 7});

  for (size_t i = 0; i < pixels.size(); ++i)
  {
    TEST_ASSERT_EQUAL_UINT8(9u, pixels[i]['R']);
    TEST_ASSERT_EQUAL_UINT8(8u, pixels[i]['G']);
    TEST_ASSERT_EQUAL_UINT8(7u, pixels[i]['B']);
  }
}

void test_fill_pixels_indexed_uses_global_index(void)
{
  std::array<Color, 6> pixels{};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), 1);
  chunks.emplace_back(pixels.data() + 1, 2);
  chunks.emplace_back(pixels.data() + 3, 3);

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};
  lw::fillPixelsIndexed(view, [](uint32_t index) { return Color(static_cast<uint8_t>(index), static_cast<uint8_t>(index + 1U), static_cast<uint8_t>(index + 2U)); });

  for (size_t i = 0; i < pixels.size(); ++i)
  {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(i), pixels[i]['R']);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(i + 1U), pixels[i]['G']);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(i + 2U), pixels[i]['B']);
  }
}

void test_iterators_walk_across_chunk_boundaries_in_order(void)
{
  std::array<Color, 5> pixels = {Color{1, 0, 0}, Color{2, 0, 0}, Color{3, 0, 0}, Color{4, 0, 0}, Color{5, 0, 0}};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), 1);
  chunks.emplace_back(pixels.data() + 1, 0);
  chunks.emplace_back(pixels.data() + 1, 2);
  chunks.emplace_back(pixels.data() + 3, 2);

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};

  uint8_t expected = 1;
  for (auto it = view.begin(); it != view.end(); ++it)
  {
    TEST_ASSERT_EQUAL_UINT8(expected, (*it)['R']);
    ++expected;
  }

  TEST_ASSERT_EQUAL_UINT8(6u, expected);
}

void test_iterators_preserve_random_access_operations(void)
{
  std::array<Color, 5> pixels = {Color{1, 0, 0}, Color{2, 0, 0}, Color{3, 0, 0}, Color{4, 0, 0}, Color{5, 0, 0}};

  std::vector<lw::span<Color>> chunks;
  chunks.emplace_back(pixels.data(), 2);
  chunks.emplace_back(pixels.data() + 2, 1);
  chunks.emplace_back(pixels.data() + 3, 2);

  lw::PixelView<Color> view{lw::span<lw::span<Color>>(chunks.data(), chunks.size())};

  auto it = view.begin();
  it += 3;
  TEST_ASSERT_EQUAL_UINT8(4u, (*it)['R']);
  TEST_ASSERT_EQUAL_UINT8(5u, it[1]['R']);

  --it;
  TEST_ASSERT_EQUAL_UINT8(3u, (*it)['R']);
  TEST_ASSERT_EQUAL_INT32(5, static_cast<int32_t>(view.end() - view.begin()));

  const lw::PixelView<Color>& constView = view;
  auto constIt = constView.cend();
  --constIt;
  TEST_ASSERT_EQUAL_UINT8(5u, (*constIt)['R']);
}

void test_concatenate_accepts_span_of_view_references(void)
{
  std::array<Color, 2> left = {Color{1, 0, 0}, Color{2, 0, 0}};
  std::array<Color, 3> right = {Color{3, 0, 0}, Color{4, 0, 0}, Color{5, 0, 0}};

  std::vector<lw::span<Color>> leftChunks;
  leftChunks.emplace_back(left.data(), left.size());
  std::vector<lw::span<Color>> rightChunks;
  rightChunks.emplace_back(right.data(), right.size());

  lw::PixelView<Color> leftView{lw::span<lw::span<Color>>(leftChunks.data(), leftChunks.size())};
  lw::PixelView<Color> rightView{lw::span<lw::span<Color>>(rightChunks.data(), rightChunks.size())};

  std::array<std::reference_wrapper<lw::PixelView<Color>>, 2> views{leftView, rightView};
  auto concatenated = lw::PixelView<Color>::concatenate(lw::span<const std::reference_wrapper<lw::PixelView<Color>>>(views.data(), views.size()));

  TEST_ASSERT_EQUAL_UINT32(5U, concatenated.size());
  concatenated[4]['R'] = 99;

  TEST_ASSERT_EQUAL_UINT8(99U, right[2]['R']);
}

void test_concatenate_accepts_span_of_const_view_references(void)
{
  std::array<Color, 1> left = {Color{7, 0, 0}};
  std::array<Color, 1> right = {Color{8, 0, 0}};

  std::vector<lw::span<Color>> leftChunks;
  leftChunks.emplace_back(left.data(), left.size());
  std::vector<lw::span<Color>> rightChunks;
  rightChunks.emplace_back(right.data(), right.size());

  const lw::PixelView<Color> leftView{lw::span<lw::span<Color>>(leftChunks.data(), leftChunks.size())};
  const lw::PixelView<Color> rightView{lw::span<lw::span<Color>>(rightChunks.data(), rightChunks.size())};

  std::array<std::reference_wrapper<const lw::PixelView<Color>>, 2> views{leftView, rightView};
  const auto concatenated = lw::PixelView<Color>::concatenate(lw::span<const std::reference_wrapper<const lw::PixelView<Color>>>(views.data(), views.size()));

  TEST_ASSERT_EQUAL_UINT32(2U, concatenated.size());
  TEST_ASSERT_EQUAL_UINT8(7U, concatenated[0]['R']);
  TEST_ASSERT_EQUAL_UINT8(8U, concatenated[1]['R']);
}
} // namespace

void setUp(void)
{
}

void tearDown(void)
{
}

int main(int, char**)
{
  UNITY_BEGIN();
  RUN_TEST(test_default_constructed_view_is_empty);
  RUN_TEST(test_slice_returns_expected_subsection);
  RUN_TEST(test_slice_write_through_updates_underlying_storage);
  RUN_TEST(test_slice_clamps_to_bounds_and_handles_reverse_range);
  RUN_TEST(test_fill_pixels_solid_color_updates_all_chunks);
  RUN_TEST(test_fill_pixels_indexed_uses_global_index);
  RUN_TEST(test_iterators_walk_across_chunk_boundaries_in_order);
  RUN_TEST(test_iterators_preserve_random_access_operations);
  RUN_TEST(test_concatenate_accepts_span_of_view_references);
  RUN_TEST(test_concatenate_accepts_span_of_const_view_references);
  return UNITY_END();
}
