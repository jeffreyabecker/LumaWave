#include <Arduino.h>
#include <LumaWave.h>

/*
Target: Arduino platforms with enough RAM for 1024 RGB pixels (RP2040 recommended).
Requires: One-wire transport and four data pins.
Namespace mode: Explicit-safe (`lw::...`).
API assumptions: Demonstrates `Topology` map for a 2x2 tile canvas using separate
BusBuilder instances over sub-spans of a shared pixel buffer.
*/

constexpr pixel_count_t panelWidth = 16;
constexpr pixel_count_t panelHeight = 16;
constexpr pixel_count_t tilesWide = 2;
constexpr pixel_count_t tilesHigh = 2;
constexpr pixel_count_t stripLedCount = static_cast<pixel_count_t>(panelWidth * panelHeight);

constexpr int dataPin0 = 2;
constexpr int dataPin1 = 3;
constexpr int dataPin2 = 4;
constexpr int dataPin3 = 5;

constexpr pixel_count_t canvasWidth = static_cast<pixel_count_t>(panelWidth * tilesWide);
constexpr pixel_count_t canvasHeight = static_cast<pixel_count_t>(panelHeight * tilesHigh);
constexpr pixel_count_t ledCount = static_cast<pixel_count_t>(canvasWidth * canvasHeight);

// Single shared pixel buffer for all four strips.
lw::Pixel g_pixels[ledCount];

auto strip1 = lw::buses::BusBuilder()
                    .setPixelStorage(lw::span<lw::Pixel>{g_pixels + 0 * stripLedCount, stripLedCount})
                    .setTransport(lw::transports::RpPioTransport{{.dataPin = dataPin0}})
                    .setProtocol(lw::protocols::Ws2812xProtocol(stripLedCount, lw::protocols::Ws2812xProtocolSettings{}))
                    .build();

auto strip2 = lw::buses::BusBuilder()
                    .setPixelStorage(lw::span<lw::Pixel>{g_pixels + 1 * stripLedCount, stripLedCount})
                    .setTransport(lw::transports::RpPioTransport{{.dataPin = dataPin1}})
                    .setProtocol(lw::protocols::Ws2812xProtocol(stripLedCount, lw::protocols::Ws2812xProtocolSettings{}))
                    .build();

auto strip3 = lw::buses::BusBuilder()
                    .setPixelStorage(lw::span<lw::Pixel>{g_pixels + 2 * stripLedCount, stripLedCount})
                    .setTransport(lw::transports::RpPioTransport{{.dataPin = dataPin2}})
                    .setProtocol(lw::protocols::Ws2812xProtocol(stripLedCount, lw::protocols::Ws2812xProtocolSettings{}))
                    .build();

auto strip4 = lw::buses::BusBuilder()
                    .setPixelStorage(lw::span<lw::Pixel>{g_pixels + 3 * stripLedCount, stripLedCount})
                    .setTransport(lw::transports::RpPioTransport{{.dataPin = dataPin3}})
                    .setProtocol(lw::protocols::Ws2812xProtocol(stripLedCount, lw::protocols::Ws2812xProtocolSettings{}))
                    .build();

lw::Topology topology({
    .panelWidth = panelWidth,
    .panelHeight = panelHeight,
    .layout = GridMapping::RowsFirstSerpentine,
    .tilesWide = tilesWide,
    .tilesHigh = tilesHigh,
    .tileLayout = GridMapping::RowsFirstProgressive,
    .mosaicRotation = false,
});
uint16_t phase = 0;

void setup()
{
  strip1->begin();
  strip2->begin();
  strip3->begin();
  strip4->begin();
}

void loop()
{
  while (true)
  {
    for (uint16_t y = 0; y < canvasHeight; ++y)
    {
      for (uint16_t x = 0; x < canvasWidth; ++x)
      {
        const size_t index = topology.map(static_cast<int16_t>(x), static_cast<int16_t>(y));
        if (index == lw::Topology::InvalidIndex)
        {
          continue;
        }

        g_pixels[index] = Color(static_cast<uint8_t>((x + phase) & 0x7F), static_cast<uint8_t>((y + phase) & 0x7F), static_cast<uint8_t>((x + y + phase) & 0x7F));
      }
    }

    strip1->show();
    strip2->show();
    strip3->show();
    strip4->show();
    ++phase;
    delay(30);
  }
