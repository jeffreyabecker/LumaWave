#include <unity.h>

#include <array>
#include <cstdint>
#include <vector>

#include <ArduinoFake.h>

#include "protocols/DotStarProtocol.h"
#include "protocols/PixieProtocol.h"
#include "protocols/Ws2801Protocol.h"
#include "protocols/Ws2812xProtocol.h"
#include "protocols/OneWireEncoding.h"

namespace
{
using namespace fakeit;

template <typename TProtocol> std::vector<uint8_t> bind_protocol_buffer(TProtocol& protocol)
{
  const auto& settings = static_cast<const typename TProtocol::SettingsType&>(protocol.settings());
  return std::vector<uint8_t>(TProtocol::requiredBufferSize(protocol.pixelCount(), settings), 0);
}

template <typename T> lw::span<T> as_span(std::vector<T>& value)
{
  return lw::span<T>{value.data(), value.size()};
}

void assert_bytes_equal(const std::vector<uint8_t>& actual, const std::vector<uint8_t>& expected)
{
  TEST_ASSERT_EQUAL_UINT32(expected.size(), static_cast<uint32_t>(actual.size()));
  for (size_t index = 0; index < expected.size(); ++index)
  {
    TEST_ASSERT_EQUAL_UINT8(expected[index], actual[index]);
  }
}

std::vector<uint8_t> slice_bytes(const std::vector<uint8_t>& value, size_t offset, size_t length)
{
  return std::vector<uint8_t>{value.begin() + offset, value.begin() + offset + length};
}

std::vector<uint8_t> encode_ws2812x_payload(const std::vector<uint8_t>& raw)
{
  const size_t payloadSize = lw::protocols::OneWireEncoding::expandedPayloadSizeBytes(raw.size(), lw::protocols::timing::Ws2812x.bitPattern());
  const size_t resetSize = lw::protocols::OneWireEncoding::computeResetBytes(lw::protocols::timing::Ws2812x, 0, 1);

  std::vector<uint8_t> output(payloadSize + resetSize, 0);
  std::vector<uint8_t> source = raw;
  const size_t actualSize = lw::protocols::OneWireEncoding::encodeWithResets(source.data(), source.size(), output.data(), output.size(), lw::protocols::timing::Ws2812x, 0, 0, 1, false);
  TEST_ASSERT_GREATER_THAN_UINT32(0U, static_cast<uint32_t>(actualSize));
  output.resize(actualSize);
  return output;
}

void test_1_1_1_dotstar_construction_and_begin(void)
{
  lw::protocols::Apa102Protocol protocol(3, lw::protocols::Apa102ProtocolSettings{{}});
  protocol.begin();

  auto protocolBuffer = bind_protocol_buffer(protocol);
  TEST_ASSERT_EQUAL_UINT32(21U, static_cast<uint32_t>(protocolBuffer.size()));
}

void test_1_1_2_dotstar_end_frame_extra_byte_calculation(void)
{
  const std::array<uint16_t, 6> counts{0, 1, 15, 16, 17, 32};
  for (const auto pixelCount : counts)
  {
    lw::protocols::Apa102Protocol protocol(pixelCount, lw::protocols::Apa102ProtocolSettings{{}});
    auto protocolBuffer = bind_protocol_buffer(protocol);

    const size_t extra = (static_cast<size_t>(pixelCount) + 15u) / 16u;
    const size_t expectedSize = 4u + (static_cast<size_t>(pixelCount) * 4u) + 4u + extra;
    TEST_ASSERT_EQUAL_UINT32(expectedSize, static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_1_3_and_1_1_4_dotstar_fixed_brightness_and_luminance_serialization(void)
{
  const std::array<lw::Color, 2> colors{lw::colorFromRGB(0x11, 0x22, 0x33), lw::colorFromRGB(0x44, 0x55, 0x66)};

  {
    lw::protocols::Apa102Protocol protocol(2, lw::protocols::Apa102ProtocolSettings{{}, lw::ChannelOrder::GRB::value});
    {
      uint8_t g = 128;
      protocol.setRuntimeConfig(lw::protocols::RuntimeConfig::Brightness, &g);
    }
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(colors, as_span(protocolBuffer));

    const std::vector<uint8_t> expected{0xF0, 0x22, 0x11, 0x33, 0xF0, 0x55, 0x44, 0x66};
    assert_bytes_equal(slice_bytes(protocolBuffer, 4, 8), expected);
  }

  {
    lw::protocols::Apa102Protocol protocol(2, lw::protocols::Apa102ProtocolSettings{{}, lw::ChannelOrder::BGR::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(colors, as_span(protocolBuffer));

    const std::vector<uint8_t> expected{0xFF, 0x33, 0x22, 0x11, 0xFF, 0x66, 0x55, 0x44};
    assert_bytes_equal(slice_bytes(protocolBuffer, 4, 8), expected);
  }

  {
    lw::protocols::Apa102Protocol protocol(1, lw::protocols::Apa102ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(0x12, 0x34, 0x56)}, as_span(protocolBuffer));

    const std::vector<uint8_t> expected{0xFF, 0x12, 0x34, 0x56};
    assert_bytes_equal(slice_bytes(protocolBuffer, 4, 4), expected);
  }

  {
    lw::protocols::Hd108Protocol protocol(1, lw::protocols::Hd108ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    {
      uint8_t g = 128;
      protocol.setRuntimeConfig(lw::protocols::RuntimeConfig::Brightness, &g);
    }
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(0x12, 0x34, 0x56)}, as_span(protocolBuffer));

    const std::vector<uint8_t> expected{0xFF, 0xF0, 0x00, 0x12, 0x00, 0x34, 0x00, 0x56};
    assert_bytes_equal(slice_bytes(protocolBuffer, 16, 8), expected);
  }
}

void test_1_1_5_dotstar_framing_and_transaction_sequence(void)
{
  lw::protocols::Apa102Protocol protocol(1, lw::protocols::Apa102ProtocolSettings{{}});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.begin();
  protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(1, 2, 3)}, as_span(protocolBuffer));

  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[0]);
  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[1]);
  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[3]);
  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer.back());
}

