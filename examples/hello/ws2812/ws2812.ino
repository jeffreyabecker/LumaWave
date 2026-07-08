#include <Arduino.h>
#include <LumaWave.h>

constexpr pixel_count_t ledCount = 30;
lw::Color pixels[30];
lw::busses::Bus strip(lw::span<lw::Color>{pixels}, {
    {std::make_unique<lw::busses::ProtocolTransportPipeline>(
         std::make_unique<Protocols::Ws2812::ProtocolType>(ledCount, Protocols::Ws2812::defaultSettings()),
         std::make_unique<lw::transports::NilTransport>()), ledCount}
});
uint16_t frame = 0;

void setup()
{
    strip.begin();
}

void loop()
{
    auto& pixels = strip.pixels();
    const size_t count = pixels.size();
    while (true)
    {

        for (size_t i = 0; i < count; ++i)
        {
            pixels[i] = Color(static_cast<uint8_t>((i + frame) & 0x3F), static_cast<uint8_t>((2U * i + frame) & 0x3F),
                              static_cast<uint8_t>((3U * i + frame) & 0x3F));
        }

        strip.show();
        ++frame;
        delay(20);
    }
