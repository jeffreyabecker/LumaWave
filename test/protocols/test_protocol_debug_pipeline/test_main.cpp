#include <unity.h>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <ArduinoFake.h>

#include "colors/Color.h"
#include "protocols/DebugProtocol.h"
#include "protocols/IProtocol.h"
#include "transports/PrintTransport.h"

namespace
{


struct ProtocolSpySettings : public lw::protocols::ProtocolSettings
{
  int* initializeCount = nullptr;
  int* updateCount = nullptr;
  bool* always = nullptr;
  std::vector<lw::Color>* lastFrame = nullptr;
};

class ProtocolSpy : public lw::protocols::IProtocol
{
public:
  using SettingsType = ProtocolSpySettings;

  static constexpr size_t requiredBufferSize(uint16_t, const SettingsType&) { return 0; }

  ProtocolSpy(uint16_t pixelCount, SettingsType settings) : lw::protocols::IProtocol(pixelCount), _settings(std::move(settings)) {}

  lw::protocols::ProtocolSettings& settings() override { return _settings; }

  void begin() override
  {
    if (_settings.initializeCount != nullptr)
    {
      ++(*_settings.initializeCount);
    }
  }

  void update(lw::span<const lw::Color> colors, lw::span<uint8_t> buffer = lw::span<uint8_t>{}) override
  {
    if (_settings.updateCount != nullptr)
    {
      ++(*_settings.updateCount);
    }

    if (_settings.lastFrame != nullptr)
    {
      _settings.lastFrame->assign(colors.begin(), colors.end());
    }

    (void)buffer;
  }

  bool alwaysUpdate() const override { return (_settings.always != nullptr) ? *_settings.always : false; }

private:
  SettingsType _settings;
};

class WritableSpy
{
public:
  size_t write(const uint8_t* data, size_t size)
  {
    if (data == nullptr || size == 0)
    {
      return 0;
    }

    bytes.insert(bytes.end(), data, data + size);
    return size;
  }

  std::vector<uint8_t> bytes{};
};

void test_debug_protocol_forwards_to_inner_protocol_without_output(void)
{
  int initializeCount = 0;
  int updateCount = 0;
  bool always = false;
  std::vector<lw::Color> lastFrame{};

  lw::protocols::DebugProtocolSettingsT<ProtocolSpy> settings{};
  settings.output = nullptr;
  settings.invert = false;
  settings.wrapped.initializeCount = &initializeCount;
  settings.wrapped.updateCount = &updateCount;
  settings.wrapped.always = &always;
  settings.wrapped.lastFrame = &lastFrame;

  lw::protocols::DebugProtocol<ProtocolSpy> protocol(2, std::move(settings));

  std::vector<lw::Color> colors{lw::Color{0x01, 0x02, 0x03, 0x04}, lw::Color{0xAA, 0xBB, 0xCC, 0xDD}};

  protocol.begin();
  protocol.update(lw::span<const lw::Color>{colors.data(), colors.size()});

  TEST_ASSERT_EQUAL_INT(1, initializeCount);
  TEST_ASSERT_EQUAL_INT(1, updateCount);
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(lastFrame.size()));
}

void test_debug_protocol_always_update_delegates_to_inner_protocol(void)
{
  int initializeCount = 0;
  int updateCount = 0;
  bool always = true;
  std::vector<lw::Color> lastFrame{};

  lw::protocols::DebugProtocolSettingsT<ProtocolSpy> settings{};
  settings.output = nullptr;
  settings.wrapped.initializeCount = &initializeCount;
  settings.wrapped.updateCount = &updateCount;
  settings.wrapped.always = &always;
  settings.wrapped.lastFrame = &lastFrame;

  lw::protocols::DebugProtocol<ProtocolSpy> protocol(1, std::move(settings));

  TEST_ASSERT_TRUE(protocol.alwaysUpdate());
}

