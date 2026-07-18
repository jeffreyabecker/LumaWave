#include <unity.h>

#include <array>
#include <cstdint>
#include <vector>

#include <ArduinoFake.h>

#include "protocols/Lpd6803Protocol.h"
#include "protocols/Lpd8806Protocol.h"
#include "protocols/P9813Protocol.h"
#include "protocols/Sm16716Protocol.h"
#include "protocols/Sm168xProtocol.h"
#include "protocols/Tlc59711Protocol.h"
#include "protocols/Tm1814Protocol.h"
#include "protocols/Tm1914Protocol.h"
#include "transports/OneWireEncoding.h"

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

std::vector<uint8_t> encode_onewire_payload(const std::vector<uint8_t>& raw, const lw::transports::OneWireTiming& timing, bool protocolIdleHigh = false, uint8_t prefixResetMultiplier = 1, uint8_t suffixResetMultiplier = 1)
{
  std::vector<uint8_t> source = raw;
  const size_t payloadBytes = lw::transports::OneWireEncoding::expandedPayloadSizeBytes(raw.size(), timing.bitPattern());
  const size_t prefixResetBytes = lw::transports::OneWireEncoding::computeResetBytes(timing, 0, prefixResetMultiplier);
  const size_t suffixResetBytes = lw::transports::OneWireEncoding::computeResetBytes(timing, 0, suffixResetMultiplier);
  std::vector<uint8_t> encoded(payloadBytes + prefixResetBytes + suffixResetBytes, 0);
  const size_t encodedSize = lw::transports::OneWireEncoding::encodeWithResets(source.data(), source.size(), encoded.data(), encoded.size(), timing, 0, prefixResetMultiplier, suffixResetMultiplier, protocolIdleHigh);
  encoded.resize(encodedSize);
  return encoded;
}

void test_1_5_1_lpd6803_packed_5_5_5_serialization(void)
{
  lw::protocols::Lpd6803ProtocolT protocol(1, lw::protocols::Lpd6803ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.begin();
  protocol.update(std::array<lw::Color, 1>{lw::Color{0xFF, 0x00, 0x88}}, as_span(protocolBuffer));

  const std::vector<uint8_t> payload = slice_bytes(protocolBuffer, 4, 2);
  TEST_ASSERT_EQUAL_UINT8(0xFC, payload[0]);
  TEST_ASSERT_EQUAL_UINT8(0x11, payload[1]);
}

void test_1_5_2_lpd6803_framing_end_frame_size(void)
{
  const std::array<uint16_t, 4> counts{1, 8, 9, 16};
  for (uint16_t n : counts)
  {
    lw::protocols::Lpd6803ProtocolT protocol(n, lw::protocols::Lpd6803ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    std::vector<lw::Color> colors(n, lw::Color{1, 2, 3});
    protocol.begin();
    protocol.update(lw::span<const lw::Color>{colors.data(), colors.size()}, as_span(protocolBuffer));

    const size_t expectedSize = 4u + (static_cast<size_t>(n) * 2u) + ((static_cast<size_t>(n) + 7u) / 8u);
    TEST_ASSERT_EQUAL_UINT32(expectedSize, static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_5_3_lpd6803_oversized_and_channel_order_safety(void)
{
  {
    lw::protocols::Lpd6803ProtocolT protocol(1, lw::protocols::Lpd6803ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 2>{lw::Color{1, 2, 3}, lw::Color{4, 5, 6}}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 4, 2).size()));
  }

  {
    lw::protocols::Lpd6803ProtocolT protocol(1, lw::protocols::Lpd6803ProtocolSettings{{}, ""});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 1>{lw::Color{9, 10, 11}}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 4, 2).size()));
  }
}

