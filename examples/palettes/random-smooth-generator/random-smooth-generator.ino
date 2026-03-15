#include <Arduino.h>
#include <LumaWave.h>

constexpr pixel_count_t ledCount = 30;
Strip<Protocols::Ws2812> strip(ledCount, Transport::DefaultSettings{{.dataPin = 2}});
uint16_t frame = 0;

lw::colors::palettes::RandomSmoothPaletteGenerator<Color> generator;

void setup()
{
    generator.setStopCount(8);
    generator.setSeed(0xABCD1234u);
    generator.setProgressStep(10);
    strip.begin();
}

void loop()
{
    while (true)
    {
        generator.update();
        samplePalette(generator, static_cast<size_t>(frame), strip.pixels());

        strip.show();
        frame = static_cast<uint16_t>(frame + 3U);
        delay(20);
    }
}
