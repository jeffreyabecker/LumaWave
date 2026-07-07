#include <unity.h>

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "buses/ReferenceBus.h"
#include "colors/Color.h"
#include "protocols/IProtocol.h"
#include "transports/ITransport.h"

namespace
{
using TestColor = lw::Color;

class CaptureProtocol : public lw::protocols::IProtocol
{
public:
  explicit CaptureProtocol(uint16_t pixelCount) : lw::protocols::IProtocol(pixelCount) {}

  ~CaptureProtocol() override { ++destructorCount; }

  void begin() override { began = true; }

  void update(lw::span<const lw::Color> colors, lw::span<uint8_t> buffer = lw::span<uint8_t>{}) override
  {
    lastSource = colors.data();
    captured.assign(colors.begin(), colors.end());
    lastBufferSize = buffer.size();
    if (!buffer.empty())
    {
      std::fill(buffer.begin(), buffer.end(), 0x5A);
    }
  }

  lw::protocols::ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  size_t requiredBufferSizeBytes() const override { return 4U; }

  static int destructorCount;
  bool began{false};
  const TestColor* lastSource{nullptr};
  size_t lastBufferSize{0};
  std::vector<TestColor> captured{};

private:
  lw::protocols::ProtocolSettings _settings{};
};

int CaptureProtocol::destructorCount = 0;

class CaptureTransport : public lw::transports::ITransport
{
public:
  ~CaptureTransport() override { ++destructorCount; }

  void begin() override { began = true; }

  void beginTransaction() override { ++beginTransactionCount; }

  void transmitBytes(lw::span<uint8_t> bytes) override { transmitted.assign(bytes.begin(), bytes.end()); }

  void endTransaction() override { ++endTransactionCount; }

  static int destructorCount;
  bool began{false};
  size_t beginTransactionCount{0};
  size_t endTransactionCount{0};
  std::vector<uint8_t> transmitted{};
};

int CaptureTransport::destructorCount = 0;

struct OwnedColor : lw::Color
{
  static int destructorCount;

  OwnedColor() = default;
  using lw::Color::Color;

  ~OwnedColor() { ++destructorCount; }
};

int OwnedColor::destructorCount = 0;

class NoopProtocolOwnedColor : public lw::protocols::IProtocol
{
public:
  explicit NoopProtocolOwnedColor(uint16_t pixelCount) : lw::protocols::IProtocol(pixelCount) {}

  ~NoopProtocolOwnedColor() override { ++destructorCount; }

  void begin() override {}

  void update(lw::span<const lw::Color>, lw::span<uint8_t> = lw::span<uint8_t>{}) override {}

  lw::protocols::ProtocolSettings& settings() override { return _settings; }

  bool alwaysUpdate() const override { return false; }

  static int destructorCount;

private:
  lw::protocols::ProtocolSettings _settings{};
};

int NoopProtocolOwnedColor::destructorCount = 0;

class NoopTransportOwnedColor : public lw::transports::ITransport
{
public:
  ~NoopTransportOwnedColor() override { ++destructorCount; }

  void begin() override {}

  void transmitBytes(lw::span<uint8_t>) override {}

  static int destructorCount;
};

int NoopTransportOwnedColor::destructorCount = 0;

void resetDestructorCounters()
{
  CaptureProtocol::destructorCount = 0;
  CaptureTransport::destructorCount = 0;
  NoopProtocolOwnedColor::destructorCount = 0;
  NoopTransportOwnedColor::destructorCount = 0;
  OwnedColor::destructorCount = 0;
}

void test_reference_bus_uses_root_buffer_directly(void)
{
  resetDestructorCounters();

  auto protocol = std::make_unique<CaptureProtocol>(2);
  auto transport = std::make_unique<CaptureTransport>();
  auto* protocolPtr = protocol.get();

  lw::busses::ReferenceBus bus(2, std::move(protocol), std::move(transport));
  bus.pixels()[0] = lw::Color{10, 2, 3};
  bus.pixels()[1] = lw::Color{20, 5, 6};

  bus.show();

  TEST_ASSERT_TRUE(protocolPtr->lastSource == bus.rootBuffer());
  TEST_ASSERT_EQUAL_UINT8(10, bus.rootBuffer()[0]['R']);
  TEST_ASSERT_EQUAL_UINT8(20, bus.rootBuffer()[1]['R']);
}

void test_reference_bus_owns_all_resources(void)
{
  resetDestructorCounters();

  {
    auto protocol = std::make_unique<NoopProtocolOwnedColor>(1);
    auto transport = std::make_unique<NoopTransportOwnedColor>();

    lw::busses::ReferenceBus bus(1, std::move(protocol), std::move(transport));
  }

  TEST_ASSERT_EQUAL_INT(1, NoopProtocolOwnedColor::destructorCount);
  TEST_ASSERT_EQUAL_INT(1, NoopTransportOwnedColor::destructorCount);
  TEST_ASSERT_EQUAL_INT(2, OwnedColor::destructorCount);
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
  RUN_TEST(test_reference_bus_uses_root_buffer_directly);
  RUN_TEST(test_reference_bus_owns_all_resources);
  return UNITY_END();
}
