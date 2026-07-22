#include <Arduino.h>
#include <LumaWave.h>

// ---------------------------------------------------------------------------
// Presets
//
// Demonstrates using protocol, transport, and shader presets with addStrip().
// Presets are plain structs with a configure(BusBuilder&) method — no
// inheritance required. Fields can be overridden inline.
//
// All presets live in lw::buses::presets. Import them with a using directive
// for concise syntax.
// ---------------------------------------------------------------------------

using namespace lw::buses::presets;

constexpr size_t kLedCount = 30;

void setup()
{
  // --- Single strip with protocol + transport presets ---
  auto strip1 = lw::buses::BusBuilder().setPixelCount(kLedCount).addStrip(ws2812x{}, nil_transport{}).build();

  strip1->begin();
  strip1->pixels()[0] = lw::pixelFromRGB(255, 0, 0);
  strip1->show();

  // --- Inline field override ---
  // Override the default GRB channel order to RGB.
  auto strip2 = lw::buses::BusBuilder().setPixelCount(kLedCount).addStrip(ws2812x{.channelOrder = "RGB"}, nil_transport{}).build();

  strip2->begin();
  strip2->show();

  // --- Presets with explicit shader ---
  // Use addStrip for protocol+transport presets, then addShader for the raw shader.
  auto strip3 = lw::buses::BusBuilder().setPixelCount(kLedCount).addStrip(ws2812x{}, nil_transport{}).addShader(lw::protocols::BrightnessShader{128}).build();

  strip3->begin();
  strip3->show();
}

void loop()
{
}
