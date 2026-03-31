#include <unity.h>

#include <algorithm>
#include <limits>
#include <vector>

#include "buses/PixelBus.h"
#include "colors/ColorMath.h"
#include "colors/Color.h"
#include "colors/IShader.h"
#include "protocols/IProtocol.h"
#include "transports/ITransport.h"

namespace
{
using TestColor = lw::Rgb8Color;

struct MockProtocolSettings : public lw::protocols::ProtocolSettings
{
    uint8_t fillByte{0xA5};
};

class MockProtocol : public lw::protocols::IProtocol<TestColor>
{
  public:
    using SettingsType = MockProtocolSettings;

    static size_t requiredBufferSize(uint16_t pixelCount, const SettingsType&)
    {
        return static_cast<size_t>(pixelCount) + 3U;
    }

    MockProtocol(uint16_t pixelCount, SettingsType settings)
        : lw::protocols::IProtocol<TestColor>(pixelCount), _settings(std::move(settings)),
          _required(requiredBufferSize(pixelCount, _settings))
    {
    }

    void begin() override { began = true; }

    void update(lw::span<const TestColor> colors, lw::span<uint8_t> buffer = lw::span<uint8_t>{}) override
    {
        lastSource = colors.data();
        captured.assign(colors.begin(), colors.end());
        if (buffer.size() >= _required)
        {
            std::fill(buffer.begin(), buffer.begin() + _required, _settings.fillByte);
        }
        lastBuffer = buffer.data();
        lastBufferSize = buffer.size();
    }

    lw::protocols::ProtocolSettings& settings() override { return _settings; }

    bool alwaysUpdate() const override { return false; }

    size_t requiredBufferSizeBytes() const override { return _required; }

    bool began{false};
    const TestColor* lastSource{nullptr};
    const uint8_t* lastBuffer{nullptr};
    size_t lastBufferSize{0};
    std::vector<TestColor> captured{};

  private:
    SettingsType _settings{};
    size_t _required{0};
};

struct MockTransportSettings
{
    bool invert{false};
};

class MockTransport : public lw::transports::ITransport
{
  public:
    using TransportSettingsType = MockTransportSettings;

    explicit MockTransport(TransportSettingsType settings) : _settings(settings) {}

    void begin() override { began = true; }

    void beginTransaction() override { ++beginTransactionCount; }

    void transmitBytes(lw::span<uint8_t> data) override
    {
        transmitted.assign(data.begin(), data.end());
        lastBrightness = {};
    }

    void transmitBytes(lw::span<uint8_t> data, lw::transports::TransportBrightness brightness) override
    {
        transmitted.assign(data.begin(), data.end());
        lastBrightness = brightness;
    }

    void endTransaction() override { ++endTransactionCount; }

    bool isReadyToUpdate() const override { return true; }

    bool began{false};
    size_t beginTransactionCount{0};
    size_t endTransactionCount{0};
    std::vector<uint8_t> transmitted{};
    lw::transports::TransportBrightness lastBrightness{};

  private:
    TransportSettingsType _settings{};
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

void test_constructor_manages_internal_typed_buffers_and_runs_pipeline(void)
{
    MockProtocolSettings protocolSettings{};
    protocolSettings.fillByte = 0x3C;

    lw::busses::PixelBus<MockProtocol, MockTransport, IncrementRedShader> bus(
        3, protocolSettings, MockTransportSettings{}, IncrementRedShader{});

    auto& root = bus.pixels();
    root[0] = TestColor{1, 2, 3};
    root[1] = TestColor{4, 5, 6};
    root[2] = TestColor{7, 8, 9};

    bus.begin();
    bus.show();

    const auto rootAfter = bus.rootPixels();
    const auto shaderScratch = bus.shaderScratch();
    const auto protocolBytes = bus.protocolBuffer();

    TEST_ASSERT_TRUE(bus.protocol().began);
    TEST_ASSERT_TRUE(bus.transport().began);

    TEST_ASSERT_TRUE(rootAfter.data() != shaderScratch.data());
    TEST_ASSERT_TRUE(reinterpret_cast<const void*>(rootAfter.data()) !=
                     reinterpret_cast<const void*>(protocolBytes.data()));
    TEST_ASSERT_TRUE(reinterpret_cast<const void*>(shaderScratch.data()) !=
                     reinterpret_cast<const void*>(protocolBytes.data()));

    TEST_ASSERT_EQUAL_UINT8(1, rootAfter[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(4, rootAfter[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(7, rootAfter[2]['R']);

    TEST_ASSERT_EQUAL_UINT8(2, shaderScratch[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(5, shaderScratch[1]['R']);
    TEST_ASSERT_EQUAL_UINT8(8, shaderScratch[2]['R']);

    TEST_ASSERT_TRUE(bus.protocol().lastSource == shaderScratch.data());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(protocolBytes.size()),
                             static_cast<uint32_t>(bus.protocol().lastBufferSize));

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(protocolBytes.size()),
                             static_cast<uint32_t>(bus.transport().transmitted.size()));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.transport().beginTransactionCount));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.transport().endTransactionCount));

    for (size_t index = 0; index < protocolBytes.size(); ++index)
    {
        TEST_ASSERT_EQUAL_HEX8(0x3C, protocolBytes[index]);
    }
}

void test_nil_shader_constructor_keeps_scratch_empty_and_uses_root_directly(void)
{
    MockProtocolSettings protocolSettings{};
    protocolSettings.fillByte = 0x5A;

    lw::busses::PixelBus<MockProtocol, MockTransport> bus(2, protocolSettings, MockTransportSettings{});

    auto& root = bus.pixels();
    root[0] = TestColor{10, 11, 12};
    root[1] = TestColor{20, 21, 22};

    bus.begin();
    bus.show();

    const auto rootAfter = bus.rootPixels();
    const auto shaderScratch = bus.shaderScratch();
    const auto protocolBytes = bus.protocolBuffer();

    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(shaderScratch.size()));
    TEST_ASSERT_TRUE(bus.protocol().lastSource == rootAfter.data());