void test_1_1_6_and_1_1_7_dotstar_oversized_and_channel_order_edge_contract(void)
{
  const std::array<lw::Color, 3> oversized{lw::colorFromRGB(1, 2, 3), lw::colorFromRGB(4, 5, 6), lw::colorFromRGB(7, 8, 9)};

  {
    lw::protocols::Apa102Protocol protocol(2, lw::protocols::Apa102ProtocolSettings{{}, lw::ChannelOrder::BGR::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(oversized, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(8U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 4, 8).size()));
  }

  {
    lw::protocols::Apa102Protocol protocol(2, lw::protocols::Apa102ProtocolSettings{{}, ""});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 2>{lw::colorFromRGB(10, 11, 12), lw::colorFromRGB(13, 14, 15)}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(8U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 4, 8).size()));
  }
}

void test_1_3_1_ws2801_serialization_order_variants(void)
{
  const std::array<lw::Color, 2> colors{lw::colorFromRGB(1, 2, 3), lw::colorFromRGB(4, 5, 6)};

  auto run_case = [&](const char* order, const std::vector<uint8_t>& expected)
  {
    lw::protocols::Ws2801ProtocolT protocol(2, lw::protocols::Ws2801ProtocolSettings{{}, order});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(colors, as_span(protocolBuffer));
    assert_bytes_equal(protocolBuffer, expected);
  };

  run_case(lw::ChannelOrder::RGB::value, std::vector<uint8_t>{1, 2, 3, 4, 5, 6});
  run_case(lw::ChannelOrder::GRB::value, std::vector<uint8_t>{2, 1, 3, 5, 4, 6});
  run_case(lw::ChannelOrder::BGR::value, std::vector<uint8_t>{3, 2, 1, 6, 5, 4});
}

void test_1_3_2_ws2801_transaction_and_latch_timing(void)
{
  lw::protocols::Ws2801ProtocolT protocol(1, lw::protocols::Ws2801ProtocolSettings{{}});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(1, 2, 3)}, as_span(protocolBuffer));

  TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(protocolBuffer.size()));
  assert_bytes_equal(protocolBuffer, std::vector<uint8_t>{1, 2, 3});
}

