#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "colors/IShader.h"
#include "colors/ColorMath.h"
#include "core/IPixelBus.h"
#include <limits>
#include "protocols/IProtocol.h"
#include "transports/ITransport.h"

namespace lw::busses
{

template <typename TColor> class ReferenceBus : public IPixelBus<TColor>
{
public:
  using BrightnessType = typename IPixelBus<TColor>::BrightnessType;

  ReferenceBus(PixelCount pixelCount, std::unique_ptr<protocols::IProtocol<TColor>> protocol, std::unique_ptr<transports::ITransport> transport, std::unique_ptr<IShader<TColor>> shader = nullptr)
      : _pixelCount(pixelCount), _rootBuffer(allocateColorBuffer(_pixelCount)), _protocol(std::move(protocol)), _protocolBuffer(allocateByteBuffer(protocolBufferSize(_protocol))), _transport(std::move(transport)),
        _shader(std::move(shader)), _shaderBuffer(allocateColorBuffer(_pixelCount)), _pixels(makePixelChunk(_rootBuffer.get(), _pixelCount))
  {
  }

  void begin() override
  {
    if (_transport)
    {
      _transport->begin();
    }

    if (_protocol)
    {
      _protocol->begin();
    }
  }

  void show() override
  {
    if (!_protocol || !_transport)
    {
      return;
    }

    if (!_dirty && !_protocol->alwaysUpdate())
    {
      return;
    }

    if (!_transport->isReadyToUpdate())
    {
      return;
    }

    span<const TColor> protocolInput{};
    span<TColor> mutableProtocolInput{};
    auto shaderOwnership = shaders::BrightnessOwnership::None;
    bool brightnessAppliedUpstream = false;

    if (_rootBuffer && _pixelCount > 0)
    {
      if (_shader && _shaderBuffer)
      {
        std::copy_n(_rootBuffer.get(), _pixelCount, _shaderBuffer.get());
        span<TColor> shaderSpan{_shaderBuffer.get(), _pixelCount};
        _shader->apply(shaderSpan);
        shaderOwnership = _shader->brightnessOwnership();
        if (shaderOwnership == shaders::BrightnessOwnership::Owns)
        {
          _shader->applyBrightness(shaderSpan, _brightness);
          brightnessAppliedUpstream = (_brightness != std::numeric_limits<BrightnessType>::max());
        }
        mutableProtocolInput = shaderSpan;
      }
      else if (_shader)
      {
        span<TColor> rootSpan{_rootBuffer.get(), _pixelCount};
        _shader->apply(rootSpan);
        shaderOwnership = _shader->brightnessOwnership();
        if (shaderOwnership == shaders::BrightnessOwnership::Owns)
        {
          _shader->applyBrightness(rootSpan, _brightness);
          brightnessAppliedUpstream = (_brightness != std::numeric_limits<BrightnessType>::max());
        }
        mutableProtocolInput = rootSpan;
      }
      else if ((_brightness != std::numeric_limits<BrightnessType>::max()) && _shaderBuffer)
      {
        std::copy_n(_rootBuffer.get(), _pixelCount, _shaderBuffer.get());
        mutableProtocolInput = span<TColor>{_shaderBuffer.get(), _pixelCount};
      }
      else
      {
        protocolInput = span<const TColor>{_rootBuffer.get(), _pixelCount};
      }
    }

    if ((shaderOwnership != shaders::BrightnessOwnership::Owns) && (_brightness != std::numeric_limits<BrightnessType>::max()) && !mutableProtocolInput.empty())
    {
      if constexpr (lw::ColorType<TColor>)
      {
        for (auto& color : mutableProtocolInput)
        {
          for (auto channel : TColor::channelIndexes())
          {
            color[channel] = static_cast<typename TColor::ComponentType>(lw::colors::applyBrightness(color[channel], _brightness));
          }
        }

        brightnessAppliedUpstream = true;
      }
    }

    if (!mutableProtocolInput.empty())
    {
      protocolInput = mutableProtocolInput;
    }

    span<uint8_t> protocolBytes{};
    const size_t requiredSize = _protocol->requiredBufferSizeBytes();
    if (_protocolBuffer && requiredSize > 0)
    {
      protocolBytes = span<uint8_t>{_protocolBuffer.get(), requiredSize};
    }

    _protocol->update(protocolInput, protocolBytes);

    if (!protocolBytes.empty())
    {
      _transport->beginTransaction();
      _transport->transmitBytes(protocolBytes, transports::TransportBrightness::from(_brightness, brightnessAppliedUpstream));
      _transport->endTransaction();
    }

    _dirty = false;
  }

  bool isReadyToUpdate() const override
  {
    if (!_transport)
    {
      return false;
    }

    return _transport->isReadyToUpdate();
  }

  PixelView<TColor>& pixels() override
  {
    _dirty = true;
    return _pixels;
  }

  const PixelView<TColor>& pixels() const override { return _pixels; }

  PixelCount pixelCount() const { return _pixelCount; }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

  TColor* rootBuffer() { return _rootBuffer.get(); }

  const TColor* rootBuffer() const { return _rootBuffer.get(); }

  TColor* shaderBuffer() { return _shaderBuffer.get(); }

  const TColor* shaderBuffer() const { return _shaderBuffer.get(); }

  uint8_t* protocolBuffer() { return _protocolBuffer.get(); }

  const uint8_t* protocolBuffer() const { return _protocolBuffer.get(); }

  protocols::IProtocol<TColor>* protocol() { return _protocol.get(); }

  const protocols::IProtocol<TColor>* protocol() const { return _protocol.get(); }

  transports::ITransport* transport() { return _transport.get(); }

  const transports::ITransport* transport() const { return _transport.get(); }

  IShader<TColor>* shader() { return _shader.get(); }

  const IShader<TColor>* shader() const { return _shader.get(); }

private:
  static std::unique_ptr<TColor[]> allocateColorBuffer(PixelCount pixelCount)
  {
    if (pixelCount == 0)
    {
      return nullptr;
    }

    return std::make_unique<TColor[]>(pixelCount);
  }

  static size_t protocolBufferSize(const std::unique_ptr<protocols::IProtocol<TColor>>& protocol) { return protocol ? protocol->requiredBufferSizeBytes() : 0u; }

  static std::unique_ptr<uint8_t[]> allocateByteBuffer(size_t size)
  {
    if (size == 0)
    {
      return nullptr;
    }

    return std::make_unique<uint8_t[]>(size);
  }

  static span<TColor> makePixelChunk(TColor* buffer, PixelCount pixelCount)
  {
    if (buffer == nullptr || pixelCount == 0)
    {
      return span<TColor>{};
    }

    return span<TColor>{buffer, pixelCount};
  }

  PixelCount _pixelCount{0};
  std::unique_ptr<TColor[]> _rootBuffer;
  std::unique_ptr<protocols::IProtocol<TColor>> _protocol;
  std::unique_ptr<uint8_t[]> _protocolBuffer;
  std::unique_ptr<transports::ITransport> _transport;
  std::unique_ptr<IShader<TColor>> _shader;
  std::unique_ptr<TColor[]> _shaderBuffer;
  PixelView<TColor> _pixels;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses
