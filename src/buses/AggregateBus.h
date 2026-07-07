#pragma once

#include <memory>
#include <vector>
#include <limits>

#include "core/IPixelBus.h"

namespace lw::busses
{

namespace detail
{

template < typename TBuses>
std::vector<typename PixelView::ChunkType> collectAggregateChunks(const TBuses& buses)
{
    using ChunkType = typename PixelView::ChunkType;

    std::vector<ChunkType> chunks;

    for (const auto& bus : buses)
    {
        if (!bus)
        {
            continue;
        }

        const PixelView& view = bus->pixels();

        for (auto chunk : view.chunks())
        {
            chunks.push_back(chunk);
        }
    }

    return chunks;
}

} // namespace detail

 class ReferenceAggregateBus : public IPixelBus
{
  public:
    using BusType = IPixelBus;
        using BrightnessType = typename BusType::BrightnessType;
    using ChunkType = typename PixelView::ChunkType;

    explicit ReferenceAggregateBus(span<BusType*> buses)
        : _buses(buses.begin(), buses.end()), _pixelChunks(detail::collectAggregateChunks(_buses)),
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

    PixelView& pixels() override { return _pixels; }

    const PixelView& pixels() const override { return _pixels; }

    void setBrightness(BrightnessType brightness) override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                bus->setBrightness(brightness);
            }
        }
    }

    BrightnessType brightness() const override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                return bus->brightness();
            }
        }

        return std::numeric_limits<BrightnessType>::max();
    }

  private:
    std::vector<BusType*> _buses;
    std::vector<ChunkType> _pixelChunks;
    PixelView _pixels;
};

 class AggregateBus : public IPixelBus
{
  public:
    using BusType = IPixelBus;
        using BrightnessType = typename BusType::BrightnessType;
    using ChunkType = typename PixelView::ChunkType;

    explicit AggregateBus(std::vector<std::unique_ptr<BusType>> buses)
        : _buses(std::move(buses)), _pixelChunks(detail::collectAggregateChunks(_buses)),
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

    PixelView& pixels() override { return _pixels; }

    const PixelView& pixels() const override { return _pixels; }

    void setBrightness(BrightnessType brightness) override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                bus->setBrightness(brightness);
            }
        }
    }

    BrightnessType brightness() const override
    {
        for (const auto& bus : _buses)
        {
            if (bus)
            {
                return bus->brightness();
            }
        }

        return std::numeric_limits<BrightnessType>::max();
    }

  private:
    std::vector<std::unique_ptr<BusType>> _buses;
    std::vector<ChunkType> _pixelChunks;
    PixelView _pixels;
};

} // namespace lw::busses
