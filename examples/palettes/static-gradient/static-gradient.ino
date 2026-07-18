#include <Arduino.h>
#include <LumaWave.h>

#include <array>

constexpr pixel_count_t ledCount = 30;
Strip<Protocols::Ws2812> strip(ledCount, Transport::DefaultSettings{{.dataPin = 2}});
uint16_t frame = 0;

using Stop = lw::palettes::PaletteStop<Color>;

constexpr std::array<Stop, 5> kStops = {
    Stop{0, Color(255, 0, 0)},   Stop{64, Color(255, 128, 0)},  Stop{128, Color(0, 255, 0)},
    Stop{192, Color(0, 0, 255)}, Stop{255, Color(128, 0, 255)},
};

const Palette<Color> palette(lw::span<const Stop>(kStops.data(), kStops.size()));

void setup()
{
    strip.begin();
}

void loop()
{
    while (true)
    {
        samplePalette(palette, static_cast<size_t>(frame), strip.pixels());

        strip.show();
        frame = static_cast<uint16_t>(frame + 2U);
        delay(20);
    }
}
