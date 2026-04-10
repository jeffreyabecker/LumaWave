#include <unity.h>

#include <array>

#include "core/IndexIterator.h"
#include "colors/palette/Palette.h"

namespace
{
using Stop = lw::colors::palettes::PaletteStop<lw::Rgb8Color>;
using Palette = lw::colors::palettes::Palette<lw::Rgb8Color>;
using PaletteSetting = std::pair<const char*, const char*>;
using SettingDescriptor = lw::colors::palettes::PaletteSettingDescriptor;
using SettingValueType = lw::colors::palettes::PaletteSettingValueType;

template <typename TPalette> void assert_same_stops(const TPalette& a, const TPalette& b)
{
  const auto sa = a.stops();
  const auto sb = b.stops();

  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa.size()), static_cast<uint32_t>(sb.size()));
  for (size_t i = 0; i < sa.size(); ++i)
  {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa[i].index), static_cast<uint32_t>(sb[i].index));
    TEST_ASSERT_TRUE(sa[i].color == sb[i].color);
  }
}

static_assert(std::is_convertible<decltype(std::declval<const Palette&>().stops()), lw::span<const Stop>>::value, "Palette stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<Palette>::value, "Palette should satisfy IsPaletteLike");
static_assert(std::is_convertible<decltype(std::declval<const lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>&>().stops()), lw::span<const Stop>>::value,
              "RainbowPaletteGenerator stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>>::value, "RainbowPaletteGenerator must satisfy IsPaletteLike");
static_assert(std::is_convertible<decltype(std::declval<const lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>&>().stops()), lw::span<const Stop>>::value,
              "TemporalRainbowPaletteGenerator stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>>::value, "TemporalRainbowPaletteGenerator must satisfy IsPaletteLike");
static_assert(std::is_convertible<decltype(std::declval<const lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>&>().stops()), lw::span<const Stop>>::value,
              "RandomSmoothPaletteGenerator stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>>::value, "RandomSmoothPaletteGenerator must satisfy IsPaletteLike");
static_assert(std::is_convertible<decltype(std::declval<const lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>&>().stops()), lw::span<const Stop>>::value,
              "RandomCyclePaletteGenerator stops() must return a stop span");
static_assert(lw::colors::palettes::IsPaletteLike<lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>>::value, "RandomCyclePaletteGenerator must satisfy IsPaletteLike");
static_assert(Palette::AllowedSettings.size() == 0u, "Palette should expose an empty AllowedSettings descriptor");
static_assert(lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>::AllowedSettings.size() == 4u, "RainbowPaletteGenerator should expose four allowed settings");
static_assert(lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>::AllowedSettings.size() == 5u, "TemporalRainbowPaletteGenerator should expose five allowed settings");
static_assert(lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>::AllowedSettings.size() == 3u, "RandomSmoothPaletteGenerator should expose three allowed settings");
static_assert(lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>::AllowedSettings.size() == 3u, "RandomCyclePaletteGenerator should expose three allowed settings");

template <size_t N> void assert_allowed_settings_equal(const std::array<SettingDescriptor, N>& actual, const std::array<SettingDescriptor, N>& expected)
{
  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(expected.size()), static_cast<uint32_t>(actual.size()));
  for (size_t i = 0; i < expected.size(); ++i)
  {
    TEST_ASSERT_EQUAL_STRING(expected[i].key, actual[i].key);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(expected[i].valueType), static_cast<uint32_t>(actual[i].valueType));
  }
}

void test_rainbow_generator_stop_shape_and_update(void)
{
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow;
  rainbow.setStopCount(8);
  const auto before = rainbow.stops();

  TEST_ASSERT_EQUAL_UINT32(8, static_cast<uint32_t>(before.size()));
  for (size_t i = 0; i < before.size(); ++i)
  {
    const size_t expectedIndex = (i * 255u) / (before.size() - 1u);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(expectedIndex), static_cast<uint32_t>(before[i].index));
  }

  const lw::Rgb8Color firstBefore = before[0].color;
  rainbow.update(16);
  const auto after = rainbow.stops();

  TEST_ASSERT_TRUE(!(after[0].color == firstBefore));
}

void test_temporal_rainbow_generator_is_two_stop_uniform_palette(void)
{
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> generator;
  const auto stops = generator.stops();

  TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(stops.size()));
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(stops[0].index));
  TEST_ASSERT_EQUAL_UINT32(255, static_cast<uint32_t>(stops[1].index));
  TEST_ASSERT_TRUE(stops[0].color == stops[1].color);
}