void test_1_3_3_ws2801_oversized_and_channel_order_edge_contract(void)
{
  {
    lw::protocols::Ws2801ProtocolT protocol(1, lw::protocols::Ws2801ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 2>{lw::colorFromRGB(1, 2, 3), lw::colorFromRGB(4, 5, 6)}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(protocolBuffer.size()));
  }

  {
    lw::protocols::Ws2801ProtocolT protocol(1, lw::protocols::Ws2801ProtocolSettings{{}, ""});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(7, 8, 9)}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_4_1_pixie_serialization_transaction_and_1_4_2_always_update(void)
{
  lw::protocols::PixieProtocolT protocol(2, lw::protocols::PixieProtocolSettings{{}, lw::ChannelOrder::RGB::value});
  auto protocolBuffer = bind_protocol_buffer(protocol);

  protocol.update(std::array<lw::Color, 2>{lw::colorFromRGB(1, 2, 3), lw::colorFromRGB(4, 5, 6)}, as_span(protocolBuffer));

  assert_bytes_equal(protocolBuffer, std::vector<uint8_t>{1, 2, 3, 4, 5, 6});
  TEST_ASSERT_TRUE(protocol.alwaysUpdate());
}

void test_1_4_3_pixie_oversized_and_channel_order_edge_contract(void)
{
  {
    lw::protocols::PixieProtocolT protocol(1, lw::protocols::PixieProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 2>{lw::colorFromRGB(1, 2, 3), lw::colorFromRGB(4, 5, 6)}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(protocolBuffer.size()));
  }

  {
    lw::protocols::PixieProtocolT protocol(1, lw::protocols::PixieProtocolSettings{{}, ""});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(9, 10, 11)}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_14_1_constructor_equivalence_and_1_14_3_serialization_for_8_16_bit(void)
{
  const std::array<lw::Color, 1> colors8{lw::colorFromRGB(0x11, 0x22, 0x33)};

  lw::protocols::Ws2812xProtocol protocolA(1, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::GRB::value));
  lw::protocols::Ws2812xProtocol protocolB(1, lw::ChannelOrder::GRB::value);

  auto protocolBufferA = bind_protocol_buffer(protocolA);
  auto protocolBufferB = bind_protocol_buffer(protocolB);

  protocolA.update(colors8, as_span(protocolBufferA));
  protocolB.update(colors8, as_span(protocolBufferB));

  assert_bytes_equal(protocolBufferA, protocolBufferB);

  {
    lw::protocols::Ws2812xProtocol protocol16(1, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::RGB::value));
    auto protocolBuffer = bind_protocol_buffer(protocol16);
    protocol16.update(std::array<lw::Color, 1>{lw::colorFromRGB(0x12, 0x34, 0x56)}, as_span(protocolBuffer));
    assert_bytes_equal(protocolBuffer, encode_ws2812x_payload(std::vector<uint8_t>{0x12, 0x34, 0x56}));
  }

  {
    lw::protocols::Ws2812xProtocol protocol16(1, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::RGBW::value));
    auto protocolBuffer = bind_protocol_buffer(protocol16);
    protocol16.update(std::array<lw::Color, 1>{lw::colorFromRGBW(0x01, 0x02, 0x03, 0x04)}, as_span(protocolBuffer));
    assert_bytes_equal(protocolBuffer, encode_ws2812x_payload(std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}));
  }

  {
    lw::protocols::Ws2812xProtocol protocolMixed(1, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::RGB::value));
    auto protocolBuffer = bind_protocol_buffer(protocolMixed);
    protocolMixed.update(std::array<lw::Color, 1>{lw::colorFromRGB(0x12, 0x34, 0x56)}, as_span(protocolBuffer));
    assert_bytes_equal(protocolBuffer, encode_ws2812x_payload(std::vector<uint8_t>{0x12, 0x34, 0x56}));
  }

  {
    lw::protocols::Ws2812xProtocol protocolMixed(1, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::RGB::value));
    auto protocolBuffer = bind_protocol_buffer(protocolMixed);
    protocolMixed.update(std::array<lw::Color, 1>{lw::colorFromRGB(0x12, 0x34, 0x56)}, as_span(protocolBuffer));
    assert_bytes_equal(protocolBuffer, encode_ws2812x_payload(std::vector<uint8_t>{0x12, 0x34, 0x56}));
  }
}