    TEST_ASSERT_EQUAL_UINT8(10, rootAfter[0]['R']);
    TEST_ASSERT_EQUAL_UINT8(20, rootAfter[1]['R']);

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(protocolBytes.size()),
                             static_cast<uint32_t>(bus.transport().transmitted.size()));

    for (size_t index = 0; index < protocolBytes.size(); ++index)
    {
        TEST_ASSERT_EQUAL_HEX8(0x5A, protocolBytes[index]);
    }
}

void test_platform_default_transport_type_constructs_and_updates(void)
{
    MockProtocolSettings protocolSettings{};
    protocolSettings.fillByte = 0x7E;

    lw::busses::PixelBus<MockProtocol> bus(2, protocolSettings, lw::busses::PlatformDefaultTransportSettings{});

    auto& root = bus.pixels();
    root[0] = TestColor{3, 4, 5};
    root[1] = TestColor{6, 7, 8};

    bus.begin();
    bus.show();

    const auto protocolBytes = bus.protocolBuffer();
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(bus.shaderScratch().size()));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(protocolBytes.size()),
                             static_cast<uint32_t>(bus.protocol().lastBufferSize));

    for (size_t index = 0; index < protocolBytes.size(); ++index)
    {
        TEST_ASSERT_EQUAL_HEX8(0x7E, protocolBytes[index]);
    }
}

void test_platform_default_transport_template_default_constructs(void)
{
    MockProtocolSettings protocolSettings{};
    protocolSettings.fillByte = 0x33;

    lw::busses::PixelBus<MockProtocol> bus(1, protocolSettings, lw::busses::PlatformDefaultTransportSettings{});

    auto& root = bus.pixels();
    root[0] = TestColor{9, 1, 2};

    bus.begin();
    bus.show();

    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(bus.shaderScratch().size()));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(bus.protocolBuffer().size()),
                             static_cast<uint32_t>(bus.protocol().lastBufferSize));
}

void test_pixel_bus_scales_before_protocol_and_marks_transport_context(void)
{
    MockProtocolSettings protocolSettings{};
    protocolSettings.fillByte = 0x55;

    lw::busses::PixelBus<MockProtocol, MockTransport> bus(1, protocolSettings, MockTransportSettings{});
    bus.setBrightness(static_cast<TestColor::ComponentType>(64));

    auto& root = bus.pixels();
    root[0] = TestColor{120, 20, 10};

    bus.show();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(lw::colors::applyBrightness<uint8_t, uint8_t>(120, 64)), bus.protocol().captured[0]['R']);
    TEST_ASSERT_EQUAL_UINT32(64U, bus.transport().lastBrightness.value);
    TEST_ASSERT_EQUAL_UINT32(255U, bus.transport().lastBrightness.max);
    TEST_ASSERT_TRUE(bus.transport().lastBrightness.upstreamApplied);
}

void test_pixel_bus_shader_owned_brightness_scales_once_and_marks_transport_context(void)
{
    MockProtocolSettings protocolSettings{};

    lw::busses::PixelBus<MockProtocol, MockTransport, BrightnessOwnerShader> bus(
        1, protocolSettings, MockTransportSettings{}, BrightnessOwnerShader{});
    bus.setBrightness(static_cast<TestColor::ComponentType>(64));

    auto& root = bus.pixels();
    root[0] = TestColor{120, 20, 10};

    bus.show();

    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.shader().applyCount));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(bus.shader().brightnessApplyCount));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(lw::colors::applyBrightness<uint8_t, uint8_t>(121, 64)), bus.protocol().captured[0]['R']);
    TEST_ASSERT_EQUAL_UINT32(64U, bus.transport().lastBrightness.value);
    TEST_ASSERT_TRUE(bus.transport().lastBrightness.upstreamApplied);
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
    RUN_TEST(test_constructor_manages_internal_typed_buffers_and_runs_pipeline);
    RUN_TEST(test_nil_shader_constructor_keeps_scratch_empty_and_uses_root_directly);
    RUN_TEST(test_platform_default_transport_type_constructs_and_updates);
    RUN_TEST(test_platform_default_transport_template_default_constructs);
    RUN_TEST(test_pixel_bus_scales_before_protocol_and_marks_transport_context);
    RUN_TEST(test_pixel_bus_shader_owned_brightness_scales_once_and_marks_transport_context);
    return UNITY_END();
}