void test_temporal_rainbow_generator_changes_uniform_color_over_time(void)
{
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> generator;
  const auto before = generator.stops();
  const lw::Rgb8Color firstBefore = before[0].color;

  for (int i = 0; i < 8; ++i)
  {
    generator.update();
  }

  const auto after = generator.stops();
  TEST_ASSERT_TRUE(after[0].color == after[1].color);
  TEST_ASSERT_TRUE(!(after[0].color == firstBefore));
}

void test_temporal_rainbow_generator_defaults_update_step_to_one(void)
{
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> implicitStep;
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> explicitStep;

  implicitStep.update();
  explicitStep.update(1);

  const auto implicitStops = implicitStep.stops();
  const auto explicitStops = explicitStep.stops();

  TEST_ASSERT_TRUE(implicitStops[0].color == explicitStops[0].color);
  TEST_ASSERT_TRUE(implicitStops[1].color == explicitStops[1].color);
}

void test_random_smooth_generator_is_deterministic(void)
{
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> a;
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> b;
  a.setStopCount(6);
  b.setStopCount(6);
  a.setSeed(12345u);
  b.setSeed(12345u);
  a.setProgressStep(20);
  b.setProgressStep(20);

  for (int i = 0; i < 12; ++i)
  {
    a.update();
    b.update();
  }

  const auto sa = a.stops();
  const auto sb = b.stops();

  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa.size()), static_cast<uint32_t>(sb.size()));
  for (size_t i = 0; i < sa.size(); ++i)
  {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa[i].index), static_cast<uint32_t>(sb[i].index));
    TEST_ASSERT_TRUE(sa[i].color == sb[i].color);
  }
}

void test_random_smooth_generator_changes_over_time(void)
{
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> generator;
  generator.setStopCount(6);
  generator.setSeed(999u);
  generator.setProgressStep(17);
  const auto before = generator.stops();
  const lw::Rgb8Color firstBefore = before[0].color;

  generator.update();
  generator.update();

  const auto after = generator.stops();
  TEST_ASSERT_TRUE(!(after[0].color == firstBefore));
}

void test_random_cycle_generator_is_deterministic(void)
{
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> a;
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> b;
  a.setStopCount(5);
  b.setStopCount(5);
  a.setSeed(42u);
  b.setSeed(42u);
  a.setCycleStep(32);
  b.setCycleStep(32);

  for (int i = 0; i < 16; ++i)
  {
    a.update();
    b.update();
  }

  const auto sa = a.stops();
  const auto sb = b.stops();

  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa.size()), static_cast<uint32_t>(sb.size()));
  for (size_t i = 0; i < sa.size(); ++i)
  {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(sa[i].index), static_cast<uint32_t>(sb[i].index));
    TEST_ASSERT_TRUE(sa[i].color == sb[i].color);
  }
}

void test_random_cycle_generator_rotates_and_samples(void)
{
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> generator;
  generator.setStopCount(5);
  generator.setSeed(77u);
  generator.setCycleStep(64);
  const auto beforeStopsView = generator.stops();
  std::array<Stop, 5> beforeStops{};
  for (size_t i = 0; i < beforeStops.size(); ++i)
  {
    beforeStops[i] = beforeStopsView[i];
  }
  for (int i = 0; i < 8; ++i)
  {
    generator.update();
  }

  const auto afterStops = generator.stops();

  bool anyStopChanged = false;
  for (size_t i = 0; i < afterStops.size(); ++i)
  {
    if (!(afterStops[i].color == beforeStops[i].color))
    {
      anyStopChanged = true;
      break;
    }
  }

  TEST_ASSERT_TRUE(anyStopChanged);
}