void test_1_6_1_lpd8806_7bit_plus_msb_serialization(void)
{
  lw::protocols::Lpd8806ProtocolT protocol(1, lw::protocols::Lpd8806ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.begin();
  protocol.update(std::array<lw::Color, 1>{lw::Color{0x00, 0xFF, 0x80}}, as_span(protocolBuffer));

  const auto payload = slice_bytes(protocolBuffer, 1, 3);
  TEST_ASSERT_EQUAL_UINT8(0x80, payload[0]);
  TEST_ASSERT_EQUAL_UINT8(0xFF, payload[1]);
  TEST_ASSERT_EQUAL_UINT8(0xC0, payload[2]);
}

void test_1_6_2_lpd8806_symmetric_start_end_framing(void)
{
  const std::array<uint16_t, 3> counts{1, 32, 33};
  for (uint16_t n : counts)
  {
    lw::protocols::Lpd8806ProtocolT protocol(n, lw::protocols::Lpd8806ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    std::vector<lw::Color> colors(n, lw::Color{1, 2, 3});
    protocol.begin();
    protocol.update(lw::span<const lw::Color>{colors.data(), colors.size()}, as_span(protocolBuffer));

    const size_t frameSize = (static_cast<size_t>(n) + 31u) / 32u;
    TEST_ASSERT_EQUAL_UINT32(frameSize + (static_cast<size_t>(n) * 3u) + frameSize, static_cast<uint32_t>(protocolBuffer.size()));
    TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0xFF, protocolBuffer.back());
  }
}

void test_1_6_3_lpd8806_oversized_and_channel_order_safety(void)
{
  {
    lw::protocols::Lpd8806ProtocolT protocol(1, lw::protocols::Lpd8806ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 2>{lw::Color{1, 2, 3}, lw::Color{4, 5, 6}}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 1, 3).size()));
  }

  {
    lw::protocols::Lpd8806ProtocolT protocol(1, lw::protocols::Lpd8806ProtocolSettings{{}, ""});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.begin();
    protocol.update(std::array<lw::Color, 1>{lw::Color{9, 10, 11}}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 1, 3).size()));
  }
}

void test_1_7_1_and_1_7_2_p9813_header_checksum_and_framing(void)
{
  lw::protocols::P9813ProtocolT protocol(1, lw::protocols::P9813ProtocolSettings{{}});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.begin();
  protocol.update(std::array<lw::Color, 1>{lw::Color{0x80, 0x40, 0x00}}, as_span(protocolBuffer));

  const auto payload = slice_bytes(protocolBuffer, 4, 4);
  const uint8_t expectedHeader = static_cast<uint8_t>(0xC0 | (((~0x00u >> 6) & 0x03) << 4) | (((~0x40u >> 6) & 0x03) << 2) | ((~0x80u >> 6) & 0x03));
  TEST_ASSERT_EQUAL_UINT8(expectedHeader, payload[0]);
  TEST_ASSERT_EQUAL_UINT8(0x00, payload[1]);
  TEST_ASSERT_EQUAL_UINT8(0x40, payload[2]);
  TEST_ASSERT_EQUAL_UINT8(0x80, payload[3]);
}

void test_1_7_3_p9813_oversized_span_safety(void)
{
  lw::protocols::P9813ProtocolT protocol(1, lw::protocols::P9813ProtocolSettings{{}});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.begin();
  protocol.update(std::array<lw::Color, 2>{lw::Color{1, 2, 3}, lw::Color{4, 5, 6}}, as_span(protocolBuffer));

  TEST_ASSERT_EQUAL_UINT32(4U, static_cast<uint32_t>(slice_bytes(protocolBuffer, 4, 4).size()));
}

