#include <unity.h>

#include <limits>

#include "buses/LightBus.h"
#include "colors/Color.h"
#include "colors/ColorMath.h"
#include "colors/IShader.h"
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

    void write(const TestColor& color) override
    {
        write(color, std::numeric_limits<BrightnessType>::max());
    }

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

class IncrementRedShader : public lw::IShader<TestColor>
{
  public:
    void apply(lw::span<TestColor> colors) override
    {
        for (auto& color : colors)
        {
            ++color['R'];
        }
    }
};

class BrightnessOwnerShader : public lw::IShader<TestColor>
{
  public:
    using BrightnessType = typename lw::IShader<TestColor>::BrightnessType;

    void apply(lw::span<TestColor> colors) override
    {
        ++applyCount;
        for (auto& color : colors)
        {
            ++color['R'];
        }
    }

    lw::shaders::BrightnessOwnership brightnessOwnership() const override { return lw::shaders::BrightnessOwnership::Owns; }

    void applyBrightness(lw::span<TestColor> colors, BrightnessType brightness) override
    {
        ++brightnessApplyCount;
        for (auto& color : colors)
        {
            for (auto channel : TestColor::channelIndexes())
            {
                color[channel] = static_cast<TestColor::ComponentType>(lw::colors::applyBrightness(color[channel], brightness));
            }
        }
    }

    size_t applyCount{0};
    size_t brightnessApplyCount{0};
};

class ConflictBrightnessShader : public lw::IShader<TestColor>
{
  public:
    using BrightnessType = typename lw::IShader<TestColor>::BrightnessType;

    void apply(lw::span<TestColor> colors) override
    {
        for (auto& color : colors)
        {
            ++color['R'];
        }
    }

    lw::shaders::BrightnessOwnership brightnessOwnership() const override { return lw::shaders::BrightnessOwnership::Conflict; }

    void applyBrightness(lw::span<TestColor>, BrightnessType) override { ++brightnessApplyCount; }

    size_t brightnessApplyCount{0};
};

void test_light_bus_applies_shader_and_preserves_root_pixel(void)
{
    lw::busses::LightBus<TestColor, MockLightDriver, IncrementRedShader> bus(MockLightDriver::LightDriverSettingsType{},
                                                                             IncrementRedShader{});

    bus.begin();
    auto& pixels = bus.pixels();
    pixels[0] = TestColor{3, 4, 5};
    bus.show();

    TEST_ASSERT_TRUE(bus.driver().began);
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.driver().writeCount));
    TEST_ASSERT_EQUAL_UINT8(4, bus.driver().lastColor['R']);
    TEST_ASSERT_EQUAL_UINT8(3, bus.pixel()['R']);

    const auto scratch = bus.shaderScratch();
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(scratch.size()));
    TEST_ASSERT_EQUAL_UINT8(4, scratch[0]['R']);
}

void test_light_bus_nil_shader_writes_root_and_uses_dirty_guard(void)
{
    lw::busses::LightBus<TestColor, MockLightDriver> bus(MockLightDriver::LightDriverSettingsType{});

    bus.pixel() = TestColor{9, 1, 2};
    bus.show();
    bus.show();

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.driver().writeCount));
    TEST_ASSERT_EQUAL_UINT8(9, bus.driver().lastColor['R']);

    const auto scratch = bus.shaderScratch();
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(scratch.size()));
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

void test_light_bus_shader_owned_brightness_uses_full_scale_downstream(void)
{
    lw::busses::LightBus<TestColor, MockLightDriver, BrightnessOwnerShader> bus(
        MockLightDriver::LightDriverSettingsType{}, BrightnessOwnerShader{});

    bus.setBrightness(static_cast<TestColor::ComponentType>(64));
    bus.pixel() = TestColor{120, 20, 10};
    bus.show();

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.shader().applyCount));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.shader().brightnessApplyCount));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(lw::colors::applyBrightness<uint8_t, uint8_t>(121, 64)), bus.driver().lastColor['R']);
    TEST_ASSERT_EQUAL_UINT8(std::numeric_limits<TestColor::ComponentType>::max(), bus.driver().lastBrightness);
    TEST_ASSERT_EQUAL_UINT8(120, bus.pixel()['R']);
}

void test_light_bus_conflicted_shader_brightness_falls_back_to_driver(void)
{
    lw::busses::LightBus<TestColor, MockLightDriver, ConflictBrightnessShader> bus(
        MockLightDriver::LightDriverSettingsType{}, ConflictBrightnessShader{});

    bus.setBrightness(static_cast<TestColor::ComponentType>(32));
    bus.pixel() = TestColor{10, 0, 0};
    bus.show();

    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(bus.shader().brightnessApplyCount));
    TEST_ASSERT_EQUAL_UINT8(11, bus.driver().lastColor['R']);
    TEST_ASSERT_EQUAL_UINT8(32, bus.driver().lastBrightness);
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
    RUN_TEST(test_light_bus_applies_shader_and_preserves_root_pixel);
    RUN_TEST(test_light_bus_nil_shader_writes_root_and_uses_dirty_guard);
    RUN_TEST(test_light_bus_checks_driver_readiness);
    RUN_TEST(test_light_bus_passes_brightness_to_driver_write);
    RUN_TEST(test_light_bus_shader_owned_brightness_uses_full_scale_downstream);
    RUN_TEST(test_light_bus_conflicted_shader_brightness_falls_back_to_driver);
    return UNITY_END();
}