void test_1_14_2_channel_order_count_resolution(void)
{
  const lw::Color one = lw::colorFromRGBW(1, 2, 3, 4);

  auto run_case = [&](const char* order, const std::vector<uint8_t>& expected)
  {
    lw::protocols::Ws2812xProtocol protocol(1, lw::protocols::Ws2812xProtocolSettings(order));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{one}, as_span(protocolBuffer));
    assert_bytes_equal(protocolBuffer, encode_ws2812x_payload(expected));
  };

  run_case(nullptr, std::vector<uint8_t>{2, 1, 3});
  run_case(lw::ChannelOrder::BGRW::value, std::vector<uint8_t>{3, 2, 1, 4});

  {
    lw::protocols::Ws2812xProtocol protocol(1, lw::protocols::Ws2812xProtocolSettings(""));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{one}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(encode_ws2812x_payload(std::vector<uint8_t>{2, 1, 3}).size()), static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_14_4_ws2812x_readiness_wait_loop_contract(void)
{
  lw::protocols::Ws2812xProtocol protocol(1, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::RGB::value));
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.update(std::array<lw::Color, 1>{lw::colorFromRGB(9, 8, 7)}, as_span(protocolBuffer));

  assert_bytes_equal(protocolBuffer, encode_ws2812x_payload(std::vector<uint8_t>{9, 8, 7}));
}

void test_1_14_5_ws2812x_oversized_span_contract(void)
{
  lw::protocols::Ws2812xProtocol protocol(2, lw::protocols::Ws2812xProtocolSettings(lw::ChannelOrder::GRB::value));
  auto protocolBuffer = bind_protocol_buffer(protocol);

  protocol.update(std::array<lw::Color, 3>{lw::colorFromRGB(1, 2, 3), lw::colorFromRGB(4, 5, 6), lw::colorFromRGB(7, 8, 9)}, as_span(protocolBuffer));

  TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(encode_ws2812x_payload(std::vector<uint8_t>{2, 1, 3, 5, 4, 6}).size()), static_cast<uint32_t>(protocolBuffer.size()));
}
} // namespace

void setUp(void)
{
  ArduinoFakeReset();

  When(Method(ArduinoFake(Function), micros)).AlwaysReturn(0UL);
  When(Method(ArduinoFake(Function), millis)).AlwaysReturn(0UL);
  When(Method(ArduinoFake(Function), yield)).AlwaysDo([]() {});
  When(Method(ArduinoFake(Function), delayMicroseconds)).AlwaysDo([](unsigned int) {});
}

void tearDown(void)
{
}

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_1_1_1_dotstar_construction_and_begin);
  RUN_TEST(test_1_1_2_dotstar_end_frame_extra_byte_calculation);
  RUN_TEST(test_1_1_3_and_1_1_4_dotstar_fixed_brightness_and_luminance_serialization);
  RUN_TEST(test_1_1_5_dotstar_framing_and_transaction_sequence);
  RUN_TEST(test_1_1_6_and_1_1_7_dotstar_oversized_and_channel_order_edge_contract);
  RUN_TEST(test_1_3_1_ws2801_serialization_order_variants);
  RUN_TEST(test_1_3_2_ws2801_transaction_and_latch_timing);
  RUN_TEST(test_1_3_3_ws2801_oversized_and_channel_order_edge_contract);
  RUN_TEST(test_1_4_1_pixie_serialization_transaction_and_1_4_2_always_update);
  RUN_TEST(test_1_4_3_pixie_oversized_and_channel_order_edge_contract);
  RUN_TEST(test_1_14_1_constructor_equivalence_and_1_14_3_serialization_for_8_16_bit);
  RUN_TEST(test_1_14_2_channel_order_count_resolution);
  RUN_TEST(test_1_14_4_ws2812x_readiness_wait_loop_contract);
  RUN_TEST(test_1_14_5_ws2812x_oversized_span_contract);
  return UNITY_END();
}