void test_1_8_1_sm168x_variant_resolution_and_frame_sizing(void)
{
  auto run_case = [&](size_t expectedFrameSize, auto protocolFactory, const char* order)
  {
    lw::protocols::Sm168xProtocolSettings settings{};
    settings.channelOrder = order;

    auto protocol = protocolFactory(std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    std::vector<lw::Color> colors(2, lw::Color{1, 2, 3, 4});
    protocol.update(lw::span<const lw::Color>{colors.data(), colors.size()}, as_span(protocolBuffer));

    TEST_ASSERT_EQUAL_UINT32(expectedFrameSize, static_cast<uint32_t>(protocolBuffer.size()));
  };

  run_case(10U, [](lw::protocols::Sm168xProtocolSettings settings) { return lw::protocols::Sm168xProtocol(2, std::move(settings)); }, "RGBW");
  run_case(10U, [](lw::protocols::Sm168xProtocolSettings settings) { return lw::protocols::Sm168xProtocol(2, std::move(settings)); }, "RGBW");
}

void test_1_8_4_sm168x_oversized_and_order_safety(void)
{
  {
    lw::protocols::Sm168xProtocolSettings settings{};
    settings.channelOrder = "";

    lw::protocols::Sm168xProtocol protocol(1, std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::Color{11, 12, 13, 14}}, as_span(protocolBuffer));

    TEST_ASSERT_EQUAL_UINT32(5U, static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_9_1_sm16716_buffer_size_and_start_bit_prefix(void)
{
  lw::protocols::Sm16716ProtocolT protocol(1, lw::protocols::Sm16716ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.update(std::array<lw::Color, 1>{lw::Color{0, 0, 0}}, as_span(protocolBuffer));

  TEST_ASSERT_EQUAL_UINT32(10U, static_cast<uint32_t>(protocolBuffer.size()));
  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[0]);
  TEST_ASSERT_EQUAL_UINT8(0x00, protocolBuffer[1]);
  TEST_ASSERT_EQUAL_UINT8(0x20, protocolBuffer[6]);
}

void test_1_9_3_sm16716_oversized_and_order_safety(void)
{
  {
    lw::protocols::Sm16716ProtocolT protocol(1, lw::protocols::Sm16716ProtocolSettings{{}, lw::ChannelOrder::RGB::value});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 2>{lw::Color{1, 2, 3}, lw::Color{4, 5, 6}}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(10U, static_cast<uint32_t>(protocolBuffer.size()));
  }

  {
    lw::protocols::Sm16716ProtocolT protocol(1, lw::protocols::Sm16716ProtocolSettings{{}, ""});
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::Color{7, 8, 9}}, as_span(protocolBuffer));
    TEST_ASSERT_EQUAL_UINT32(10U, static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_11_1_and_1_11_3_tlc59711_header_encoding_and_latch_guard(void)
{
  lw::protocols::Tlc59711Settings cfg{};
  cfg.outtmg = true;
  cfg.extgck = true;
  cfg.tmgrst = false;
  cfg.dsprpt = true;
  cfg.blank = true;

  lw::protocols::Tlc59711ProtocolT protocol(1, lw::protocols::Tlc59711ProtocolSettings{{}, cfg});
  protocol.setGain(128);
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.update(std::array<lw::Color, 1>{lw::Color{1, 2, 3}}, as_span(protocolBuffer));

  TEST_ASSERT_EQUAL_UINT8(0x97, protocolBuffer[0]);
  TEST_ASSERT_EQUAL_UINT8(0x70, protocolBuffer[1]);
  TEST_ASSERT_EQUAL_UINT8(0x20, protocolBuffer[2]);
  TEST_ASSERT_EQUAL_UINT8(0x40, protocolBuffer[3]);

  TEST_ASSERT_EQUAL_UINT32(28U, static_cast<uint32_t>(protocolBuffer.size()));
}

void test_1_12_1_1_12_2_1_12_3_tm1814_currents_inversion_and_payload_order(void)
{
  lw::protocols::Tm1814ProtocolSettings settings{};
  settings.channelOrder = "WRGB";
  settings.current.redMilliAmps = 10;
  settings.current.greenMilliAmps = 190;
  settings.current.blueMilliAmps = 380;
  settings.current.whiteMilliAmps = 1000;

  lw::protocols::Tm1814ProtocolT protocol(1, std::move(settings));
  auto protocolBuffer = bind_protocol_buffer(protocol);
  protocol.update(std::array<lw::Color, 1>{lw::Color{1, 2, 3, 4}}, as_span(protocolBuffer));

  const std::vector<uint8_t> expectedRaw{63, 0, 25, 63, static_cast<uint8_t>(~63), static_cast<uint8_t>(~0), static_cast<uint8_t>(~25), static_cast<uint8_t>(~63), 4, 1, 2, 3};
  assert_bytes_equal(protocolBuffer, encode_onewire_payload(expectedRaw, lw::transports::timing::Tm1814, true));
}

void test_1_12_4_tm1814_oversized_and_order_safety(void)
{
  {
    lw::protocols::Tm1814ProtocolSettings settings{};
    settings.channelOrder = "WRGB";

    lw::protocols::Tm1814ProtocolT protocol(1, std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 2>{lw::Color{1, 2, 3, 4}, lw::Color{5, 6, 7, 8}}, as_span(protocolBuffer));

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(encode_onewire_payload(std::vector<uint8_t>{63, 63, 63, 63, 192, 192, 192, 192, 4, 1, 2, 3}, lw::transports::timing::Tm1814, true).size()),
                             static_cast<uint32_t>(protocolBuffer.size()));
  }

  {
    lw::protocols::Tm1814ProtocolSettings settings{};
    settings.channelOrder = "";

    lw::protocols::Tm1814ProtocolT protocol(1, std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::Color{9, 10, 11, 12}}, as_span(protocolBuffer));

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(encode_onewire_payload(std::vector<uint8_t>{63, 63, 63, 63, 192, 192, 192, 192, 10, 9, 11, 0}, lw::transports::timing::Tm1814, true).size()),
                             static_cast<uint32_t>(protocolBuffer.size()));
  }
}

void test_1_13_1_and_1_13_2_tm1914_mode_matrix_inversion_and_payload_order(void)
{
  auto run_mode = [&](lw::protocols::Tm1914Mode mode, uint8_t expectedMode)
  {
    lw::protocols::Tm1914ProtocolSettings settings{};
    settings.channelOrder = lw::ChannelOrder::GRB::value;
    settings.mode = mode;

    lw::protocols::Tm1914ProtocolT protocol(1, std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::Color{1, 2, 3}}, as_span(protocolBuffer));

    const std::vector<uint8_t> expectedRaw{0xFF, 0xFF, expectedMode, static_cast<uint8_t>(~0xFF), static_cast<uint8_t>(~0xFF), static_cast<uint8_t>(~expectedMode), 2, 1, 3};
    assert_bytes_equal(protocolBuffer, encode_onewire_payload(expectedRaw, lw::transports::timing::Tm1914, true));
  };

  run_mode(lw::protocols::Tm1914Mode::DinFdinAutoSwitch, 0xFF);
  run_mode(lw::protocols::Tm1914Mode::DinOnly, 0xF5);
  run_mode(lw::protocols::Tm1914Mode::FdinOnly, 0xFA);
}

