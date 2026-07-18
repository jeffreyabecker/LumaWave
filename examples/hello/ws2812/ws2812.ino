#include <Arduino.h>
#include <LumaWave.h>

constexpr pixel_count_t ledCount = 30;

// Caller-owned pixel storage
lw::Color pixels[30];

// Caller-owned protocol and transport
lw::protocols::Ws2812xProtocol ws2812proto(ledCount, {lw::ChannelOrder::GRB, lw::protocols::timing::Ws2812x});
lw::transports::Transport nilTransport;

// Caller-owned buffers
constexpr size_t protocolBufferSize = decltype(ws2812proto)::requiredBufferSize(ledCount, {lw::ChannelOrder::GRB, lw::protocols::timing::Ws2812x});
uint8_t protocolBuffer[protocolBufferSize];
lw::Color scratchPixels[ledCount];

// Shader chain with brightness
lw::protocols::BrightnessShader brightnessShader;
lw::protocols::IShader* shaders[] = {&brightnessShader};
lw::protocols::ShaderProtocol shaderProto(ws2812proto, shaders, scratchPixels);

// Caller-owned pipeline and run array
lw::busses::ProtocolTransportPipeline pipeline(shaderProto, nilTransport, protocolBuffer);
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
      pixels[i] = Color(static_cast<uint8_t>((i + frame) & 0x3F), static_cast<uint8_t>((2U * i + frame) & 0x3F), static_cast<uint8_t>((3U * i + frame) & 0x3F));
    }

    strip.show();
    ++frame;
    delay(20);
  }
