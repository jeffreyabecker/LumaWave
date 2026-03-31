#pragma once

#include <memory>
#include <vector>
#include <limits>

#include "core/IPixelBus.h"

namespace lw::busses
{

namespace detail
{

template <typename TColor, typename TBuses>
std::vector<typename PixelView<TColor>::ChunkType> collectAggregateChunks(const TBuses& buses)
{
    using ChunkType = typename PixelView<TColor>::ChunkType;

    std::vector<ChunkType> chunks;

    for (const auto& bus : buses)
    {
        if (!bus)
        {
            continue;
        }

        const PixelView<TColor>& view = bus->pixels();

        for (auto chunk : view.chunks())
        {
            chunks.push_back(chunk);
        }
    }

    return chunks;
}

} // namespace detail

template <typename TColor> class ReferenceAggregateBus : public IPixelBus<TColor>
{
  public:
    using BusType = IPixelBus<TColor>;
    using ChunkType = typename PixelView<TColor>::ChunkType;

    explicit ReferenceAggregateBus(span<BusType*> buses)
        : _buses(buses.begin(), buses.end()), _pixelChunks(detail::collectAggregateChunks<TColor>(_buses)),
          _pixels(span<ChunkType>{_pixelChunks.data(), _pixelChunks.size()})
    {
    }

    void begin() override
    {
        for (auto* bus : _buses)
        {
            if (bus != nullptr)
            {
                bus->begin();
            }
        }
    }

    void show() override
    {
        for (auto* bus : _buses)
        {
            if (bus != nullptr)
            {
                bus->show();
            }
        }
    }

    bool isReadyToUpdate() const override
    {
        for (const auto* bus : _buses)
        {
            if (bus == nullptr || !bus->isReadyToUpdate())
            {
                return false;
            }
        }

        return true;
    }

    PixelView<TColor>& pixels() override { return _pixels; }

    const PixelView<TColor>& pixels() const override { return _pixels; }

    void setGlobalBrightness(uint16_t brightness) override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                bus->setGlobalBrightness(brightness);
            }
        }
    }

    uint16_t globalBrightness() const override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                return bus->globalBrightness();
            }
        }

        return static_cast<uint16_t>(std::numeric_limits<uint16_t>::max());
    }

    void setGlobalBrightness(uint16_t brightness) override
    {
        for (auto* bus : _buses)
        {
            if (bus)
            {
                bus->setGlobalBrightness(brightness);
            }
        }
    }

    uint16_t globalBrightness() const override
    {
        for (const auto* bus : _buses)
        {
            if (bus)
            {
                return bus->globalBrightness();
            }
        }

        return static_cast<uint16_t>(std::numeric_limits<uint16_t>::max());
    }

  private:
    std::vector<BusType*> _buses;
    std::vector<ChunkType> _pixelChunks;
    PixelView<TColor> _pixels;
};

template <typename TColor> class AggregateBus : public IPixelBus<TColor>
{
  public:
    using BusType = IPixelBus<TColor>;
    using ChunkType = typename PixelView<TColor>::ChunkType;

    explicit AggregateBus(std::vector<std::unique_ptr<BusType>> buses)
        : _buses(std::move(buses)), _pixelChunks(detail::collectAggregateChunks<TColor>(_buses)),
          _pixels(span<ChunkType>{_pixelChunks.data(), _pixelChunks.size()})
    {
    }

    void begin() override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                bus->begin();
            }
        }
    }

    void show() override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                bus->show();
            }
        }
    }

    bool isReadyToUpdate() const override
    {
        for (const auto& bus : _buses)
        {
            if (!bus || !bus->isReadyToUpdate())
            {
                return false;
            }
        }

        return true;
    }

    PixelView<TColor>& pixels() override { return _pixels; }

    const PixelView<TColor>& pixels() const override { return _pixels; }

  private:
    std::vector<std::unique_ptr<BusType>> _buses;
    std::vector<ChunkType> _pixelChunks;
    PixelView<TColor> _pixels;
};

} // namespace lw::busses
