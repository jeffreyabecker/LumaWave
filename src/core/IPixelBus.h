#pragma once

#include <cstddef>
#include <cstdint>

#include "core/Compat.h"
#include "core/PixelView.h"

namespace lw
{
template <typename TColor> class IPixelBus
{
  public:
    virtual ~IPixelBus() = default;

    virtual void begin() = 0;
    virtual void show() = 0;
    virtual bool isReadyToUpdate() const = 0;

    // Global brightness contract (0..65535). Buses own the user-facing
    // brightness state and must expose setter/getter here so callers can
    // manipulate brightness uniformly across bus types.
    virtual void setGlobalBrightness(uint16_t brightness) = 0;
    virtual uint16_t globalBrightness() const = 0;

    virtual PixelView<TColor>& pixels() = 0;
    virtual const PixelView<TColor>& pixels() const = 0;
};

} // namespace lw
