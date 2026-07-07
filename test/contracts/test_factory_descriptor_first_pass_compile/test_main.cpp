#include <unity.h>

#include <type_traits>

#include "buses/PixelBus.h"
#include "protocols/DotStarProtocol.h"
#include "protocols/ProtocolAliases.h"
#include "protocols/Ws2812xProtocol.h"
#include "transports/NilTransport.h"

namespace
{
struct ClockedSettings
{
  bool invert{false};
  uint32_t clockRateHz{0};

  static ClockedSettings normalize(ClockedSettings settings) { return settings; }
};

class ClockedTransport : public lw::transports::ITransport
{
public:
  using TransportSettingsType = ClockedSettings;

  explicit ClockedTransport(TransportSettingsType settings = {}) : settings_(settings) {}

  void begin() override {}

  void transmitBytes(lw::span<uint8_t>) override {}

  void transmitBytes(lw::span<uint8_t>, lw::transports::TransportBrightness) override {}

  TransportSettingsType settings_{};
};

void test_pixel_bus_typed_protocol_transport(void)
{
  using Protocol = lw::protocols::Apa102Protocol<lw::Rgbw8Color>;

  lw::busses::PixelBus<Protocol, lw::transports::NilTransport> bus(16, lw::protocols::Apa102ProtocolSettings{}, lw::transports::NilTransportSettings{});

  TEST_ASSERT_EQUAL_UINT32(16U, static_cast<uint32_t>(bus.pixelCount()));
}

void test_pixel_bus_ws_alias_defaults_transport_rate(void)
{
  using Alias = lw::protocols::Ws2812x<>;

  lw::busses::PixelBus<Alias, ClockedTransport> bus(10, ClockedSettings{});

  TEST_ASSERT_EQUAL_UINT32(lw::transports::timing::Generic800.encodedDataRateHz(), bus.transport().settings_.clockRateHz);
  TEST_ASSERT_FALSE(bus.transport().settings_.invert);
}

void test_pixel_bus_ws_alias_timing_override(void)
{
  using Alias = lw::protocols::Ws2812x<>;
  auto protocolSettings = Alias::defaultSettings();
  protocolSettings.timing = lw::transports::OneWireTiming::Ws2811;

  lw::busses::PixelBus<Alias, ClockedTransport> bus(12, protocolSettings, ClockedSettings{});

  TEST_ASSERT_EQUAL_UINT32(lw::transports::timing::Ws2811.encodedDataRateHz(), bus.transport().settings_.clockRateHz);
}

void test_direct_pixel_bus_with_alias_spec(void)
{
  using Alias = lw::protocols::DotStar<>;

  lw::busses::PixelBus<Alias, lw::transports::NilTransport> bus(6, lw::transports::NilTransportSettings{});

  auto& settings = static_cast<lw::protocols::Apa102ProtocolSettings&>(bus.protocol().settings());
  TEST_ASSERT_EQUAL_PTR(lw::ChannelOrder::BGR::value, settings.channelOrder);
}

void test_alias_type_is_direct_protocol(void)
{
  static_assert(std::is_same<typename lw::protocols::Ws2812x<lw::Rgbw8Color>::ProtocolType, lw::protocols::Ws2812xProtocol<lw::Rgbw8Color, 3>>::value, "Ws2812x alias ProtocolType should resolve to direct protocol type");
  TEST_ASSERT_TRUE(true);
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
  RUN_TEST(test_pixel_bus_typed_protocol_transport);
  RUN_TEST(test_pixel_bus_ws_alias_defaults_transport_rate);
  RUN_TEST(test_pixel_bus_ws_alias_timing_override);
  RUN_TEST(test_direct_pixel_bus_with_alias_spec);
  RUN_TEST(test_alias_type_is_direct_protocol);
  return UNITY_END();
}