void test_1_13_3_tm1914_oversized_and_order_safety(void)
{
  {
    lw::protocols::Tm1914ProtocolSettings settings{};
    settings.channelOrder = lw::ChannelOrder::GRB::value;

    lw::protocols::Tm1914ProtocolT protocol(1, std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 2>{lw::Color{1, 2, 3}, lw::Color{4, 5, 6}}, as_span(protocolBuffer));

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(encode_onewire_payload(std::vector<uint8_t>{0xFF, 0xFF, 0xF5, 0x00, 0x00, 0x0A, 2, 1, 3}, lw::transports::timing::Tm1914, true).size()),
                             static_cast<uint32_t>(protocolBuffer.size()));
  }

  {
    lw::protocols::Tm1914ProtocolSettings settings{};
    settings.channelOrder = "";

    lw::protocols::Tm1914ProtocolT protocol(1, std::move(settings));
    auto protocolBuffer = bind_protocol_buffer(protocol);
    protocol.update(std::array<lw::Color, 1>{lw::Color{7, 8, 9}}, as_span(protocolBuffer));

    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(encode_onewire_payload(std::vector<uint8_t>{0xFF, 0xFF, 0xF5, 0x00, 0x00, 0x0A, 8, 7, 9}, lw::transports::timing::Tm1914, true).size()),
                             static_cast<uint32_t>(protocolBuffer.size()));
  }
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
  RUN_TEST(test_1_5_1_lpd6803_packed_5_5_5_serialization);
  RUN_TEST(test_1_5_2_lpd6803_framing_end_frame_size);
  RUN_TEST(test_1_5_3_lpd6803_oversized_and_channel_order_safety);
  RUN_TEST(test_1_6_1_lpd8806_7bit_plus_msb_serialization);
  RUN_TEST(test_1_6_2_lpd8806_symmetric_start_end_framing);
  RUN_TEST(test_1_6_3_lpd8806_oversized_and_channel_order_safety);
  RUN_TEST(test_1_7_1_and_1_7_2_p9813_header_checksum_and_framing);
  RUN_TEST(test_1_7_3_p9813_oversized_span_safety);
  RUN_TEST(test_1_8_1_sm168x_variant_resolution_and_frame_sizing);
  RUN_TEST(test_1_8_4_sm168x_oversized_and_order_safety);
  RUN_TEST(test_1_9_1_sm16716_buffer_size_and_start_bit_prefix);
  RUN_TEST(test_1_9_3_sm16716_oversized_and_order_safety);
  RUN_TEST(test_1_11_1_and_1_11_3_tlc59711_header_encoding_and_latch_guard);
  RUN_TEST(test_1_12_1_1_12_2_1_12_3_tm1814_currents_inversion_and_payload_order);
  RUN_TEST(test_1_12_4_tm1814_oversized_and_order_safety);
  RUN_TEST(test_1_13_1_and_1_13_2_tm1914_mode_matrix_inversion_and_payload_order);
  RUN_TEST(test_1_13_3_tm1914_oversized_and_order_safety);
  return UNITY_END();
}
