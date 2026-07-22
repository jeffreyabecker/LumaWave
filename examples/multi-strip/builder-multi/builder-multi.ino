#include <Arduino.h>
#include <LumaWave.h>

// ---------------------------------------------------------------------------
// Multi-Strip via Shared Pixel Buffer
//
// Demonstrates driving multiple independent strips from a single pixel
// buffer using separate BusBuilder instances with setPixelStorage().
//
// Each strip owns its own protocol, transport, buffers, and pipeline.
// The pixel buffer is caller-owned and split into sub-spans — each strip
// reads/writes its own region. No composite or aggregate types needed.
// ---------------------------------------------------------------------------

constexpr size_t kStrip1Len = 30;
constexpr size_t kStrip2Len = 60;
constexpr size_t kTotalPixels = kStrip1Len + kStrip2Len;

// Single shared pixel buffer — all strips write into disjoint regions.
lw::Pixel g_pixels[kTotalPixels];

// Each strip is a fully independent IPixelBus.
std::unique_ptr<lw::IPixelBus> g_strip1;
std::unique_ptr<lw::IPixelBus> g_strip2;

void setup()
{
  // Strip 1: pixels [0..29], GRB channel order
  g_strip1 = lw::buses::BusBuilder()
                 .setPixelStorage(lw::span<lw::Pixel>{g_pixels, kStrip1Len})
                 .setTransport(lw::transports::NilTransport{})
                 .setProtocol(lw::protocols::Ws2812xProtocol(kStrip1Len, lw::protocols::Ws2812xProtocolSettings{lw::ChannelOrder::GRB::value}))
                 .build();

  // Strip 2: pixels [30..89], RGB channel order
  g_strip2 = lw::buses::BusBuilder()
                 .setPixelStorage(lw::span<lw::Pixel>{g_pixels + kStrip1Len, kStrip2Len})
                 .setTransport(lw::transports::NilTransport{})
                 .setProtocol(lw::protocols::Ws2812xProtocol(kStrip2Len, lw::protocols::Ws2812xProtocolSettings{lw::ChannelOrder::RGB::value}))
                 .build();

  g_strip1->begin();
  g_strip2->begin();

  // Animation loop
  uint16_t frame = 0;
  while (true)
  {
    // Paint strip 1 directly into g_pixels[0..29]
    auto& p1 = g_strip1->pixels();
    for (size_t i = 0; i < kStrip1Len; ++i)
    {
      p1[i] = lw::pixelFromRGB(static_cast<uint8_t>((i + frame) & 0xFF), static_cast<uint8_t>((2U * i + frame) & 0xFF), 0);
    }

    // Paint strip 2 directly into g_pixels[30..89]
    auto& p2 = g_strip2->pixels();
    for (size_t i = 0; i < kStrip2Len; ++i)
    {
      p2[i] = lw::pixelFromRGB(0, static_cast<uint8_t>((i + frame) & 0xFF), static_cast<uint8_t>((2U * i + frame) & 0xFF));
    }

    g_strip1->show();
    g_strip2->show();
    ++frame;
    delay(20);
  }
}

void loop()
{
  // setup() never returns
}
