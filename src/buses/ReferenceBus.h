#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

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

  ReferenceBus(PixelCount pixelCount, std::unique_ptr<protocols::IProtocol<TColor>> protocol, std::unique_ptr<transports::ITransport> transport)
      : _pixelCount(pixelCount), _rootBuffer(allocateColorBuffer(_pixelCount)), _protocol(std::move(protocol)), _protocolBuffer(allocateByteBuffer(protocolBufferSize(_protocol))), _transport(std::move(transport)),
        _brightnessScratch(allocateColorBuffer(_pixelCount)), _pixels(makePixelChunk(_rootBuffer.get(), _pixelCount))
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

    if (_rootBuffer && _pixelCount > 0)
    {
      protocolInput = span<const TColor>{_rootBuffer.get(), _pixelCount};
    }

    // Apply bus-level global brightness scaling before protocol encoding.
    if ((_brightness != std::numeric_limits<BrightnessType>::max()) && !protocolInput.empty() && _brightnessScratch)
    {
      const size_t count = protocolInput.size();
      for (size_t i = 0; i < count; ++i)
      {
        _brightnessScratch[i] = protocolInput[i];
        auto& dst = _brightnessScratch[i];
        for (auto channel : TColor::channelIndexes())
        {
          dst[channel] = static_cast<typename TColor::ComponentType>(lw::colors::applyBrightness(dst[channel], _brightness));
        }
      }
      protocolInput = span<const TColor>{_brightnessScratch.get(), count};
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
      _transport->transmitBytes(protocolBytes, transports::TransportBrightness::from(_brightness, false));
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

  size_t pixelCount() const { return _pixelCount; }

  TColor* rootBuffer() { return _rootBuffer.get(); }

  const TColor* rootBuffer() const { return _rootBuffer.get(); }

  protocols::IProtocol<TColor>* protocol() { return _protocol.get(); }

  const protocols::IProtocol<TColor>* protocol() const { return _protocol.get(); }

  transports::ITransport* transport() { return _transport.get(); }

  const transports::ITransport* transport() const { return _transport.get(); }

  void setBrightness(BrightnessType brightness) override { _brightness = brightness; }
  BrightnessType brightness() const override { return _brightness; }

private:
  static std::unique_ptr<TColor[]> allocateColorBuffer(size_t count)
  {
    if (count == 0)
    {
      return nullptr;
    }

    return std::make_unique<TColor[]>(count);
  }

  static std::unique_ptr<uint8_t[]> allocateByteBuffer(size_t size)
  {
    if (size == 0)
    {
      return nullptr;
    }

    return std::make_unique<uint8_t[]>(size);
  }

  static size_t protocolBufferSize(const std::unique_ptr<protocols::IProtocol<TColor>>& protocol)
  {
    if (!protocol)
    {
      return 0;
    }

    return protocol->requiredBufferSizeBytes();
  }

  static span<TColor> makePixelChunk(TColor* buffer, size_t count)
  {
    if (buffer == nullptr || count == 0)
    {
      return span<TColor>{};
    }

    return span<TColor>{buffer, count};
  }

  PixelCount _pixelCount{0};
  std::unique_ptr<TColor[]> _rootBuffer;
  std::unique_ptr<protocols::IProtocol<TColor>> _protocol;
  std::unique_ptr<uint8_t[]> _protocolBuffer;
  std::unique_ptr<transports::ITransport> _transport;
  std::unique_ptr<TColor[]> _brightnessScratch;
  PixelView<TColor> _pixels;
  BrightnessType _brightness{std::numeric_limits<BrightnessType>::max()};
  bool _dirty{true};
};

} // namespace lw::busses
