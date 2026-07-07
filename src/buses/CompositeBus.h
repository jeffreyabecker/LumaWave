#pragma once

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <limits>

#include "core/IPixelBus.h"

namespace lw::busses
{

#if !LW_DISABLE_TEMPLATE_COMBINATORIAL_TYPES
template <typename... TBuses> class CompositeBus : public IPixelBus
{
public:
  static_assert(sizeof...(TBuses) > 0, "CompositeBus requires at least one bus type.");

  using ColorType = typename std::tuple_element<0, std::tuple<TBuses...>>::type::ColorType;
  using BusBaseType = IPixelBus;
  using BrightnessType = typename BusBaseType::BrightnessType;
  using ChunkType = typename PixelView::ChunkType;
  using BusesTupleType = std::tuple<TBuses...>;

  static_assert((std::is_convertible_v<TBuses*, BusBaseType*> && ...), "All CompositeBus members must derive from IPixelBus.");

  explicit CompositeBus(TBuses... buses) : _buses(std::move(buses)...), _busPointers(makeBusPointers(_buses)), _pixelChunks(collectChunks(_busPointers)), _pixels(span<PixelView::ChunkType>{_pixelChunks.data(), _pixelChunks.size()}) {}

  void begin() override
  {
    for (auto* bus : _busPointers)
    {
      if (bus != nullptr)
      {
        bus->begin();
      }
    }
  }

  void show() override
  {
    for (auto* bus : _busPointers)
    {
      if (bus != nullptr)
      {
        bus->show();
      }
    }
  }

  bool isReadyToUpdate() const override
  {
    for (const auto* bus : _busPointers)
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

  BusesTupleType& buses() { return _buses; }

  const BusesTupleType& buses() const { return _buses; }

  void setBrightness(BrightnessType brightness) override
  {
    for (auto* bus : _busPointers)
    {
      if (bus)
      {
        bus->setBrightness(brightness);
      }
    }
  }

  BrightnessType brightness() const override
  {
    for (auto* bus : _busPointers)
    {
      if (bus)
      {
        return bus->brightness();
      }
    }

    return std::numeric_limits<BrightnessType>::max();
  }

private:
  template <size_t... TIndices> static std::array<BusBaseType*, sizeof...(TBuses)> makeBusPointers(BusesTupleType& buses, std::index_sequence<TIndices...>)
  {
    return {static_cast<BusBaseType*>(&std::get<TIndices>(buses))...};
  }

  static std::array<BusBaseType*, sizeof...(TBuses)> makeBusPointers(BusesTupleType& buses) { return makeBusPointers(buses, std::index_sequence_for<TBuses...>{}); }

  static std::vector<ChunkType> collectChunks(const std::array<BusBaseType*, sizeof...(TBuses)>& buses)
  {
    std::vector<ChunkType> chunks;

    for (auto* bus : buses)
    {
      if (bus == nullptr)
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

  BusesTupleType _buses;
  std::array<BusBaseType*, sizeof...(TBuses)> _busPointers;
  std::vector<ChunkType> _pixelChunks;
  PixelView _pixels;
};

#endif

} // namespace lw::busses
