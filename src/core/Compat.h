#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#if defined(ARDUINO)
#define LW_HAS_ARDUINO 1
#elif defined(__has_include)
#if __has_include(<Arduino.h>)
#define LW_HAS_ARDUINO 1
#else
#define LW_HAS_ARDUINO 0
#endif
#else
#define LW_HAS_ARDUINO 0
#endif

#if defined(__has_include)
#if __has_include(<SPI.h>)
#define LW_HAS_SPI_TRANSPORT 1
#endif
#endif

#ifndef LW_SPI_CLOCK_DEFAULT_HZ
#define LW_SPI_CLOCK_DEFAULT_HZ 10000000UL
#endif

#ifndef LW_ESP32_DMA_SPI_CLOCK_DEFAULT_HZ
#define LW_ESP32_DMA_SPI_CLOCK_DEFAULT_HZ 10000000UL
#endif

#ifndef LW_ESP32_DMA_SPI_DEFAULT_HOST
#if defined(SPI2_HOST)
#define LW_ESP32_DMA_SPI_DEFAULT_HOST SPI2_HOST
#else
#define LW_ESP32_DMA_SPI_DEFAULT_HOST 1
#endif
#endif

#ifndef LW_RP_DMA_IRQ_INDEX
#define LW_RP_DMA_IRQ_INDEX 1
#endif

#ifndef LW_PIXEL_COMPONENT_SIZE
#define LW_PIXEL_COMPONENT_SIZE 8
#endif

#ifndef LW_USE_PLATFORM_RANDOM
#define LW_USE_PLATFORM_RANDOM 0
#endif

namespace lw
{

using PixelCount = uint16_t;

static constexpr std::size_t dynamic_extent = std::dynamic_extent;
template <typename T, std::size_t Extent = std::dynamic_extent> using span = std::span<T, Extent>;

} // namespace lw