void test_generators_satisfy_palette_like_usage(void)
{
  const std::array<lw::colors::palettes::PaletteStop<lw::Rgb8Color>, 2> solidStops = {
      lw::colors::palettes::PaletteStop<lw::Rgb8Color>{0, lw::Rgb8Color(7, 8, 9)},
      lw::colors::palettes::PaletteStop<lw::Rgb8Color>{255, lw::Rgb8Color(7, 8, 9)},
  };
  const Palette solid(solidStops);
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow;
  rainbow.setStopCount(6);
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalRainbow;
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smooth;
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle;
  smooth.setStopCount(6);
  smooth.setSeed(1u);
  smooth.setProgressStep(25);
  cycle.setStopCount(6);
  cycle.setSeed(2u);
  cycle.setCycleStep(25);

  std::array<lw::Rgb8Color, 4> out{};
  lw::IndexRange paletteIndexes(0, 32, out.size());
  const size_t solidWritten = lw::colors::palettes::samplePalette(solid, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
  const size_t rainbowWritten = lw::colors::palettes::samplePalette(rainbow, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
  const size_t temporalRainbowWritten = lw::colors::palettes::samplePalette(temporalRainbow, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
  const size_t smoothWritten = lw::colors::palettes::samplePalette(smooth, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));
  const size_t cycleWritten = lw::colors::palettes::samplePalette(cycle, paletteIndexes, lw::span<lw::Rgb8Color>(out.data(), out.size()));

  TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(solidWritten));
  TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(rainbowWritten));
  TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(temporalRainbowWritten));
  TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(smoothWritten));
  TEST_ASSERT_EQUAL_UINT32(4, static_cast<uint32_t>(cycleWritten));
}

void test_palette_type_codes_are_initialized_and_unique_per_implementation(void)
{
  const Palette solid;
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow;
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalRainbow;
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smooth;
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle;

  TEST_ASSERT_NOT_EQUAL(0u, solid.typeCode());
  TEST_ASSERT_NOT_EQUAL(0u, rainbow.typeCode());
  TEST_ASSERT_NOT_EQUAL(0u, temporalRainbow.typeCode());
  TEST_ASSERT_NOT_EQUAL(0u, smooth.typeCode());
  TEST_ASSERT_NOT_EQUAL(0u, cycle.typeCode());

  TEST_ASSERT_EQUAL_UINT32(Palette::TypeCode, solid.typeCode());
  TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>::TypeCode, rainbow.typeCode());
  TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>::TypeCode, temporalRainbow.typeCode());
  TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>::TypeCode, smooth.typeCode());
  TEST_ASSERT_EQUAL_UINT32(lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>::TypeCode, cycle.typeCode());

  TEST_ASSERT_NOT_EQUAL(solid.typeCode(), rainbow.typeCode());
  TEST_ASSERT_NOT_EQUAL(solid.typeCode(), temporalRainbow.typeCode());
  TEST_ASSERT_NOT_EQUAL(solid.typeCode(), smooth.typeCode());
  TEST_ASSERT_NOT_EQUAL(solid.typeCode(), cycle.typeCode());
  TEST_ASSERT_NOT_EQUAL(rainbow.typeCode(), temporalRainbow.typeCode());
  TEST_ASSERT_NOT_EQUAL(rainbow.typeCode(), smooth.typeCode());
  TEST_ASSERT_NOT_EQUAL(rainbow.typeCode(), cycle.typeCode());
  TEST_ASSERT_NOT_EQUAL(temporalRainbow.typeCode(), smooth.typeCode());
  TEST_ASSERT_NOT_EQUAL(temporalRainbow.typeCode(), cycle.typeCode());
  TEST_ASSERT_NOT_EQUAL(smooth.typeCode(), cycle.typeCode());
}

