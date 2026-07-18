#include <Arduino.h>
#include <LumaWave.h>

/*
Target: Arduino platforms with SPI support.
Requires: `LW_HAS_SPI_TRANSPORT` and valid SPI pins.
API: Direct Bus construction with ProtocolTransportPipeline.
*/

constexpr pixel_count_t ledCount = 60;
constexpr int clockPin = 18;
constexpr int dataPin = 23;

// Caller-owned pixel storage
lw::Color pixels[60];

// Caller-owned protocol and transport
lw::protocols::Apa102Protocol apa102proto(ledCount, {lw::ChannelOrder::BGR});
lw::transports::SpiTransport spiTransport(lw::transports::SpiTransportSettings{{false, 8000000UL, lw::transports::BitOrderMsbFirst, lw::transports::SpiMode0, clockPin, dataPin}});

// Caller-owned buffers
constexpr size_t protocolBufferSize = decltype(apa102proto)::requiredBufferSize(ledCount, {lw::ChannelOrder::BGR});
uint8_t protocolBuffer[protocolBufferSize];
lw::Color scratchPixels[ledCount];

// Shader chain with brightness
lw::protocols::BrightnessShader brightnessShader;
lw::protocols::IShader* shaders[] = {&brightnessShader};
lw::protocols::ShaderProtocol shaderProto(apa102proto, shaders, scratchPixels);

// Caller-owned pipeline and run array
lw::busses::ProtocolTransportPipeline pipeline(shaderProto, spiTransport, protocolBuffer);
lw::busses::PipelineRun runs[] = {{&pipeline, ledCount}};

// Bus: non-owning view over caller-managed resources
lw::busses::Bus strip(lw::span<lw::Color>{pixels}, lw::span<const lw::busses::PipelineRun>{runs});
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
      pixels[i] = lw::Color(static_cast<uint8_t>((i + frame) & 0x3F), static_cast<uint8_t>((2U * i + frame) & 0x3F), static_cast<uint8_t>((3 * i + frame) & 0x3F));
    }

    strip.show();
    ++frame;
    delay(20);
  }
