#include <Arduino.h>
#include <LumaWave.h>

constexpr pixel_count_t ledCount = 30;
Strip<Protocols::Ws2812> strip(ledCount, Transport::DefaultSettings{{.dataPin = 2}});
uint16_t frame = 0;

lw::palettes::RainbowPaletteGenerator<Color> generator;

void setup()
{
    generator.setStopCount(16);
    generator.setSaturation(255u);
    generator.setBrightness(217u);
    generator.setHueOffset(0u);
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