void test_generator_sync_copies_internal_state_for_same_type(void)
{
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbowSource;
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbowTarget;
  rainbowSource.setStopCount(6);
  rainbowSource.setSaturation(204u);
  rainbowSource.setBrightness(128u);
  rainbowSource.setHueOffset(17u);
  rainbowTarget.setStopCount(10);
  rainbowTarget.setSaturation(255u);
  rainbowTarget.setBrightness(255u);
  rainbowTarget.setHueOffset(99u);
  rainbowSource.update(13);
  rainbowTarget.update(7);
  rainbowSource.syncTo(&rainbowTarget);
  assert_same_stops(rainbowSource, rainbowTarget);
  rainbowSource.update(5);
  rainbowTarget.update(5);
  assert_same_stops(rainbowSource, rainbowTarget);

  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalSource;
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporalTarget;
  temporalSource.update(23);
  temporalTarget.update(2);
  temporalSource.syncTo(&temporalTarget);
  assert_same_stops(temporalSource, temporalTarget);
  temporalSource.update(9);
  temporalTarget.update(9);
  assert_same_stops(temporalSource, temporalTarget);

  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smoothSource;
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smoothTarget;
  smoothSource.setStopCount(6);
  smoothSource.setSeed(111u);
  smoothSource.setProgressStep(19);
  smoothTarget.setStopCount(4);
  smoothTarget.setSeed(222u);
  smoothTarget.setProgressStep(5);
  smoothSource.update();
  smoothSource.update();
  smoothTarget.update();
  smoothSource.syncTo(&smoothTarget);
  assert_same_stops(smoothSource, smoothTarget);
  smoothSource.update();
  smoothTarget.update();
  assert_same_stops(smoothSource, smoothTarget);

  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycleSource;
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycleTarget;
  cycleSource.setStopCount(5);
  cycleSource.setSeed(333u);
  cycleSource.setCycleStep(41);
  cycleTarget.setStopCount(7);
  cycleTarget.setSeed(444u);
  cycleTarget.setCycleStep(9);
  cycleSource.update();
  cycleSource.update();
  cycleTarget.update();
  cycleSource.syncTo(&cycleTarget);
  assert_same_stops(cycleSource, cycleTarget);
  cycleSource.update();
  cycleTarget.update();
  assert_same_stops(cycleSource, cycleTarget);
}

void test_generator_sync_ignores_mismatched_type_and_null_target(void)
{
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow;
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle;
  rainbow.setStopCount(6);
  rainbow.setSaturation(230u);
  rainbow.setBrightness(179u);
  rainbow.setHueOffset(33u);
  cycle.setStopCount(6);
  cycle.setSeed(555u);
  cycle.setCycleStep(12);

  rainbow.update(4);
  const auto before = cycle.stops();
  std::array<Stop, 6> beforeStops{};
  for (size_t i = 0; i < beforeStops.size(); ++i)
  {
    beforeStops[i] = before[i];
  }

  rainbow.syncTo(nullptr);
  rainbow.syncTo(&cycle);

  const auto after = cycle.stops();
  for (size_t i = 0; i < beforeStops.size(); ++i)
  {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(beforeStops[i].index), static_cast<uint32_t>(after[i].index));
    TEST_ASSERT_TRUE(beforeStops[i].color == after[i].color);
  }
}

void test_palette_update_settings_defaults_to_noop_for_static_palettes(void)
{
  const std::array<Stop, 2> staticStops = {
      Stop{0, lw::Rgb8Color(12, 34, 56)},
      Stop{255, lw::Rgb8Color(12, 34, 56)},
  };
  Palette palette(staticStops);
  lw::colors::palettes::IPalette<lw::Rgb8Color>* interfacePalette = &palette;
  const std::array<PaletteSetting, 2> settings = {
      PaletteSetting{"stop_count", "9"},
      PaletteSetting{"seed", "1234"},
  };

  interfacePalette->updateSettings(lw::span<const PaletteSetting>(settings.data(), settings.size()));

  const auto stops = palette.stops();
  TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(stops.size()));
  TEST_ASSERT_EQUAL_UINT32(0, static_cast<uint32_t>(stops[0].index));
  TEST_ASSERT_EQUAL_UINT32(255, static_cast<uint32_t>(stops[1].index));
  TEST_ASSERT_TRUE(stops[0].color == lw::Rgb8Color(12, 34, 56));
  TEST_ASSERT_TRUE(stops[1].color == lw::Rgb8Color(12, 34, 56));
}

