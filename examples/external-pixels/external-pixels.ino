#include <Arduino.h>
#include <LumaWave.h>

// ---------------------------------------------------------------------------
// External Pixel Storage
//
// Demonstrates using a caller-owned pixel buffer with the builder. The bus
// reads and writes directly into the caller's array — no internal pixel
// allocation. Useful for zero-copy integration with frameworks that manage
// their own LED buffers (e.g., WLED).
// ---------------------------------------------------------------------------

constexpr size_t kLedCount = 30;

// Caller-owned pixel storage — must outlive the strip.
lw::Pixel g_pixels[kLedCount];

void setup()
{
  // Pass external pixels to the builder via setPixelStorage().
  auto strip = lw::buses::BusBuilder()
                   .setPixelStorage(lw::span<lw::Pixel>{g_pixels, kLedCount})
                   .setTransport(lw::transports::NilTransport{})
                   .setProtocol(lw::protocols::Ws2812xProtocol(kLedCount, lw::protocols::Ws2812xProtocolSettings{lw::ChannelOrder::GRB::value}))
                   .build();

  strip->begin();

  // Writing through strip->pixels() modifies g_pixels directly.
  strip->pixels()[0] = lw::pixelFromRGB(255, 0, 0);
  // g_pixels[0] is now {255, 0, 0}
  strip->show();
}

void loop()
{
}