void test_print_transport_forwards_raw_bytes_without_ascii_or_debug(void)
{
  WritableSpy writable{};
  lw::transports::PrintTransportSettingsT<WritableSpy> config{};
  config.output = &writable;
  config.asciiOutput = false;
  config.debugOutput = false;

  lw::transports::PrintTransportT<WritableSpy> transport(std::move(config));

  std::array<uint8_t, 3> bytes{0x12, 0x34, 0xAB};

  transport.begin();
  transport.beginTransaction();
  transport.transmitBytes(bytes);
  transport.endTransaction();

  TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(writable.bytes.size()));
  TEST_ASSERT_EQUAL_UINT8(0x12, writable.bytes[0]);
  TEST_ASSERT_EQUAL_UINT8(0x34, writable.bytes[1]);
  TEST_ASSERT_EQUAL_UINT8(0xAB, writable.bytes[2]);
}

void test_print_transport_ascii_output_hex_encodes_bytes(void)
{
  WritableSpy writable{};
  lw::transports::PrintTransportSettingsT<WritableSpy> config{};
  config.output = &writable;
  config.asciiOutput = true;
  config.debugOutput = false;

  lw::transports::PrintTransportT<WritableSpy> transport(std::move(config));

  std::array<uint8_t, 2> bytes{0x00, 0xAF};
  transport.transmitBytes(bytes);

  TEST_ASSERT_EQUAL_UINT32(4U, static_cast<uint32_t>(writable.bytes.size()));
  TEST_ASSERT_EQUAL_UINT8('0', writable.bytes[0]);
  TEST_ASSERT_EQUAL_UINT8('0', writable.bytes[1]);
  TEST_ASSERT_EQUAL_UINT8('A', writable.bytes[2]);
  TEST_ASSERT_EQUAL_UINT8('F', writable.bytes[3]);
}

void test_print_transport_debug_output_emits_event_messages(void)
{
  WritableSpy writable{};
  lw::transports::PrintTransportSettingsT<WritableSpy> config{};
  config.output = &writable;
  config.asciiOutput = false;
  config.debugOutput = true;

  lw::transports::PrintTransportT<WritableSpy> transport(std::move(config));

  std::array<uint8_t, 2> bytes{0x12, 0x34};
  transport.begin();
  transport.beginTransaction();
  transport.transmitBytes(bytes);
  transport.endTransaction();

  std::string output(writable.bytes.begin(), writable.bytes.end());
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(output.find("[BUS] begin")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(output.find("[BUS] beginTransaction")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(output.find("[BUS] bytes(2,bri=65535/65535)")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(output.find("[BUS] endTransaction")));
}

void test_print_transport_copies_identifier_for_debug_prefix(void)
{
  WritableSpy writable{};

  char identifier[] = "alpha";

  lw::transports::PrintTransportSettingsT<WritableSpy> config{};
  config.output = &writable;
  config.debugOutput = true;
  config.identifier = identifier;

  lw::transports::PrintTransportT<WritableSpy> transport(std::move(config));

  identifier[0] = 'X';

  transport.begin();

  std::string output(writable.bytes.begin(), writable.bytes.end());
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(output.find("[BUS:alpha] begin")));
  TEST_ASSERT_EQUAL(-1, static_cast<int>(output.find("[BUS:Xlpha] begin")));
}
} // namespace

void setUp(void)
{
  ArduinoFakeReset();
}

void tearDown(void)
{
}

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_debug_protocol_forwards_to_inner_protocol_without_output);
  RUN_TEST(test_debug_protocol_always_update_delegates_to_inner_protocol);
  RUN_TEST(test_print_transport_forwards_raw_bytes_without_ascii_or_debug);
  RUN_TEST(test_print_transport_ascii_output_hex_encodes_bytes);
  RUN_TEST(test_print_transport_debug_output_emits_event_messages);
  RUN_TEST(test_print_transport_copies_identifier_for_debug_prefix);
  return UNITY_END();
}