void test_rainbow_generator_update_settings_applies_snake_case_keys(void)
{
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> rainbow;
  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> expected;
  lw::colors::palettes::IPalette<lw::Rgb8Color>* interfacePalette = &rainbow;
  const std::array<PaletteSetting, 4> settings = {
      PaletteSetting{"stop_count", "5"},
      PaletteSetting{"saturation", "111"},
      PaletteSetting{"brightness", "203"},
      PaletteSetting{"hue_offset", "17"},
  };

  interfacePalette->updateSettings(lw::span<const PaletteSetting>(settings.data(), settings.size()));

  expected.setStopCount(5);
  expected.setSaturation(111);
  expected.setBrightness(203);
  expected.setHueOffset(17);
  assert_same_stops(rainbow, expected);
}

void test_temporal_rainbow_update_settings_applies_snake_case_keys(void)
{
  lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color> temporal;
  lw::colors::palettes::IPalette<lw::Rgb8Color>* interfacePalette = &temporal;
  const std::array<PaletteSetting, 5> settings = {
      PaletteSetting{"stop_count", "7"}, PaletteSetting{"saturation", "128"}, PaletteSetting{"brightness", "200"}, PaletteSetting{"hue_offset", "29"}, PaletteSetting{"step_index", "11"},
  };

  interfacePalette->updateSettings(lw::span<const PaletteSetting>(settings.data(), settings.size()));

  lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color> expectedRainbow;
  expectedRainbow.setStopCount(7);
  expectedRainbow.setSaturation(128);
  expectedRainbow.setBrightness(200);
  expectedRainbow.setHueOffset(29);

  lw::colors::palettes::PaletteSampleOptions<lw::Rgb8Color> sampleOpts{};
  sampleOpts.wrapMode = lw::colors::palettes::WrapMode::Circular;
  sampleOpts.blendMode = lw::colors::palettes::BlendMode::Linear;
  sampleOpts.quantizedLevels = 0;
  const lw::Rgb8Color expectedColor = lw::colors::palettes::samplePaletteAt<lw::Rgb8Color>(expectedRainbow.stops(), 11, sampleOpts);
  const auto stops = temporal.stops();

  TEST_ASSERT_EQUAL_UINT32(2, static_cast<uint32_t>(stops.size()));
  TEST_ASSERT_TRUE(stops[0].color == expectedColor);
  TEST_ASSERT_TRUE(stops[1].color == expectedColor);
}

void test_random_generators_update_settings_applies_snake_case_keys(void)
{
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> smooth;
  lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color> expectedSmooth;
  lw::colors::palettes::IPalette<lw::Rgb8Color>* smoothInterface = &smooth;
  const std::array<PaletteSetting, 3> smoothSettings = {
      PaletteSetting{"stop_count", "6"},
      PaletteSetting{"seed", "2468"},
      PaletteSetting{"progress_step", "21"},
  };

  smoothInterface->updateSettings(lw::span<const PaletteSetting>(smoothSettings.data(), smoothSettings.size()));
  expectedSmooth.setStopCount(6);
  expectedSmooth.setSeed(2468u);
  expectedSmooth.setProgressStep(21);
  smooth.update();
  expectedSmooth.update();
  assert_same_stops(smooth, expectedSmooth);

  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> cycle;
  lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color> expectedCycle;
  lw::colors::palettes::IPalette<lw::Rgb8Color>* cycleInterface = &cycle;
  const std::array<PaletteSetting, 3> cycleSettings = {
      PaletteSetting{"stop_count", "6"},
      PaletteSetting{"seed", "1357"},
      PaletteSetting{"cycle_step", "34"},
  };

  cycleInterface->updateSettings(lw::span<const PaletteSetting>(cycleSettings.data(), cycleSettings.size()));
  expectedCycle.setStopCount(6);
  expectedCycle.setSeed(1357u);
  expectedCycle.setCycleStep(34);
  cycle.update();
  expectedCycle.update();
  assert_same_stops(cycle, expectedCycle);
}

