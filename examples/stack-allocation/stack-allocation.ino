#include <Arduino.h>
#include <LumaWave.h>

// ---------------------------------------------------------------------------
// Stack (Static) Allocation
//
// Demonstrates zero-heap bus construction using StackBusStorage. All buffers
// are compile-time-sized C arrays — no std::vector, no new, no malloc.
// Suitable for -fno-rtti -fno-exceptions embedded builds.
//
// The builder validates the configuration matches the storage's compile-time
// sizes, then returns a reference to the storage's Bus.
// ---------------------------------------------------------------------------

constexpr size_t kLedCount = 30;

// All storage is stack-allocated by the template. No heap usage.
lw::buses::StackBusStorage<kLedCount, lw::protocols::Ws2812xProtocol, lw::transports::NilTransport> g_storage;

void setup()
{
  auto& strip = lw::buses::BusBuilder()
                    .setPixelCount(kLedCount)
                    .setTransport(lw::transports::NilTransport{})
                    .setProtocol(lw::protocols::Ws2812xProtocol(kLedCount, lw::protocols::Ws2812xProtocolSettings{lw::ChannelOrder::GRB::value}))
                    .buildInto(g_storage);

  strip.begin();

  strip.pixels()[0] = lw::pixelFromRGB(0, 255, 0);
  strip.show();
}

void loop()
{
}
