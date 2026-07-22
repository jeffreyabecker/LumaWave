#include <unity.h>

#include <array>
#include <type_traits>
#include <vector>

#include "palettes/Palette.h"

namespace
{
void test_palette_first_pass_compile(void)
{
  static_assert(std::is_same_v<lw::Pixel, uint32_t>, "Pixel must be uint32_t");
  static_assert(lw::palettes::IsPaletteLike<lw::palettes::Palette>::value, "Palette must satisfy IsPaletteLike");
  static_assert(lw::palettes::Palette::AllowedSettings.size() == 0u, "Palette must expose an empty AllowedSettings descriptor");
  static_assert(lw::palettes::blend::Linear == lw::palettes::BlendMode::Linear, "blend::Linear must map to BlendMode::Linear");
  static_assert(lw::palettes::blend::Nearest == lw::palettes::BlendMode::Nearest, "blend::Nearest must map to BlendMode::Nearest");
  static_assert(lw::palettes::blend::Midpoint == lw::palettes::BlendMode::HoldMidpoint, "blend::Midpoint must map to BlendMode::HoldMidpoint");
  static_assert(std::is_enum<lw::palettes::WrapMode>::value, "WrapMode must be an enum");
  static_assert(lw::palettes::WrapMode::Clamp != lw::palettes::WrapMode::Circular, "WrapMode values must remain distinct");

  lw::palettes::PaletteSampleOptions options;

  lw::palettes::PaletteStop invalidStop{};
  invalidStop.index = 0;
  invalidStop.color = lw::pixelFromRGB(1, 2, 3);

  lw::palettes::Palette invalidPalette(lw::span<const lw::palettes::PaletteStop>(&invalidStop, 1));
  TEST_ASSERT_TRUE(invalidPalette.stops().empty());

  std::array<lw::palettes::PaletteStop, 2> sampleStops = {lw::palettes::PaletteStop{0, lw::pixelFromRGB(0, 0, 0)}, lw::palettes::PaletteStop{255, lw::pixelFromRGB(255, 255, 255)}};
  lw::palettes::Palette samplePaletteLike(lw::span<const lw::palettes::PaletteStop>(sampleStops.data(), sampleStops.size()));
  lw::palettes::Palette ownedPaletteLike(std::vector<lw::palettes::PaletteStop>(sampleStops.begin(), sampleStops.end()));
  std::array<lw::Pixel, 2> sampledOutput{};
  lw::IndexRange sampleIndexes(0, 128, sampledOutput.size());
  const size_t sampledCount = lw::palettes::samplePalette(samplePaletteLike, sampleIndexes, lw::span<lw::Pixel>(sampledOutput.data(), sampledOutput.size()), options);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(sampledCount));

  const size_t ownedSampledCount = lw::palettes::samplePalette(ownedPaletteLike, sampleIndexes, lw::span<lw::Pixel>(sampledOutput.data(), sampledOutput.size()), options);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(ownedSampledCount));

  lw::IndexRange nearestSampleIndexes(0, 128, sampledOutput.size());
  lw::palettes::PaletteSampleOptions nearestOptions = options;
  nearestOptions.blendMode = lw::palettes::BlendMode::Nearest;
  const size_t nearestSampledCount = lw::palettes::samplePalette(samplePaletteLike, nearestSampleIndexes, lw::span<lw::Pixel>(sampledOutput.data(), sampledOutput.size()), nearestOptions);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(nearestSampledCount));

  lw::IndexRange midpointIndexes(0, 128, sampledOutput.size());
  lw::palettes::PaletteSampleOptions midpointOptions = options;
  midpointOptions.blendMode = lw::palettes::BlendMode::HoldMidpoint;
  const size_t midpointSampledCount = lw::palettes::samplePalette(samplePaletteLike, midpointIndexes, lw::span<lw::Pixel>(sampledOutput.data(), sampledOutput.size()), midpointOptions);
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sampledOutput.size()), static_cast<uint32_t>(midpointSampledCount));
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
  RUN_TEST(test_palette_first_pass_compile);
  return UNITY_END();
}