void test_palette_implementations_expose_allowed_settings_descriptors(void)
{
  constexpr std::array<SettingDescriptor, 4> expectedRainbow{{
      SettingDescriptor{"stop_count", SettingValueType::UnsignedSize},
      SettingDescriptor{"saturation", SettingValueType::UnsignedColorComponent},
      SettingDescriptor{"brightness", SettingValueType::UnsignedColorComponent},
      SettingDescriptor{"hue_offset", SettingValueType::UnsignedColorComponent},
  }};
  constexpr std::array<SettingDescriptor, 5> expectedTemporal{{
      SettingDescriptor{"stop_count", SettingValueType::UnsignedSize},
      SettingDescriptor{"saturation", SettingValueType::UnsignedColorComponent},
      SettingDescriptor{"brightness", SettingValueType::UnsignedColorComponent},
      SettingDescriptor{"hue_offset", SettingValueType::UnsignedColorComponent},
      SettingDescriptor{"step_index", SettingValueType::UnsignedSize},
  }};
  constexpr std::array<SettingDescriptor, 3> expectedSmooth{{
      SettingDescriptor{"stop_count", SettingValueType::UnsignedSize},
      SettingDescriptor{"seed", SettingValueType::UInt32},
      SettingDescriptor{"progress_step", SettingValueType::UInt8},
  }};
  constexpr std::array<SettingDescriptor, 3> expectedCycle{{
      SettingDescriptor{"stop_count", SettingValueType::UnsignedSize},
      SettingDescriptor{"seed", SettingValueType::UInt32},
      SettingDescriptor{"cycle_step", SettingValueType::UInt8},
  }};

  TEST_ASSERT_EQUAL_UINT32(0u, static_cast<uint32_t>(Palette::AllowedSettings.size()));
  assert_allowed_settings_equal(lw::colors::palettes::RainbowPaletteGenerator<lw::Rgb8Color>::AllowedSettings, expectedRainbow);
  assert_allowed_settings_equal(lw::colors::palettes::TemporalRainbowPaletteGenerator<lw::Rgb8Color>::AllowedSettings, expectedTemporal);
  assert_allowed_settings_equal(lw::colors::palettes::RandomSmoothPaletteGenerator<lw::Rgb8Color>::AllowedSettings, expectedSmooth);
  assert_allowed_settings_equal(lw::colors::palettes::RandomCyclePaletteGenerator<lw::Rgb8Color>::AllowedSettings, expectedCycle);
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
  RUN_TEST(test_rainbow_generator_stop_shape_and_update);
  RUN_TEST(test_temporal_rainbow_generator_is_two_stop_uniform_palette);
  RUN_TEST(test_temporal_rainbow_generator_changes_uniform_color_over_time);
  RUN_TEST(test_temporal_rainbow_generator_defaults_update_step_to_one);
  RUN_TEST(test_random_smooth_generator_is_deterministic);
  RUN_TEST(test_random_smooth_generator_changes_over_time);
  RUN_TEST(test_random_cycle_generator_is_deterministic);
  RUN_TEST(test_random_cycle_generator_rotates_and_samples);
  RUN_TEST(test_generators_satisfy_palette_like_usage);
  RUN_TEST(test_palette_type_codes_are_initialized_and_unique_per_implementation);
  RUN_TEST(test_generator_sync_copies_internal_state_for_same_type);
  RUN_TEST(test_generator_sync_ignores_mismatched_type_and_null_target);
  RUN_TEST(test_palette_update_settings_defaults_to_noop_for_static_palettes);
  RUN_TEST(test_rainbow_generator_update_settings_applies_snake_case_keys);
  RUN_TEST(test_palette_implementations_expose_allowed_settings_descriptors);
  RUN_TEST(test_temporal_rainbow_update_settings_applies_snake_case_keys);
  RUN_TEST(test_random_generators_update_settings_applies_snake_case_keys);
  return UNITY_END();
}
