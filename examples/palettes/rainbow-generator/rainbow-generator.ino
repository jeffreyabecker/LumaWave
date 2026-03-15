#include <Arduino.h>
#include <LumaWave.h>

constexpr pixel_count_t ledCount = 30;
Strip<Protocols::Ws2812> strip(ledCount, Transport::DefaultSettings{{.dataPin = 2}});
uint16_t frame = 0;

lw::colors::palettes::RainbowPaletteGenerator<Color> generator(16, 255u, 217u, 0);

void setup()
{
    strip.begin();
}

void loop()
{
    while (true)
    {
        generator.update(2);
        samplePalette(generator, static_cast<size_t>(frame), strip.pixels());

        strip.show();
        ++frame;
        delay(20);
    }
}
